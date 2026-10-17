#pragma once
#include <stdint.h>
#include <memory>

/**
* 
* @file Buffers.h
* @brief Buffers.h is a header file that contains the definition of the VertexBuffer and IndexBuffer classes.
* each class is an abstract base class that defines the interface for creating and managing vertex and index buffers.
* The static create method for each buffer are called during run time to 
* choose the specific API implementation of the buffer.
*/ 
namespace Rubber {
	class BufferLayout;
    /**
     * Owns a GPU vertex buffer; create transfers unique ownership to the caller.
     * All create/upload size arguments are bytes. Input data is borrowed only
     * during the call and copied to GL storage; uploads replace that storage.
     * setLayout copies the layout; getLayout borrows this buffer's stored copy.
     * Construction, upload, binding and destruction require a live GL context.
     */
	class VertexBuffer {
	public:
		virtual ~VertexBuffer() {} ;
		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual void setLayout(const BufferLayout& layout) = 0;
		virtual void uploadVertexData(const void* const data, uint32_t size) = 0;
		virtual const BufferLayout& getLayout() const = 0;
		static Scope<VertexBuffer> create(float* vertices, uint32_t size);
		static Scope<VertexBuffer> create(uint32_t size);
	};


    /**
     * Owns a GPU uint32_t index buffer; create transfers unique ownership.
     * Allocation and upload sizes are bytes (supply whole uint32_t elements).
     * getCount returns size / sizeof(uint32_t) index elements, suitable for an
     * explicit indexed draw count. Input data is borrowed during the call;
     * uploads replace GL storage and update the count. Keep a GL context live
     * through construction, use and destruction.
     */
	class IndexBuffer {
	public:
		virtual ~IndexBuffer() {};
		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual uint32_t getCount() const = 0;
		virtual void uploadIndexData(const uint32_t* const data, uint32_t size) = 0;
		static Scope<IndexBuffer> create(uint32_t* indices, uint32_t size);
		static Scope<IndexBuffer> create(uint32_t size);
	};

}
