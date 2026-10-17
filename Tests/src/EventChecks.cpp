#include "pch.h"
#include "EventChecks.h"
#include "Rubber/Event/EventManager.h"
#include "Rubber/Event/MouseEvent.h"
#include <stdexcept>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

        // Remove static subscriptions before their captured local state dies,
        // including when a runtime oracle throws.
        template<typename T>
        class Subscription {
        public:
            Subscription(std::string_view name, typename EventBus<T>::EventHandler handler)
                : m_Name(name) {
                EventManager::create()->subscribe<T>(name, std::move(handler));
            }
            ~Subscription() { EventManager::create()->unsubscribe<T>(m_Name); }
            Subscription(const Subscription&) = delete;
            Subscription& operator=(const Subscription&) = delete;
        private:
            std::string_view m_Name;
        };
    }

    void checkTypedEvents() {
        auto manager = EventManager::create();
        MousePressedEvent pressed(2);
        MouseReleasedEvent released(2);
        require(pressed.getEventType() == EventType::MouseButtonPressed &&
            MousePressedEvent::s_GetEventType() == EventType::MouseButtonPressed,
            "Pressed event has incorrect identity");
        require(released.getEventType() == EventType::MouseButtonReleased &&
            MouseReleasedEvent::s_GetEventType() == EventType::MouseButtonReleased &&
            released.getEventType() != pressed.getEventType(),
            "Released event must have a distinct release identity");

        int calls = 0;
        const std::function<bool(const MouseReleasedEvent&)> consume = [&](const auto& e) {
            require(&e == &released && e.getButton() == 2,
                "Immediate dispatch did not borrow the supplied typed event");
            ++calls;
            return true;
        };
        require(manager->dispatchEvent<MouseReleasedEvent>(released, consume),
            "Matching immediate consuming dispatch did not match");
        require(calls == 1 && released.isHandled(),
            "Immediate consuming dispatch must call once and set handled");
        const std::function<bool(const MouseReleasedEvent&)> observe = [&](const auto&) {
            ++calls;
            return false;
        };
        require(manager->dispatchEvent<MouseReleasedEvent>(released, observe),
            "Matching nonconsuming dispatch must still report a type match");
        require(calls == 2 && !released.isHandled(),
            "Nonconsuming immediate callback must run once and clear handled");

        std::vector<int> delivered;
        Subscription<MouseReleasedEvent> queued("F09 queued", [&](const auto& e) {
            delivered.push_back(e.getButton());
            if (e.getButton() == 3) manager->enqueue(MouseReleasedEvent(5));
            return false;
        });
        {
            MouseReleasedEvent source(3);
            manager->enqueue(source);
            source.setHandleStatus(true); // Must not affect the owned queued copy.
        } // Original payload dies before delivery.
        manager->enqueue(MouseReleasedEvent(4));
        manager->dispatchAllEvents();
        require(delivered.empty(), "First dispatch must only promote new events");
        manager->dispatchAllEvents();
        require(delivered == std::vector<int>({3, 4}),
            "Queued copies were lost, reordered, or delivered followup too early");
        manager->dispatchAllEvents();
        require(delivered == std::vector<int>({3, 4, 5}),
            "Event enqueued during delivery must arrive on the following dispatch");
        manager->dispatchAllEvents();
        require(delivered.size() == 3, "Completed queue was delivered again");
    }

    void checkEventNames() {
        auto manager = EventManager::create();
        const std::string backing = "handler-extra";
        const std::string_view slice(backing.data(), 7);
        const char raw[] = {'r', 'a', 'w'}; // Intentionally has no terminator.
        const std::string_view bounded(raw, sizeof(raw));
        std::vector<int> order;
        int pressedCalls = 0;
        bool consume = false;
        Subscription<MousePressedEvent> otherType(slice, [&](const auto&) {
            ++pressedCalls;
            return false;
        });
        Subscription<MouseReleasedEvent> full(backing, [&](const auto&) {
            order.push_back(1);
            return false;
        });
        Subscription<MouseReleasedEvent> tail("unrelated", [&](const auto&) {
            order.push_back(2);
            return false;
        });
        {
            Subscription<MouseReleasedEvent> sliced(slice, [&](const auto&) {
                order.push_back(3);
                return consume;
            });
            {
                Subscription<MouseReleasedEvent> nonterminated(bounded, [&](const auto&) {
                    order.push_back(4);
                    return false;
                });
                MouseReleasedEvent event(1);
                EventBus<MouseReleasedEvent>::processAllHandlers(event);
                require(order == std::vector<int>({1, 2, 3, 4}) && !event.isHandled(),
                    "Exact sliced/nonterminated subscriptions or registration order failed");
                order.clear();
                consume = true;
                EventBus<MouseReleasedEvent>::processAllHandlers(event);
                require(order == std::vector<int>({1, 2, 3}) && event.isHandled(),
                    "Consuming handler did not stop delivery in registration order");
                order.clear();
                EventBus<MouseReleasedEvent>::processAllHandlers(event);
                require(order.empty(), "Already handled event reached subscribers");
            } // Removes with the original nonterminated view.
            manager->subscribe<MouseReleasedEvent>(bounded, [&](const auto&) {
                order.push_back(5);
                return false;
            });
            manager->unsubscribe<MouseReleasedEvent>(bounded);
        }
        // Both removed names can be reused; full suffix-bearing name and other
        // event-type subscription must survive their removal.
        {
            Subscription<MouseReleasedEvent> reused(slice, [&](const auto&) {
                order.push_back(6);
                return false;
            });
            MouseReleasedEvent event(1);
            EventBus<MouseReleasedEvent>::processAllHandlers(event);
            require(order == std::vector<int>({1, 2, 6}),
                "Removal lost the full suffix name/unrelated handler or retained an old handler");
        }
        order.clear();
        manager->enqueue(MouseReleasedEvent(7));
        manager->enqueue(MousePressedEvent(7));
        manager->dispatchAllEvents();
        require(order.empty() && pressedCalls == 0, "Named queued handlers delivered before promotion");
        manager->dispatchAllEvents();
        require(order == std::vector<int>({1, 2}) && pressedCalls == 1,
            "Removing a sliced name affected unrelated or other-type subscriptions");
    }
}
