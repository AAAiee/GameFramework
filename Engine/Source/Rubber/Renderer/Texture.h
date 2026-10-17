#pragma  once
#include <cstdint>
#include <string>
#include <Rubber/Core/Core.h>
#include "Rubber/Resources/Asset.h"
namespace Rubber{

	// GPU texture ownership is shared by Ref<Texture2D>; the final owner deletes
	// the GL object. Creation, binding, upload and final release require a live,
	// compatible graphics context current on the calling thread. IDs are borrowed
	// GL handles valid only while the texture and its context remain alive.
	class Texture: public Asset {
	public:
		virtual ~Texture() {};
		virtual void bind(uint16_t slot) const = 0;
		virtual void unBind() const = 0;
		virtual uint32_t getTextureID() const = 0;
		virtual int32_t getWidth() const = 0;
		virtual int32_t getHeight() const = 0;
	};


	class Texture2D: public Texture {
	public: 
		virtual ~Texture2D() {};
		// Replace the full level-zero image synchronously; data is borrowed for this
		// call. Supply width * height * channels bytes of tightly packed unsigned
		// RGB/RGBA pixels (3/4 bytes per pixel, matching the loaded image). Size-only
		// textures use RGBA. First row maps to v=0; no vertical flip is applied here.
		// GL_UNPACK_ALIGNMENT is preserved. Other unpack state must describe client
		// memory with no row length/skips and no pixel unpack buffer bound.
		virtual void setData(void* data, uint32_t size) = 0;
		// Path is a bounded slice copied during construction; no trailing terminator
		// is required. RGB/RGBA files are flipped vertically so v=0 is the bottom
		// image row. Decoded CPU pixels are freed after upload. Requires the same
		// unpack-state preconditions as setData; preserves unpack alignment.
		static Ref<Texture2D> create(std::string_view texturePath);
		// Allocate RGBA storage; initialize its pixels with setData before sampling.
		static Ref<Texture2D> create(uint16_t width, uint16_t heigh);
		virtual bool operator==(const Texture& texture)  = 0;
		static AssetType getStaticType() {
			return AssetType::Texture2D;
		}
		virtual AssetType getType() const override {
			return  getStaticType();
		}
	}; 



}