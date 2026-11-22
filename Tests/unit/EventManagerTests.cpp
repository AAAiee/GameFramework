// EventManager behavior through the public subscribe/enqueue/dispatch API.
// EventBus keeps per-type static storage that outlives any manager instance,
// so every case uses unique subscriber names and unsubscribes before exit.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include <doctest.h>

#include "Rubber/Event/AppEvent.h"
#include "Rubber/Event/KeyEvent.h"
#include "Rubber/Event/EventManager.h"

#include <vector>

TEST_CASE("eventmanager-immediate-typed-dispatch") {
    auto eventManager = Rubber::EventManager::create();

    Rubber::KeyPressedEvent pressed(65, 3);
    CHECK(!pressed.isHandled());

    // The immediate path invokes only the supplied callback; no subscriber is
    // involved. It reports the type match and takes the callback's verdict.
    int seenKeyCode = 0;
    int seenRepeatCount = 0;
    bool matched = eventManager->dispatchEvent(pressed, [&](const Rubber::KeyPressedEvent& e) {
        seenKeyCode = e.getKeyCode();
        seenRepeatCount = e.getRepeatCount();
        return true;
    });
    CHECK(matched);
    CHECK(pressed.isHandled());
    CHECK(seenKeyCode == 65);
    CHECK(seenRepeatCount == 3);

    // A returning-false callback replaces even an already-set handled status.
    pressed.setHandleStatus(false);
    matched = eventManager->dispatchEvent(pressed, [](const Rubber::KeyPressedEvent& e) {
        return false;
    });
    CHECK(matched);
    CHECK(!pressed.isHandled());
}

TEST_CASE("eventmanager-queued-delivery-promotes-then-delivers-in-order") {
    auto eventManager = Rubber::EventManager::create();

    std::vector<int> deliveryOrder;
    eventManager->subscribe<Rubber::AppTickEvent>("unit-queued-first",
        [&](const Rubber::AppTickEvent& e) {
            deliveryOrder.push_back(1);
            return false;
        });
    eventManager->subscribe<Rubber::AppTickEvent>("unit-queued-second",
        [&](const Rubber::AppTickEvent& e) {
            deliveryOrder.push_back(2);
            return false;
        });

    eventManager->enqueue(Rubber::AppTickEvent{});

    // One call only promotes the queued event; delivery happens on the next.
    eventManager->dispatchAllEvents();
    CHECK(deliveryOrder.empty());

    eventManager->dispatchAllEvents();
    REQUIRE(deliveryOrder.size() == 2);
    CHECK(deliveryOrder[0] == 1); // registration order is delivery order
    CHECK(deliveryOrder[1] == 2);

    eventManager->unsubscribe<Rubber::AppTickEvent>("unit-queued-first");
    eventManager->unsubscribe<Rubber::AppTickEvent>("unit-queued-second");
}

TEST_CASE("eventmanager-consumption-stops-later-handlers") {
    auto eventManager = Rubber::EventManager::create();

    std::vector<int> calls;
    eventManager->subscribe<Rubber::AppRenderEvent>("unit-consume-first",
        [&](const Rubber::AppRenderEvent& e) {
            calls.push_back(1);
            return true; // consumed
        });
    eventManager->subscribe<Rubber::AppRenderEvent>("unit-consume-second",
        [&](const Rubber::AppRenderEvent& e) {
            calls.push_back(2);
            return false;
        });

    eventManager->enqueue(Rubber::AppRenderEvent{});
    eventManager->dispatchAllEvents();
    eventManager->dispatchAllEvents();
    REQUIRE(calls.size() == 1);
    CHECK(calls[0] == 1);

    eventManager->unsubscribe<Rubber::AppRenderEvent>("unit-consume-first");
    eventManager->unsubscribe<Rubber::AppRenderEvent>("unit-consume-second");
}

TEST_CASE("eventmanager-followup-enqueued-during-delivery-arrives-next-call") {
    auto eventManager = Rubber::EventManager::create();

    std::vector<int> deliveries;
    int followUpsEnqueued = 0;
    eventManager->subscribe<Rubber::AppUpdateEvent>("unit-followup",
        [&](const Rubber::AppUpdateEvent& e) {
            deliveries.push_back(1);
            if (followUpsEnqueued == 0) {
                ++followUpsEnqueued;
                eventManager->enqueue(Rubber::AppUpdateEvent{});
            }
            return false;
        });

    eventManager->enqueue(Rubber::AppUpdateEvent{});
    eventManager->dispatchAllEvents(); // promote
    eventManager->dispatchAllEvents(); // deliver; the handler enqueues a follow-up
    REQUIRE(deliveries.size() == 1);

    // The follow-up arrives on the next call, not during the current one.
    eventManager->dispatchAllEvents();
    CHECK(deliveries.size() == 2);

    eventManager->unsubscribe<Rubber::AppUpdateEvent>("unit-followup");
}

TEST_CASE("eventmanager-unsubscribe-stops-delivery-and-payload-survives-queueing") {
    auto eventManager = Rubber::EventManager::create();

    int deliveries = 0;
    int seenWidth = 0;
    int seenHeight = 0;
    eventManager->subscribe<Rubber::WindowResizeEvent>("unit-unsub",
        [&](const Rubber::WindowResizeEvent& e) {
            ++deliveries;
            seenWidth = static_cast<int>(e.getWidth());
            seenHeight = static_cast<int>(e.getHeight());
            return false;
        });

    // Enqueue owns a copy; the delivered payload matches what was queued.
    eventManager->enqueue(Rubber::WindowResizeEvent(640, 480));
    eventManager->dispatchAllEvents();
    eventManager->dispatchAllEvents();
    CHECK(deliveries == 1);
    CHECK(seenWidth == 640);
    CHECK(seenHeight == 480);

    // With the subscription gone, the same queueing delivers to no one.
    eventManager->unsubscribe<Rubber::WindowResizeEvent>("unit-unsub");
    eventManager->enqueue(Rubber::WindowResizeEvent(800, 600));
    eventManager->dispatchAllEvents();
    eventManager->dispatchAllEvents();
    CHECK(deliveries == 1);
}
