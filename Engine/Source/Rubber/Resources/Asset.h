#pragma once
#include <cstdint>
#include "Rubber/Core/Utility.h"
#include <string>

namespace Rubber{
	enum class AssetType : uint16_t {
		None = 0ui16,
		Texture2D,
		AnimationAtlas,
		AnimationSpriteSheet
	};

	// Name-derived asset lookup key (FNV-1a, zero remapped to one).
	// Not an entt entity identity or a lifetime/validity guarantee; hash collisions
	// and duplicate names are not resolved by this handle.
	struct AssetHandle {
		uint64_t handler = INVALID_HANDLER;
		static constexpr uint64_t INVALID_HANDLER = 0ui64;
		AssetHandle() = default;
		AssetHandle(std::string_view name);
		operator bool() const noexcept;
		bool operator==(const AssetHandle& other) const noexcept;
	};

	
	class Asset {
	public:
		// UUID FOR THE FUTURE
		AssetHandle Handle;
		virtual ~Asset() {};
		virtual AssetType getType() const = 0;
	};

}


namespace std {
	template<> struct hash<Rubber::AssetHandle> {
		size_t operator() (const Rubber::AssetHandle& assetHandler) const noexcept {
			return assetHandler.handler;
		}
	};

	//template<> struct hash<Rubber::AssetType> {
	//	size_t operator()(const Rubber::AssetType& type) const noexcept {
	//		using UT = std::underlying_type_t<Rubber::AssetType>;
	//		return std::hash<UT>{}(static_cast<UT>(type));
	//	}
	//};
}

