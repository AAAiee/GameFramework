#include <pch.h>
#include "TextureImporter.h"
#include "Rubber/Resources/AssetMetaData.h"
#include "Rubber/Resources/AnimationResources/AnimationSpriteSheet.h"
#include "Rubber/Resources/AnimationResources/AnimationAtlas.h"
#include "Rubber/Resources/AssetManager.h"
#include "Rubber/Renderer/Texture.h"

namespace fs = std::filesystem;

namespace Rubber {


	struct MissingTexture {
		Ref<Texture2D> texture;
		MissingTexture() {
			uint32_t magenta = 0xFF00FFFF;
			texture = Texture2D::create(1, 1);
			texture->setData(&magenta, 4);
		}
	};

	inline Ref<Texture2D>& getMissingTexture() {
		static MissingTexture instance;  // <-- constructed on first call, after context is ready
		return instance.texture;
	}

	Ref<Texture2D> TextureImporter::importTexture2D(AssetManager& manager, const AssetMetaData& metadata )
	{
		RB_CORE_ASSERT(metadata.type == AssetType::Texture2D, "not the correct type");

		auto pathString = metadata.filepath.generic_string();
		return Texture2D::create(pathString);
	}

	Ref<AnimationAtlas> TextureImporter::loadTAnimationAtlas(AssetManager& manager, const AssetMetaData& metadata)
	{
		AssetType type = metadata.type;
		RB_CORE_ASSERT(type == AssetType::AnimationAtlas, "Wrong type !");
		const AnimationResourcesMetaData& animationMetaData = static_cast<const AnimationResourcesMetaData&>(metadata);
		uint32_t frameCount = animationMetaData.frameCount;
		const fs::path& fileFolder = animationMetaData.filepath;

		Ref<AnimationAtlas> atlas = makeRef<AnimationAtlas>();
		atlas->setManager(&manager);

		std::string textureName, pathString;
		textureName.reserve(30);
		pathString.reserve(30);

		AssetMetaData md;

		for (uint32_t i = 1; i <= frameCount; ++i) {
			fs::path path =  fileFolder / (std::to_wstring(i) + L".png");

			textureName.clear();
			pathString.clear();
			pathString.append(path.string());
			textureName.append(path.parent_path().filename().string());
			textureName.append(path.stem().string());
			
			if (!fs::exists(path)) {
				md.filepath = path;
				md.type = AssetType::Texture2D;
				md.name = textureName;
				manager.addAsset(md.name, md, getMissingTexture());
				AssetHandle handler{ md.name };
				atlas->addFrame(handler);
				continue;
			}

			RB_INFO("Current load from: {}", pathString);
			RB_INFO("Current handleName: {}", textureName);
			md.filepath = path;
			md.type = AssetType::Texture2D;
			md.name = textureName;
			AssetHandle handler = manager.loadAsset(md);
			atlas->addFrame(handler);
		}
		return atlas;
	}
 
	Ref<AnimationSpriteSheet> TextureImporter::loadAnimationSpriteSheet(AssetManager& manager, const AssetMetaData& metadata)
	{
		RB_CORE_ASSERT(metadata.type == AssetType::AnimationSpriteSheet, "Wrong type!");

		RB_INFO("Current load from: {}", metadata.filepath.string());
		const auto& actualMetaData = static_cast<const AnimationResourcesMetaData&> (metadata);

		// loaded through plain texture2D importer
		AssetMetaData md;
		md.type = AssetType::Texture2D;
		// maybe need a better solution than this
		md.name = metadata.name + "Sheet";
		RB_INFO("Current handleName: {}", md.name );
		md.filepath = metadata.filepath;
		AssetHandle textureHandle = manager.loadAsset(md);

		// add handle to sprite sheet for management purpose
		Ref<AnimationSpriteSheet> spriteSheet = makeRef<AnimationSpriteSheet>(textureHandle, actualMetaData.frameCount, &manager);
		return spriteSheet;
	}

}