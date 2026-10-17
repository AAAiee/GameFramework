#pragma once
#include "Rubber/Event/Event.h"
#include "Rubber/Debug/Profiler.h"
#include "Rubber/Core/Search.h"
#include "Rubber/Core/Assert.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <functional>

namespace Rubber {

    /** Per-event-type static subscriptions survive manager/Application destruction.
     * Names are owned copies of the exact view, unique and nonempty within T;
     * unregister requires an existing name. Callers must unsubscribe before any
     * captured objects die. Handlers borrow the event for the duration of the call.
     * Registration order is delivery order; true consumes and stops delivery.
     * Already handled events are skipped. Registering/unregistering during delivery
     * can invalidate vector iteration and is unsupported; access is unsynchronized.
     */
	template <typename T>
	class EventBus{

	public:
		using EventHandler = std::function<bool(const T& event)>;

		static void registerEventHandler(std::string_view name, EventHandler&& handler) {
			RB_PROFILE_FUNC();
			if (name.empty()) {
				RB_CORE_ASSERT(false); // can not have empty name, empty name is invalid id
			}
			// Copy the bounded view once; every operation uses this exact name.
			std::string ownedName(name);
			// check to see if the handler is already in the pool
			auto handlerItr = s_NamePool.find(ownedName);
			RB_CORE_ASSERT(handlerItr == s_NamePool.end(), "trying to register a handler that already existed");

			s_Handlers.emplace_back(std::move(handler), std::string(ownedName));
			s_NamePool.emplace(std::move(ownedName));
		}

		static void unregisterEventHandler(std::string_view name){
			RB_PROFILE_FUNC();

			std::string ownedName(name);
			// first check if the name is a existed handler name in the pool
			auto iterator = s_NamePool.find(ownedName);
			RB_CORE_ASSERT(iterator != s_NamePool.end(), "the handler's name does not exists!");

			auto vecIt = std::find_if(s_Handlers.begin(), s_Handlers.end(),
				[&](auto& h) { return h.name == ownedName; });


			s_Handlers.erase(vecIt);
			s_NamePool.erase(ownedName);
		}

		static void processAllHandlers(const T& e){
			RB_PROFILE_FUNC();
			if (e.isHandled()) return; // if already handled, do nothing

			for(auto& handlerData : s_Handlers){
				if (handlerData.handler(e)) {
					e.setHandleStatus(true);
					break;
				}
			}
		}

	private:
		struct EventHandlerData {
			EventHandler handler;
			std::string name;  // must be unique for the same event type

			EventHandlerData(EventHandler&& handler, std::string&& name)
				: handler(std::move(handler)), name(std::move(name)) {
			}

			EventHandlerData(EventHandlerData&& other) noexcept
				: handler(std::move(other.handler)), name(std::move(other.name)) {
			}

			EventHandlerData& operator=(EventHandlerData&& other) noexcept {
				if (this != &other) {
					handler = std::move(other.handler);
					name = std::move(other.name);
				}
				return *this;
			}

			EventHandlerData(const EventHandlerData& other) = default;
			EventHandlerData& operator=(const EventHandlerData& other) = default;

			bool operator ==(const EventHandlerData& other) const {
				return  name == other.name;
			}
		};

	private:
		static Vector<EventHandlerData> s_Handlers;
		static std::unordered_set<std::string>s_NamePool;
	};

	template<typename T>
	Vector<typename EventBus<T>::EventHandlerData> EventBus<T>::s_Handlers;

	template<typename T>
	std::unordered_set<std::string> EventBus<T>::s_NamePool;

}

