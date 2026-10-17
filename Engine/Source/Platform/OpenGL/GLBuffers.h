#pragma once
#include "Rubber/Renderer/Buffers.h"
#include "Rubber/Renderer/BufferLayout.h"


namespace Rubber {

	/**
	* @file Buffers.h
	* @brief the OPEN_GL implementation of the vertex buffer
	*/
	class GLVertexBuffer : public VertexBuffer {
	public:
		GLVertexBuffer(float* vertices, uint32_t size);
		GLVertexBuffer(uint32_t size);
		virtual void bind() const final override;
		virtual void unbind() const final override;
		virtual void setLayout(const BufferLayout& layout) final override;
		virtual void uploadVertexData(const void* const data, uint32_t size) final override;
		virtual inline const BufferLayout& getLayout() const final override;
		~GLVertexBuffer();

	private:
		uint32_t m_RendererID;
		BufferLayout m_layout = BufferLayout();
	};



	class GLIndexBuffer : public IndexBuffer {
	public:
		GLIndexBuffer(uint32_t* indices, uint32_t size);
		GLIndexBuffer(uint32_t size);
		virtual void bind() const final override;
		virtual void unbind() const final override;
		virtual void uploadIndexData(const uint32_t* const data, uint32_t size) final override;
		virtual inline uint32_t getCount() const final override { return this->m_Count;};
		~GLIndexBuffer();

	private:
		uint32_t m_RendererID;
		uint32_t m_Count;
	};

}
