#include "Debug.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

#include "EventBus.hpp"
#include "SystemEvents.hpp"


namespace {

    // ============================================================
    // Log Entry
    // ============================================================

    struct LogEntry {

        Debug::LogLevel level =
            Debug::LogLevel::Info;

        // --------------------------------------------------------
        // Timestamp
        //
        // Milliseconds since Unix epoch.
        // Formatted only on logger thread.
        // --------------------------------------------------------

        std::uint64_t timestamp = 0;

        // --------------------------------------------------------
        // Thread
        // --------------------------------------------------------

        std::uint32_t threadId = 0;

        // --------------------------------------------------------
        // Source
        // --------------------------------------------------------

        const char* file = nullptr;
        const char* function = nullptr;

        std::uint32_t line = 0;

        // --------------------------------------------------------
        // Message
        // --------------------------------------------------------

        std::uint16_t messageLength = 0;

        char message[
            Debug::LOG_MESSAGE_SIZE
        ]{};
    };


    // ============================================================
    // Fixed-size lock-free queue
    //
    // Multiple producers.
    // Single consumer.
    //
    // Capacity must be power of two.
    // ============================================================

    template<typename T, std::size_t Capacity>
    class LockFreeQueue {

        static_assert(
            (Capacity& (Capacity - 1)) == 0,
            "Queue capacity must be a power of two"
            );

    private:

        struct Cell {

            std::atomic<std::size_t> sequence;

            T data;
        };

        static constexpr std::size_t MASK =
            Capacity - 1;

    public:

        LockFreeQueue() noexcept
        {
            for (std::size_t i = 0; i < Capacity; ++i) {

                m_buffer[i].sequence.store(
                    i,
                    std::memory_order_relaxed
                );
            }
        }


        bool tryPush(const T& value) noexcept
        {
            Cell* cell;

            std::size_t position =
                m_enqueuePosition.load(
                    std::memory_order_relaxed
                );

            for (;;) {

                cell =
                    &m_buffer[position & MASK];

                const std::size_t sequence =
                    cell->sequence.load(
                        std::memory_order_acquire
                    );

                const std::intptr_t difference =
                    static_cast<std::intptr_t>(sequence)
                    -
                    static_cast<std::intptr_t>(position);

                if (difference == 0) {

                    if (
                        m_enqueuePosition.compare_exchange_weak(
                            position,
                            position + 1,
                            std::memory_order_relaxed
                        )
                        ) {

                        break;
                    }

                }
                else if (difference < 0) {

                    // Queue full.
                    return false;
                }
                else {

                    position =
                        m_enqueuePosition.load(
                            std::memory_order_relaxed
                        );
                }
            }

            cell->data = value;

            cell->sequence.store(
                position + 1,
                std::memory_order_release
            );

            return true;
        }


        bool tryPop(T& value) noexcept
        {
            const std::size_t position =
                m_dequeuePosition.load(
                    std::memory_order_relaxed
                );

            Cell& cell =
                m_buffer[position & MASK];

            const std::size_t sequence =
                cell.sequence.load(
                    std::memory_order_acquire
                );

            const std::intptr_t difference =
                static_cast<std::intptr_t>(sequence)
                -
                static_cast<std::intptr_t>(position + 1);

            if (difference != 0) {
                return false;
            }

            value = cell.data;

            cell.sequence.store(
                position + Capacity,
                std::memory_order_release
            );

            m_dequeuePosition.store(
                position + 1,
                std::memory_order_relaxed
            );

            return true;
        }

    private:

        alignas(64)
            std::atomic<std::size_t>
            m_enqueuePosition{ 0 };

        alignas(64)
            std::atomic<std::size_t>
            m_dequeuePosition{ 0 };

        alignas(64)
            std::array<Cell, Capacity>
            m_buffer{};
    };


    // ============================================================
    // Logger
    // ============================================================

    class Logger {

    public:

        Logger() = default;

        ~Logger()
        {
            stop();
        }


        // ========================================================
        // Lifecycle
        // ========================================================

