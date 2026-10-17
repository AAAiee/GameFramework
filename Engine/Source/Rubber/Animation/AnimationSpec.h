#pragma once
#include "Rubber/Animation/AnimationTimer.h"
#include "Rubber/Core/Utility.h"
#include "Rubber/Resources/Asset.h"
#include <glm/glm.hpp>

namespace Rubber {
	struct AnimationClipConfig;

	/** Mutable playback state for the dormant animation system; not installed by Scene.
     * Configured construction/setSpec requires Application asset metadata for an
     * animation atlas or sheet. Frame indices are zero-based; resources stay in
     * the manager. Completion callbacks execute synchronously during updates;
     * do not destroy/move components or mutate iterated registry storage there.
     * Captured component references and repeated nonloop completion remain unsafe. */
	struct AnimationSpec {

		enum class AnchorMode: uint8_t {
			CENTERED,
			BOTTOMCENTERED
		};

		AnimationSpec() = default;
		AnimationSpec(const AnimationClipConfig& config);

		void setSpec(const AnimationClipConfig& config);

		/** Legacy name: assigns m_IsLoop=flag, rather than inverting it. */
		void setRunOnce(bool flag) {
			m_IsLoop = flag;
		}

		void setFrameCount(uint32_t frameCount) {
			m_FrameCount = frameCount;
		}

		void oppositeDirection(bool flag) {
			m_IsOpposizeDireciton = flag;
		}

		void setAnchorMode(AnchorMode mode) {
			m_AnchorMode = mode;
		}

		void setOnfinished(std::function<void()> function) {
			m_OnFinished = std::move(function);
		}

		AnimationTimer m_Timer;
		bool m_IsLoop = true;
		uint32_t m_FrameIndex = 0;
		AssetHandle m_ActiveAnimationClip;
		uint32_t m_FrameCount = 0;
		bool m_IsOpposizeDireciton = false;
		AnchorMode m_AnchorMode = AnchorMode::CENTERED;
		std::function<void()> m_OnFinished;
	};

	/** Copies a clip handle, callback and playback settings; frameDuration is seconds
     * per frame and must be positive. This configuration owns no asset manager. */
	struct AnimationClipConfig {
		std::function<void()> onFinished = nullptr;
		AssetHandle clipHandle;
		float frameDuration = 1.0f;
		bool loop = true;
		bool isFlipped = false;
		AnimationSpec::AnchorMode anchorMode = AnimationSpec::AnchorMode::CENTERED;
		AnimationClipConfig() = default;
	};
}