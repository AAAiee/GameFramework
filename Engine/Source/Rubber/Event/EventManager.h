#pragma once
#include "Rubber/Event/EventBus.h"


namespace Rubber{

    // Shared singleton; queues can outlive an Application. enqueue owns a copy
    // (including handled status); callbacks borrow that copy only during delivery.
    // dispatchAllEvents delivers the current queue in insertion order, then
    // promotes the next queue. Newly enqueued events need promotion then delivery
    // on two calls; events enqueued during delivery arrive on the following call.
    // Do not recursively dispatchAllEvents from a callback. Subscription mutation
    // during delivery is unsafe; see EventBus. No thread synchronization is provided.
	class EventManager{  
	private:
		template<typename EventType>
		using EventHandlerFunc = std::function<bool(const EventType& e)>;

	public: 
		template<typename EventType>
		void subscribe(std::string_view name, EventBus<EventType>::EventHandler&& handler);

		template<typename EventType>
		void unsubscribe(std::string_view name);


		template<typename EventType>
		void enqueue(const EventType& e);

		void dispatchAllEvents(); // in queued order

        // Borrows e and invokes only func synchronously on a type match; no queue
        // or named subscribers participate. Returns whether the type matched,
        // independently of consumption. Replaces handled status with func's result,
        // even if e was already handled. The callback must not retain e's reference.
		template<typename EventType>
		bool dispatchEvent(EventType& e, const EventHandlerFunc<EventType>& func); // immediately

		static Ref<EventManager> create();

		~EventManager() = default;

	private:
		EventManager() = default ;
		Vector<std::function<void()>> m_CurrentQueue;
		Vector<std::function<void()>> m_NextQueue;
	};

	template<typename EventType>
	bool Rubber::EventManager::dispatchEvent(EventType& e, const EventHandlerFunc<EventType>& func)
	{
		if (EventType::s_GetEventType() == e.getEventType()) {
			e.setHandleStatus(func(e));
			return true;
		}
		return false;
	}

	template<typename EventType>
	void Rubber::EventManager::subscribe(std::string_view name, EventBus<EventType>::EventHandler&& handler)
	{
		EventBus<EventType>::registerEventHandler(name, std::move(handler));
	}

	template<typename EventType>
	void Rubber::EventManager::unsubscribe(std::string_view name)
	{
		EventBus<EventType>::unregisterEventHandler(name);
	}

	template<typename EventType>
	void Rubber::EventManager::enqueue(const EventType& e)
	{
		RB_PROFILE_FUNC();
		auto fn = [ev = e]() mutable {  // pay a copy, for now all events ar small so no smart pointer involved
			EventBus<EventType>::processAllHandlers(ev);
		};
		m_NextQueue.emplace_back(std::move(fn));
	}
}

