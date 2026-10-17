#include "pch.h"
#include "LayerPartitionCheck.h"
#include "Rubber/Layer/Layer.h"
#include "Rubber/Layer/LayerStack.h"
#include <stdexcept>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

        class ProbeLayer final : public Layer {
        public:
            explicit ProbeLayer(int& destructions) : m_Destructions(destructions) {}
            ~ProbeLayer() override { ++m_Destructions; }
            void onAttach() override {}
            void onDetach() override {}
        private:
            int& m_Destructions;
        };

        void requireOrder(LayerStack& stack, const std::vector<Layer*>& ordinary,
            const std::vector<Layer*>& overlays) {
            auto expected = ordinary;
            expected.insert(expected.end(), overlays.begin(), overlays.end());
            require(std::vector<Layer*>(stack.begin(), stack.end()) == expected,
                "LayerStack contents or ordinary/overlay order changed unexpectedly");
        }

        void checkAbsent(int ordinaryCount, int overlayCount) {
            int destructions = 0;
            int missingDestructions = 0;
            ProbeLayer missing(missingDestructions);
            int allocations = 0;
            {
                LayerStack stack;
                std::vector<Layer*> ordinary;
                std::vector<Layer*> overlays;
                auto insertOrdinary = [&] {
                    auto* layer = new ProbeLayer(destructions);
                    ++allocations;
                    stack.pushLayer(layer);
                    ordinary.push_back(layer);
                    requireOrder(stack, ordinary, overlays);
                };
                for (int i = 0; i < ordinaryCount; ++i) insertOrdinary();
                for (int i = 0; i < overlayCount; ++i) {
                    auto* overlay = new ProbeLayer(destructions);
                    ++allocations;
                    stack.pushOverlay(overlay);
                    overlays.push_back(overlay);
                }
                requireOrder(stack, ordinary, overlays);

                // Check contents before pushing so a corrupted empty partition
                // fails here rather than using an invalid insertion iterator.
                for (auto* overlay : overlays) {
                    stack.popLayer(overlay);
                    requireOrder(stack, ordinary, overlays);
                }
                stack.popLayer(&missing);
                requireOrder(stack, ordinary, overlays);
                insertOrdinary();
                stack.popLayer(&missing);
                requireOrder(stack, ordinary, overlays);
                insertOrdinary();

                for (auto* overlay : overlays) {
                    stack.popLayer(overlay);
                    requireOrder(stack, ordinary, overlays);
                    insertOrdinary();
                }
                require(destructions == 0 && missingDestructions == 0,
                    "An absent pop or insertion destroyed a layer");
            }
            require(destructions == allocations,
                "Stack destruction did not delete every retained layer exactly once");
        }

        void checkPresent(int ordinaryCount, int overlayCount, int removedIndex) {
            int destructions = 0;
            std::unique_ptr<Layer> popped;
            {
                LayerStack stack;
                std::vector<Layer*> ordinary;
                std::vector<Layer*> overlays;
                for (int i = 0; i < ordinaryCount; ++i) {
                    auto* layer = new ProbeLayer(destructions);
                    stack.pushLayer(layer);
                    ordinary.push_back(layer);
                }
                for (int i = 0; i < overlayCount; ++i) {
                    auto* overlay = new ProbeLayer(destructions);
                    stack.pushOverlay(overlay);
                    overlays.push_back(overlay);
                }

                auto* removed = ordinary[removedIndex];
                stack.popLayer(removed);
                require(destructions == 0, "Popping an ordinary layer deleted it");
                // The caller now owns the popped object, even after stack teardown.
                popped.reset(removed);
                ordinary.erase(ordinary.begin() + removedIndex);
                requireOrder(stack, ordinary, overlays);
                stack.popLayer(removed);
                requireOrder(stack, ordinary, overlays);

                auto* replacement = new ProbeLayer(destructions);
                stack.pushLayer(replacement);
                ordinary.push_back(replacement);
                requireOrder(stack, ordinary, overlays);
                require(destructions == 0, "Removal or reinsertion destroyed a layer");
            }
            require(destructions == ordinaryCount + overlayCount,
                "Stack destruction did not delete exactly the retained layers");
            popped.reset();
            require(destructions == ordinaryCount + 1 + overlayCount,
                "Stack retained ownership of a popped layer or lost a remaining layer");
        }
    }

    void checkLayerPartition() {
        for (int overlayCount : { 0, 1, 3 }) {
            // Includes empty and overlay-only stacks as well as mixed partitions.
            for (int ordinaryCount : { 0, 1, 3 })
                checkAbsent(ordinaryCount, overlayCount);
            // Remove a sole layer, or the first, middle and last independently.
            for (int ordinaryCount : { 1, 3 })
                for (int removedIndex = 0; removedIndex < ordinaryCount; ++removedIndex)
                    checkPresent(ordinaryCount, overlayCount, removedIndex);
        }
    }
}
