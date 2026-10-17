#pragma  once
#include <Rubber/Core/Utility.h>
#include "Rubber/Resources/Asset.h"
#include <filesystem>
// Ordered asset handles for one animation. Import maps i.png to index i - 1,
// including missing images registered with the existing fallback texture.

namespace Rubber {
	class Texture2D;
	class SubTexture2D;
	class AssetManager;

	class AnimationAtlas final : public Asset{
	public:
		AnimationAtlas() = default;
		~AnimationAtlas() = default;

		static AssetType s_GetAssetType() {
			return AssetType::AnimationAtlas;
		}
		virtual AssetType getType() const override {
			return s_GetAssetType();
		}

		// Copies the handle into the next slot; no sorting or deduplication.
		void addFrame(AssetHandle& handler);
		// Borrows manager: it must outlive this atlas and all frame lookups.
		void setManager(AssetManager* manager) {
			m_Manager = manager;
		}

		// Requires a manager and a valid index. Returns shared texture ownership.
		const Ref<Texture2D> getTextureAtIndex(uint32_t index) const;  

	private:
		// Regression inspects exact slot count/order without widening the public API.
		friend void checkAtlasPlaceholders();
		AssetManager* m_Manager = nullptr;
		Vector<AssetHandle> m_Frames; 
	};
}