        void start(const Debug::Config& config)
        {
            bool expected = false;

            if (!m_running.compare_exchange_strong(
                expected,
                true,
                std::memory_order_acq_rel
            )
                ) {
                return;
            }

            m_timestamp.store(
                config.timestamp,
                std::memory_order_relaxed
            );

            m_source.store(
                config.source,
                std::memory_order_relaxed
            );

            m_threadId.store(
                config.threadId,
                std::memory_order_relaxed
            );

            m_consoleOutput.store(
                config.consoleOutput,
                std::memory_order_relaxed
            );

            m_fileOutput.store(
                config.fileOutput,
                std::memory_order_relaxed
            );

            m_eventBusOutput.store(
                config.eventBusOutput,
                std::memory_order_relaxed
            );

            m_minimumLevel.store(
                config.minimumLevel,
                std::memory_order_relaxed
            );


            // ----------------------------------------------------
            // File
            // ----------------------------------------------------

            if (
                config.fileOutput
                &&
                config.filePath != nullptr
                ) {

                m_file.open(
                    config.filePath,
                    std::ios::out |
                    std::ios::app
                );
            }


            // ----------------------------------------------------
            // Worker
            // ----------------------------------------------------

            m_worker =
                std::thread(
                    &Logger::workerLoop,
                    this
                );
        }


        void stop()
        {
            bool expected = true;

            if (!m_running.compare_exchange_strong(
                expected,
                false,
                std::memory_order_acq_rel
            )
                ) {
                return;
            }

            m_condition.notify_one();

            if (m_worker.joinable()) {
                m_worker.join();
            }

            if (m_file.is_open()) {
                m_file.flush();
                m_file.close();
            }
        }


        bool isRunning() const noexcept
        {
            return m_running.load(
                std::memory_order_relaxed
            );
        }


        // ========================================================
        // Runtime configuration
        // ========================================================

        void setLogLevel(Debug::LogLevel level)
        {
            m_minimumLevel.store(
                level,
                std::memory_order_relaxed
            );
        }


        Debug::LogLevel getLogLevel() const
        {
            return m_minimumLevel.load(
                std::memory_order_relaxed
            );
        }


        bool isEnabled(Debug::LogLevel level) const noexcept
        {
            return
                static_cast<std::uint8_t>(level)
                >=
                static_cast<std::uint8_t>(
                    m_minimumLevel.load(
                        std::memory_order_relaxed
                    )
                    );
        }


        void setTimestampEnabled(bool enabled)
        {
            m_timestamp.store(
                enabled,
                std::memory_order_relaxed
            );
        }


        void setSourceEnabled(bool enabled)
        {
            m_source.store(
                enabled,
                std::memory_order_relaxed
            );
        }


        void setThreadIdEnabled(bool enabled)
        {
            m_threadId.store(
                enabled,
                std::memory_order_relaxed
            );
        }


        void setConsoleOutputEnabled(bool enabled)
        {
            m_consoleOutput.store(
                enabled,
                std::memory_order_relaxed
            );
        }


        void setFileOutputEnabled(bool enabled)
        {
            m_fileOutput.store(
                enabled,
                std::memory_order_relaxed
            );
        }


        void setEventBusOutputEnabled(bool enabled)
        {
            m_eventBusOutput.store(
                enabled,
                std::memory_order_relaxed
            );
        }


        // ========================================================
        // Write raw message
        // ========================================================

        void write(
            Debug::LogLevel level,
            std::string_view message,
            std::source_location location
        )
        {
            // ----------------------------------------------------
            // IMPORTANT:
            //
            // Check level BEFORE doing any work.
            // ----------------------------------------------------

            if (!isEnabled(level)) {
                return;
            }

            if (!isRunning()) {
                return;
            }


            LogEntry entry;

            entry.level = level;


            // ----------------------------------------------------
            // Timestamp
            //
            // We use system_clock because we want:
            //
            // YYYY-MM-DD HH:MM:SS.mmm
            //
            // Timestamp is generated here on producer.
            //
            // Formatting itself happens on logger thread.
            // ----------------------------------------------------

            if (
                m_timestamp.load(
                    std::memory_order_relaxed
                )
                ) {

                const auto now =
                    std::chrono::system_clock::now();

                entry.timestamp =
                    static_cast<std::uint64_t>(
                        std::chrono::duration_cast<
                        std::chrono::milliseconds
                        >(
                            now.time_since_epoch()
                        ).count()
                        );
            }


            // ----------------------------------------------------
            // Thread ID
            //
            // Cached using thread_local.
            // First log from a thread pays registration cost.
            // Subsequent logs are very cheap.
            // ----------------------------------------------------

            if (
                m_threadId.load(
                    std::memory_order_relaxed
                )
                ) {

                entry.threadId =
                    getThreadId();
            }


            // ----------------------------------------------------
            // Source
            //
            // Pointers point to static compiler-generated strings.
            // No string allocation.
            // ----------------------------------------------------

            if (
                m_source.load(
                    std::memory_order_relaxed
                )
                ) {

                entry.file =
                    location.file_name();

                entry.function =
                    location.function_name();

                entry.line =
                    location.line();
            }


            // ----------------------------------------------------
            // Message
            // ----------------------------------------------------

            const std::size_t length =
                message.size()
                <
                Debug::LOG_MESSAGE_SIZE - 1
                ? message.size()
                : Debug::LOG_MESSAGE_SIZE - 1;

            std::memcpy(
                entry.message,
                message.data(),
                length
            );

            entry.message[length] = '\0';

            entry.messageLength =
                static_cast<std::uint16_t>(
                    length
                    );


            // ----------------------------------------------------
            // Queue
            // ----------------------------------------------------

            if (!m_queue.tryPush(entry)) {

                m_dropped.fetch_add(
                    1,
                    std::memory_order_relaxed
                );

                // NEVER block game/render thread.
                return;
            }


            // ----------------------------------------------------
            // Wake logger thread
            // ----------------------------------------------------

            m_condition.notify_one();
        }


