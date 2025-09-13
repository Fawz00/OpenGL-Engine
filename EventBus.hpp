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

#include "Debug.hpp"

namespace EventBus {

    template<typename T>
    using EventListener = std::function<void(const T&)>;

    using ListenerId = uint64_t;

    namespace detail {
        struct EventBusData {
            std::unordered_map<std::type_index, std::any> listeners;
            std::atomic<ListenerId> nextListenerId{1};
        };

        inline EventBusData& getData() {
            static EventBusData data;
            return data;
        }
    } // namespace detail

    template<typename T>
    ListenerId subscribe(EventListener<T> listener) {
        auto& data = detail::getData();
        const ListenerId id = data.nextListenerId++;
        const auto typeIndex = std::type_index(typeid(T));

        using ListenerList = std::vector<std::pair<ListenerId, EventListener<T>>>;

        auto& listenersAny = data.listeners[typeIndex];

        if (!listenersAny.has_value()) {
            listenersAny.emplace<ListenerList>();
        }

        if (auto* listenerList = std::any_cast<ListenerList>(&listenersAny)) {
            listenerList->emplace_back(id, listener);
        }

        return id;
    }

    template<typename T>
    void unsubscribe(ListenerId listenerId) {
        auto& data = detail::getData();
        const auto typeIndex = std::type_index(typeid(T));

        auto it = data.listeners.find(typeIndex);
        if (it == data.listeners.end()) {
			Debug::logWarn("EventBus::unsubscribe: No listeners found for the given event type.");
            return;
        }

        try {
            using ListenerList = std::vector<std::pair<ListenerId, EventListener<T>>>;
            if (auto* listenerList = std::any_cast<ListenerList>(&it->second)) {
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

    template<typename T>
    void publish(const T& event) {
        auto& data = detail::getData();
        const auto typeIndex = std::type_index(typeid(T));

        auto it = data.listeners.find(typeIndex);
        if (it == data.listeners.end()) {
            return;
        }

        try {
            using ListenerList = std::vector<std::pair<ListenerId, EventListener<T>>>;
            if (auto* listenerList = std::any_cast<ListenerList>(&it->second)) {
                auto listenersCopy = *listenerList;
                for (const auto& pair : listenersCopy) {
                    pair.second(event);
                }
            }
        }
        catch (const std::bad_any_cast& e) {
            Debug::logError(std::string("EventBus::publish: Bad any_cast for event type: ") + e.what());
        }
    }

    inline void clear() {
        detail::getData().listeners.clear();
    }

} // namespace EventBus