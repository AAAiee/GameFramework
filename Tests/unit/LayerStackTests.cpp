// LayerStack behavior through its public push/pop API with a minimal Layer
// stub: ordinary-before-overlay ordering, kind-safe pops, and end-of-life
// ownership. Lifetime is observed through the stub's live-instance counter.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include <doctest.h>

#include "Rubber/Layer/Layer.h"
#include "Rubber/Layer/LayerStack.h"

#include <string>
#include <vector>

namespace {

    class ProbeLayer : public Rubber::Layer {
    public:
        explicit ProbeLayer(const std::string& name)
            : Rubber::Layer(name) {
            ++liveCount;
        }
        ~ProbeLayer() override { --liveCount; }

        void onAttach() override {}
        void onDetach() override {}

        static int liveCount;
    };

    int ProbeLayer::liveCount = 0;

    std::vector<std::string> stackedNames(Rubber::LayerStack& stack) {
        std::vector<std::string> names;
        for (const Rubber::Layer* layer : stack)
            names.push_back(layer->getName());
        return names;
    }

    std::vector<std::string> names(std::initializer_list<const char*> values) {
        return std::vector<std::string>(values);
    }

} // namespace

TEST_CASE("layerstack-orders-ordinary-layers-before-overlays") {
    Rubber::LayerStack stack;
    auto* first = new ProbeLayer("first");
    auto* second = new ProbeLayer("second");
    auto* third = new ProbeLayer("third");
    auto* overlayA = new ProbeLayer("overlay-a");
    auto* overlayB = new ProbeLayer("overlay-b");

    stack.pushLayer(first);
    stack.pushLayer(second);
    stack.pushOverlay(overlayA);
    stack.pushOverlay(overlayB);
    CHECK(stackedNames(stack) == names({ "first", "second", "overlay-a", "overlay-b" }));

    // An ordinary push after overlays still lands before every overlay.
    stack.pushLayer(third);
    CHECK(stackedNames(stack) == names({ "first", "second", "third", "overlay-a", "overlay-b" }));

    // The stack still owns everything pushed here; it deletes all five.
}

TEST_CASE("layerstack-pop-ignores-the-other-kind-and-absent-pointers") {
    Rubber::LayerStack stack;
    auto* ordinary = new ProbeLayer("ordinary");
    auto* late = new ProbeLayer("late");
    auto* overlay = new ProbeLayer("overlay");
    ProbeLayer neverPushed("never-pushed");

    stack.pushLayer(ordinary);
    stack.pushOverlay(overlay);

    // Wrong-kind and never-pushed pointers are no-ops.
    stack.popLayer(overlay);
    stack.popOverlay(ordinary);
    stack.popLayer(&neverPushed);
    stack.popOverlay(&neverPushed);
    CHECK(stackedNames(stack) == names({ "ordinary", "overlay" }));

    // The insert position survived those no-op pops: a new ordinary layer
    // still goes before the overlays.
    stack.pushLayer(late);
    CHECK(stackedNames(stack) == names({ "ordinary", "late", "overlay" }));

    // Pop the ordinary layers; the caller now owns them and deletes them.
    stack.popLayer(ordinary);
    stack.popLayer(late);
    CHECK(stackedNames(stack) == names({ "overlay" }));
    delete ordinary;
    delete late;

    // The stack keeps owning the overlay and deletes it at case exit.
}

TEST_CASE("layerstack-destructor-deletes-only-still-owned-layers") {
    REQUIRE(ProbeLayer::liveCount == 0);
    ProbeLayer* popped = nullptr;
    {
        Rubber::LayerStack stack;
        auto* ownedA = new ProbeLayer("owned-a");
        auto* ownedB = new ProbeLayer("owned-b");
        popped = new ProbeLayer("popped");

        stack.pushLayer(ownedA);
        stack.pushLayer(ownedB);
        stack.pushOverlay(popped);

        // Ownership of popped returns to the caller.
        stack.popOverlay(popped);
        CHECK(ProbeLayer::liveCount == 3);
    }
    // The stack deleted its two still-owned layers; the popped one survived.
    CHECK(ProbeLayer::liveCount == 1);
    delete popped;
    CHECK(ProbeLayer::liveCount == 0);
}