        // ========================================================
        // Formatted write
        // ========================================================

        void writeFormat(
            Debug::LogLevel level,
            std::source_location location,
            const char* format,
            va_list args
        )
        {
            // ----------------------------------------------------
            // IMPORTANT:
            //
            // If level disabled, vsnprintf is NEVER called.
            // ----------------------------------------------------

            if (!isEnabled(level)) {
                return;
            }

            if (!isRunning()) {
                return;
            }


            // ----------------------------------------------------
            // Stack-only formatting.
            //
            // No std::string.
            // No std::vector.
            // No heap allocation.
            // ----------------------------------------------------

            char buffer[
                Debug::LOG_MESSAGE_SIZE
            ];


            va_list copy;

            va_copy(
                copy,
                args
            );

            const int result =
                std::vsnprintf(
                    buffer,
                    sizeof(buffer),
                    format,
                    copy
                );

            va_end(copy);


            if (result <= 0) {
                return;
            }


            const std::size_t length =
                static_cast<std::size_t>(result)
                <
                sizeof(buffer) - 1
                ? static_cast<std::size_t>(result)
                : sizeof(buffer) - 1;


            write(
                level,
                std::string_view(
                    buffer,
                    length
                ),
                location
            );
        }


        // ========================================================
        // Statistics
        // ========================================================

        std::uint64_t droppedCount() const noexcept
        {
            return m_dropped.load(
                std::memory_order_relaxed
            );
        }


    private:

        // ========================================================
        // Thread ID
        // ========================================================

        static std::uint32_t getThreadId()
        {
            thread_local std::uint32_t id = [] {

                static std::atomic<std::uint32_t>
                    nextId{ 1 };

                return nextId.fetch_add(
                    1,
                    std::memory_order_relaxed
                );

                }();

            return id;
        }


        // ========================================================
        // Level string
        // ========================================================

        static const char* levelString(
            Debug::LogLevel level
        )
        {
            switch (level) {

            case Debug::LogLevel::Info:
                return "INFO";

            case Debug::LogLevel::Warning:
                return "WARN";

            case Debug::LogLevel::Error:
                return "ERROR";

            case Debug::LogLevel::Debug:
                return "DEBUG";
            }

            return "UNKNOWN";
        }


        // ========================================================
        // Timestamp formatting
        // ========================================================

        static std::size_t formatTimestamp(
            std::uint64_t timestamp,
            char* buffer,
            std::size_t bufferSize
        )
        {
            using namespace std::chrono;


            const auto milliseconds =
                static_cast<std::int64_t>(
                    timestamp
                    );


            const auto timePoint =
                system_clock::time_point{
                    std::chrono::milliseconds(
                        milliseconds
                    )
            };


            const auto time =
                system_clock::to_time_t(
                    timePoint
                );


            std::tm localTime{};


#if defined(_WIN32)

            localtime_s(
                &localTime,
                &time
            );

#else

            localtime_r(
                &time,
                &localTime
            );

#endif


            const unsigned ms =
                static_cast<unsigned>(
                    timestamp % 1000
                    );


            const int written =
                std::snprintf(
                    buffer,
                    bufferSize,

                    "%04d-%02d-%02d "
                    "%02d:%02d:%02d.%03u",

                    localTime.tm_year + 1900,
                    localTime.tm_mon + 1,
                    localTime.tm_mday,

                    localTime.tm_hour,
                    localTime.tm_min,
                    localTime.tm_sec,

                    ms
                );


            if (written <= 0) {
                return 0;
            }


            return static_cast<std::size_t>(
                written
                );
        }


