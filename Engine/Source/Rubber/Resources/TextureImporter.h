#pragma once  
#include "Rubber/Core/Utility.h"  

namespace Rubber {  

	class AnimationSpriteSheet;
	struct AssetMetaData;
	class AssetManager;
	class Texture2D; 
	class AnimationAtlas;

	class TextureImporter {  
	public:

		static Ref<Texture2D> importTexture2D(AssetManager& manager, const AssetMetaData& metadata);  

		// Imports 1.png..frameCount.png in order, retaining missing-image slots.
		// Requires a live GL context. The result borrows manager. Missing slots share
		// a process-lived fallback: its destruction after GL teardown remains a defect.
		static Ref<AnimationAtlas> loadTAnimationAtlas(AssetManager& manager,const AssetMetaData& metadata);  

		static Ref<AnimationSpriteSheet> loadAnimationSpriteSheet(AssetManager& manager,const AssetMetaData& metadata);  
		
		static Ref<Texture2D> s_MissingTexture;
	};  
}