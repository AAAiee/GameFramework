#pragma once

#include "Asset.h"
#include <filesystem>

namespace Rubber {

	struct AssetMetaData {
		std::string name;
		AssetType type = AssetType::None;
		std::filesystem::path filepath;

		AssetMetaData(std::string_view name,AssetType type, std::string_view filepath)
			:name(name), type(type), filepath(filepath) { }

		AssetMetaData() = default;
		virtual  ~AssetMetaData() = default;

		virtual operator bool() const { return type != AssetType::None; }
	};
	
	/** Animation import description: filepath is a numbered-image directory for
     * an atlas, or an image file for a horizontal sprite sheet; frameCount is
     * the declared number of frames, including missing-image placeholder slots. */
	struct AnimationResourcesMetaData : public AssetMetaData {
		AnimationResourcesMetaData() = default;

		//filepath now is a bit misleading because for a spriteSheet type, it is the actual filePath, but for a 
		//Atlas, it is actually the Folder address to that Atlas
		AnimationResourcesMetaData(std::string_view name, AssetType type, std::string_view path, uint32_t frameCount)
			:AssetMetaData(name, type, path), frameCount(frameCount)
		{
		}

		uint32_t frameCount = 0;
	};

}