        // ========================================================
        // Worker
        // ========================================================

        void workerLoop()
        {
            LogEntry entry;


            while (
                m_running.load(
                    std::memory_order_acquire
                )
                ||
                m_queue.tryPop(entry)
                ) {

                bool processed = false;


                // ------------------------------------------------
                // Drain queue in batches.
                // ------------------------------------------------

                while (
                    m_queue.tryPop(entry)
                    ) {

                    processed = true;

                    process(entry);
                }


                if (
                    !m_running.load(
                        std::memory_order_acquire
                    )
                    ) {
                    break;
                }


                // ------------------------------------------------
                // Nothing to process.
                //
                // Sleep until producer wakes us.
                // ------------------------------------------------

                if (!processed) {

                    std::unique_lock<std::mutex> lock(
                        m_waitMutex
                    );

                    m_condition.wait_for(
                        lock,
                        std::chrono::milliseconds(2)
                    );
                }
            }


            // ----------------------------------------------------
            // Final drain.
            // ----------------------------------------------------

            while (
                m_queue.tryPop(entry)
                ) {

                process(entry);
            }
        }


        // ========================================================
        // Process one log
        // ========================================================

        void process(
            const LogEntry& entry
        )
        {
            char output[
                Debug::LOG_MESSAGE_SIZE + 256
            ];


            std::size_t offset = 0;


            // ----------------------------------------------------
            // Timestamp
            // ----------------------------------------------------

            if (
                m_timestamp.load(
                    std::memory_order_relaxed
                )
                ) {

                char timestamp[32];

                const std::size_t length =
                    formatTimestamp(
                        entry.timestamp,
                        timestamp,
                        sizeof(timestamp)
                    );


                if (
                    length > 0
                    &&
                    offset + length + 3
                    < sizeof(output)
                    ) {

                    output[offset++] = '[';

                    std::memcpy(
                        output + offset,
                        timestamp,
                        length
                    );

                    offset += length;

                    output[offset++] = ']';
                    output[offset++] = ' ';
                }
            }


            // ----------------------------------------------------
            // Level
            // ----------------------------------------------------

            const char* level =
                levelString(
                    entry.level
                );


            offset += static_cast<std::size_t>(
                std::snprintf(
                    output + offset,
                    sizeof(output) - offset,
                    "[%s] ",
                    level
                )
                );


            // ----------------------------------------------------
            // Thread ID
            // ----------------------------------------------------

            if (
                m_threadId.load(
                    std::memory_order_relaxed
                )
                ) {

                offset += static_cast<std::size_t>(
                    std::snprintf(
                        output + offset,
                        sizeof(output) - offset,
                        "[T:%u] ",
                        entry.threadId
                    )
                    );
            }


            // ----------------------------------------------------
            // Source
            // ----------------------------------------------------

            if (
                m_source.load(
                    std::memory_order_relaxed
                )
                &&
                entry.file != nullptr
                ) {

                offset += static_cast<std::size_t>(
                    std::snprintf(
                        output + offset,
                        sizeof(output) - offset,
                        "[%s:%u] ",
                        entry.file,
                        entry.line
                    )
                    );
            }


            // ----------------------------------------------------
            // Message
            // ----------------------------------------------------

            const std::size_t remaining =
                sizeof(output) - offset;


            if (remaining > 2) {

                const std::size_t length =
                    entry.messageLength
                    <
                    remaining - 2
                    ? entry.messageLength
                    : remaining - 2;


                std::memcpy(
                    output + offset,
                    entry.message,
                    length
                );

                offset += length;
            }


            output[offset++] = '\n';
            output[offset] = '\0';


            // ----------------------------------------------------
            // Console
            // ----------------------------------------------------

            if (
                m_consoleOutput.load(
                    std::memory_order_relaxed
                )
                ) {

                if (
                    entry.level
                    ==
                    Debug::LogLevel::Error
                    ) {

                    std::cerr.write(
                        output,
                        static_cast<std::streamsize>(
                            offset
                            )
                    );

                }
                else {

                    std::cout.write(
                        output,
                        static_cast<std::streamsize>(
                            offset
                            )
                    );
                }
            }


            // ----------------------------------------------------
            // File
            //
            // Default OFF.
            // ----------------------------------------------------

            if (
                m_fileOutput.load(
                    std::memory_order_relaxed
                )
                &&
                m_file.is_open()
                ) {

                m_file.write(
                    output,
                    static_cast<std::streamsize>(
                        offset
                        )
                );
            }


            // ----------------------------------------------------
            // EventBus
            // ----------------------------------------------------

            if (
                m_eventBusOutput.load(
                    std::memory_order_relaxed
                )
                ) {

                EventBus::publish(
                    DebugLogEvent(
                        std::string(
                            entry.message,
                            entry.messageLength
                        ),
                        entry.level
                    )
                );
            }
        }


