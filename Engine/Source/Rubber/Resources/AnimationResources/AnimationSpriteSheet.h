#pragma once
#include <cstdint>
#include <string>
#include "Rubber/Resources/Asset.h"


namespace Rubber {
	class Texture2D;
	class SubTexture2D;
	class AssetManager;

	/** Borrows its AssetManager; keep the manager alive for all resource lookups.
	 * Returned subtextures share texture ownership. This does not own the manager.
	 */
	class AnimationSpriteSheet: public Asset {
	public:
		AnimationSpriteSheet() = default;
		~AnimationSpriteSheet() = default;

		/** Stores a texture handle and positive frameCount for one horizontal strip.
         * Borrows manager; the handle must resolve to a Texture2D during lookups. */
		AnimationSpriteSheet(AssetHandle handle,  uint32_t frameCount,  AssetManager* manager);

		static AssetType s_GetType();
	    AssetType getType() const override;

		inline void setManager(AssetManager* manager) {
			m_Manager = manager;
		}
		/** Returns a shared view retaining the sheet texture. col is a zero-based
         * frame below frameCount; use row=0 for the supported single strip.
         * oppositeDirection flips X; indices are not range-checked here. */
	    Ref<SubTexture2D> getSubTextureAt(uint32_t col, uint32_t row=0ui32,  bool oppositeDirection= false);
		
	private:
		AssetManager* m_Manager = nullptr;
		uint32_t m_FrameCount = 0;
		AssetHandle m_AssetHandle;
	};
}