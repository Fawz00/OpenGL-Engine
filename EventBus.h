#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <unordered_map>
#include <any>
#include <typeindex>
#include <cstdint>
#include <atomic>
#include <algorithm>
#include "Debug.h"

/**
 * @brief Namespace untuk sistem EventBus global.
 *
 * EventBus adalah sistem publish-subscribe sederhana untuk mengelola event listener
 * dan mengirimkan event secara global di seluruh aplikasi.
 *
 * - subscribe: Mendaftarkan sebuah listener untuk tipe event tertentu.
 * - unsubscribe: Membatalkan pendaftaran listener.
 * - publish: Menerbitkan event ke semua listener yang terdaftar untuk tipenya.
 * - clear: Menghapus semua listener yang terdaftar.
 */
namespace EventBus {

    // Tipe data untuk listener. Menggunakan std::function agar bisa menerima
    // fungsi bebas, lambda, atau member function.
    template<typename T>
    using EventListener = std::function<void(const T&)>;

    // Tipe data untuk ID unik setiap listener, digunakan untuk unsubscribe.
    using ListenerId = uint64_t;

    namespace detail {
        // Struct internal untuk menyimpan semua data EventBus.
        // Dibuat sebagai static lokal di dalam fungsi untuk memastikan inisialisasi tunggal (Singleton).
        struct EventBusData {
            // Peta dari tipe event ke daftar listener-nya.
            // std::type_index: Kunci unik untuk setiap tipe data (e.g., DamageEvent).
            // std::any: Wadah yang bisa menampung tipe data apa pun. Di sini kita akan menyimpan
            //           std::vector dari listener yang spesifik untuk tipe event tersebut.
            std::unordered_map<std::type_index, std::any> listeners;

            // Penghitung atomik untuk menghasilkan ID listener yang unik.
            std::atomic<ListenerId> nextListenerId{1};
        };

        // Fungsi untuk mendapatkan instance tunggal dari data EventBus.
        inline EventBusData& getData() {
            static EventBusData data;
            return data;
        }
    } // namespace detail

    /**
     * @brief Mendaftarkan listener untuk sebuah tipe event.
     * @tparam T Tipe event yang ingin didengarkan.
     * @param listener Fungsi (atau lambda) yang akan dipanggil saat event dipublikasikan.
     * @return ID unik untuk listener ini, yang dapat digunakan untuk unsubscribe.
     */
    template<typename T>
    ListenerId subscribe(EventListener<T> listener) {
        auto& data = detail::getData();
        const ListenerId id = data.nextListenerId++;
        const auto typeIndex = std::type_index(typeid(T));

        // Tipe spesifik dari daftar listener untuk event T
        using ListenerList = std::vector<std::pair<ListenerId, EventListener<T>>>;

        // Ambil atau buat wadah 'any' untuk tipe event ini
        auto& listenersAny = data.listeners[typeIndex];

        // Jika wadah ini baru dibuat (kosong), inisialisasi dengan daftar listener yang benar
        if (!listenersAny.has_value()) {
            listenersAny.emplace<ListenerList>();
        }

        // Lakukan cast 'any' ke tipe daftar listener yang benar dan tambahkan listener baru
        if (auto* listenerList = std::any_cast<ListenerList>(&listenersAny)) {
            listenerList->emplace_back(id, listener);
        }

        return id;
    }

    /**
     * @brief Membatalkan pendaftaran listener menggunakan ID-nya.
     * @tparam T Tipe event yang listener-nya ingin dihapus.
     * @param listenerId ID yang dikembalikan oleh fungsi subscribe.
     */
    template<typename T>
    void unsubscribe(ListenerId listenerId) {
        auto& data = detail::getData();
        const auto typeIndex = std::type_index(typeid(T));

        auto it = data.listeners.find(typeIndex);
        if (it == data.listeners.end()) {
			Debug::logWarn("EventBus::unsubscribe: No listeners found for the given event type.");
            return; // Tidak ada listener untuk tipe event ini
        }

        try {
            using ListenerList = std::vector<std::pair<ListenerId, EventListener<T>>>;
            if (auto* listenerList = std::any_cast<ListenerList>(&it->second)) {
                // Gunakan erase-remove idiom untuk menghapus listener berdasarkan ID
                auto newEnd = std::remove_if(listenerList->begin(), listenerList->end(),
                    [listenerId](const auto& pair) {
                        return pair.first == listenerId;
                    });
                listenerList->erase(newEnd, listenerList->end());
            }
        }
        catch (const std::bad_any_cast& e) {
            Debug::logError(std::string("EventBus::unsubscribe: Bad any_cast for event type: ") + e.what());
		}
    }

    /**
     * @brief Menerbitkan event ke semua listener yang terdaftar untuk tipe event tersebut.
     * @tparam T Tipe event. Tipe ini akan disimpulkan secara otomatis dari argumen.
     * @param event Objek event yang akan dikirim.
     */
    template<typename T>
    void publish(const T& event) {
        auto& data = detail::getData();
        // Dapatkan tipe dari objek event yang sebenarnya
        const auto typeIndex = std::type_index(typeid(T));

        auto it = data.listeners.find(typeIndex);
        if (it == data.listeners.end()) {
            return; // Tidak ada listener, tidak perlu melakukan apa-apa
        }

        try {
            using ListenerList = std::vector<std::pair<ListenerId, EventListener<T>>>;
            if (auto* listenerList = std::any_cast<ListenerList>(&it->second)) {
                // Panggil setiap listener yang terdaftar
                // Buat salinan untuk menghindari masalah jika listener mencoba unsubscribe di dalam callback
                auto listenersCopy = *listenerList;
                for (const auto& pair : listenersCopy) {
                    pair.second(event); // Panggil fungsi onEvent
                }
            }
        }
        catch (const std::bad_any_cast& e) {
            Debug::logError(std::string("EventBus::publish: Bad any_cast for event type: ") + e.what());
        }
    }

    /**
     * @brief Menghapus semua listener dari semua tipe event.
     */
    inline void clear() {
        detail::getData().listeners.clear();
    }

} // namespace EventBus