    private:

        // ========================================================
        // Queue
        // ========================================================

        LockFreeQueue<
            LogEntry,
            Debug::LOG_QUEUE_CAPACITY
        > m_queue;


        // ========================================================
        // State
        // ========================================================

        std::atomic<bool>
            m_running{ false };


        std::atomic<Debug::LogLevel>
            m_minimumLevel{
                Debug::LogLevel::Info
        };


        // ========================================================
        // Metadata switches
        // ========================================================

        std::atomic<bool>
            m_timestamp{ true };

        std::atomic<bool>
            m_source{ true };

        std::atomic<bool>
            m_threadId{ true };


        // ========================================================
        // Output switches
        // ========================================================

        std::atomic<bool>
            m_consoleOutput{ true };

        std::atomic<bool>
            m_fileOutput{ false };

        std::atomic<bool>
            m_eventBusOutput{ true };


        // ========================================================
        // Statistics
        // ========================================================

        std::atomic<std::uint64_t>
            m_dropped{ 0 };


        // ========================================================
        // Worker
        // ========================================================

        std::thread m_worker;


        std::condition_variable
            m_condition;

        std::mutex
            m_waitMutex;


        // ========================================================
        // File
        // ========================================================

        std::ofstream m_file;
    };


    // ============================================================
    // Global logger
    // ============================================================

    Logger& getLogger()
    {
        static Logger logger;

        return logger;
    }

} // namespace


// ============================================================
// Public API
// ============================================================

namespace Debug {


    // ============================================================
    // Lifecycle
    // ============================================================

    void start(const Config& config)
    {
        getLogger().start(config);
    }


    void stop()
    {
        getLogger().stop();
    }


    bool isRunning()
    {
        return getLogger().isRunning();
    }


    // ============================================================
    // Runtime configuration
    // ============================================================

    void setLogLevel(LogLevel level)
    {
        getLogger().setLogLevel(level);
    }


    LogLevel getLogLevel()
    {
        return getLogger().getLogLevel();
    }


    bool isEnabled(LogLevel level)
    {
        return getLogger().isEnabled(level);
    }


    void setTimestampEnabled(bool enabled)
    {
        getLogger().setTimestampEnabled(
            enabled
        );
    }


    void setSourceEnabled(bool enabled)
    {
        getLogger().setSourceEnabled(
            enabled
        );
    }


    void setThreadIdEnabled(bool enabled)
    {
        getLogger().setThreadIdEnabled(
            enabled
        );
    }


    void setConsoleOutputEnabled(bool enabled)
    {
        getLogger().setConsoleOutputEnabled(
            enabled
        );
    }


    void setFileOutputEnabled(bool enabled)
    {
        getLogger().setFileOutputEnabled(
            enabled
        );
    }


    void setEventBusOutputEnabled(bool enabled)
    {
        getLogger().setEventBusOutputEnabled(
            enabled
        );
    }


    // ============================================================
    // Basic logging
    // ============================================================

    void log(
        std::string_view message,
        std::source_location location
    )
    {
        getLogger().write(
            LogLevel::Info,
            message,
            location
        );
    }


    void logWarn(
        std::string_view message,
        std::source_location location
    )
    {
        getLogger().write(
            LogLevel::Warning,
            message,
            location
        );
    }


    void logError(
        std::string_view message,
        std::source_location location
    )
    {
        getLogger().write(
            LogLevel::Error,
            message,
            location
        );
    }


    void logDebug(
        std::string_view message,
        std::source_location location
    )
    {
        getLogger().write(
            LogLevel::Debug,
            message,
            location
        );
    }


    // ============================================================
    // Formatted logging
    // ============================================================

    void logFormat(
        LogLevel level,
        std::source_location location,
        const char* format,
        ...
    )
    {
        va_list args;

        va_start(
            args,
            format
        );


        getLogger().writeFormat(
            level,
            location,
            format,
            args
        );


        va_end(args);
    }


    // ============================================================
    // Statistics
    // ============================================================

    std::uint64_t droppedCount()
    {
        return getLogger().droppedCount();
    }

} // namespace Debug