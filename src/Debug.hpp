#pragma once

#include <cstddef>
#include <cstdint>
#include <source_location>
#include <string_view>

namespace Debug {

    // ============================================================
    // Constants
    // ============================================================

    constexpr std::size_t LOG_QUEUE_CAPACITY = 4096;
    constexpr std::size_t LOG_MESSAGE_SIZE = 512;


    // ============================================================
    // Log Level
    // ============================================================

    enum class LogLevel : std::uint8_t {
        Info = 0,
        Warning,
        Error,
        Debug
    };


    // ============================================================
    // Configuration
    // ============================================================

    struct Config {

        // --------------------------------------------------------
        // Metadata
        // --------------------------------------------------------

        bool timestamp = true;
        bool source = true;
        bool threadId = true;

        // --------------------------------------------------------
        // Output
        // --------------------------------------------------------

        bool consoleOutput = true;
        bool fileOutput = false;
        bool eventBusOutput = true;

        // --------------------------------------------------------
        // File
        // --------------------------------------------------------

        const char* filePath = "engine.log";

        // --------------------------------------------------------
        // Runtime filtering
        // --------------------------------------------------------

        LogLevel minimumLevel = LogLevel::Info;
    };


    // ============================================================
    // Lifecycle
    // ============================================================

    void start(const Config& config = {});
    void stop();

    bool isRunning();


    // ============================================================
    // Runtime configuration
    // ============================================================

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel();

    bool isEnabled(LogLevel level);

    void setTimestampEnabled(bool enabled);
    void setSourceEnabled(bool enabled);
    void setThreadIdEnabled(bool enabled);

    void setConsoleOutputEnabled(bool enabled);
    void setFileOutputEnabled(bool enabled);
    void setEventBusOutputEnabled(bool enabled);


    // ============================================================
    // Basic logging
    // ============================================================

    void log(
        std::string_view message,
        std::source_location location =
        std::source_location::current()
    );

    void logWarn(
        std::string_view message,
        std::source_location location =
        std::source_location::current()
    );

    void logError(
        std::string_view message,
        std::source_location location =
        std::source_location::current()
    );

    void logDebug(
        std::string_view message,
        std::source_location location =
        std::source_location::current()
    );


    // ============================================================
    // Formatted logging
    //
    // Internal API for macros.
    // Do not call these directly unless necessary.
    // ============================================================

    void logFormat(
        LogLevel level,
        std::source_location location,
        const char* format,
        ...
    );


    // ============================================================
    // Statistics
    // ============================================================

    std::uint64_t droppedCount();

} // namespace Debug


// ============================================================
// Macros
// ============================================================

#define LOG_INFO(...)                                                   \
    ::Debug::logFormat(                                                \
        ::Debug::LogLevel::Info,                                       \
        std::source_location::current(),                               \
        __VA_ARGS__                                                     \
    )


#define LOG_WARN(...)                                                   \
    ::Debug::logFormat(                                                \
        ::Debug::LogLevel::Warning,                                    \
        std::source_location::current(),                               \
        __VA_ARGS__                                                     \
    )


#define LOG_ERROR(...)                                                  \
    ::Debug::logFormat(                                                \
        ::Debug::LogLevel::Error,                                      \
        std::source_location::current(),                               \
        __VA_ARGS__                                                     \
    )


#ifdef ENGINE_DISABLE_DEBUG_LOGGING

#define LOG_DEBUG(...) ((void)0)

#else

#define LOG_DEBUG(...)                                              \
        ::Debug::logFormat(                                            \
            ::Debug::LogLevel::Debug,                                  \
            std::source_location::current(),                            \
            __VA_ARGS__                                                  \
        )

#endif