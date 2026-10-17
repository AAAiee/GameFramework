#pragma once
#include "Rubber/Renderer/Texture.h"
#include <Rubber/Core/Utility.h>

namespace Rubber{
	
	/** Target dimensions are positive pixels; current GL implementation allocates
     * one RGBA8 color and depth/stencil attachment. Samples and SwapChainTarget
     * are stored settings, not implemented multisample/swapchain modes. */
	struct FrameBufferSpecification{
		uint32_t m_Width=1280, m_Height=720; 
		uint32_t m_Samples = 1; 

		bool m_SwapChainTarget = false;
	}; 

    /** Owns GL attachments; creation, use and destruction require a current GL context.
     * bind selects this target and sets the GL viewport to its pixel dimensions.
     * unbind selects framebuffer 0; it does not restore a prior binding or viewport.
     * resize requires positive pixels and recreates attachments even for equal sizes.
     * Previously borrowed attachment IDs become invalid; reacquire them after resize.
     */
	class FrameBuffer{
	public:
		virtual ~FrameBuffer() {};
		virtual const FrameBufferSpecification& getSpecification() const = 0;
		virtual void bind() = 0;
		virtual void unbind() = 0;
		virtual const uint32_t getRendererId() const = 0;
		virtual const uint32_t getColorAttachmentID() const = 0;
		virtual const uint32_t getDepthAttachmentID() const = 0;
		virtual void resize(uint32_t width, uint32_t height) = 0;

	public:
		static Ref<FrameBuffer> create(const FrameBufferSpecification& spec); 
	};


}