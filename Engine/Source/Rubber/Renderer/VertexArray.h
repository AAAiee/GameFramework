#pragma once
#include <memory>
#include <vector>

namespace Rubber {
	class IndexBuffer;
	class VertexBuffer;

    /**
     * Owns a GPU vertex array and retains shared ownership of attached buffers.
     * create returns shared ownership. Supply nonnull buffers; vertex buffers
     * need a layout before attachment. Keep the GL context live through use
     * and destruction. The mutable accessors borrow the stored container/Ref;
     * callers can replace that storage and must preserve these preconditions.
     */
	class VertexArray {
	public:
		virtual ~VertexArray() {};
		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;

		virtual Vector<Ref<VertexBuffer>>& getVertexBuffers() = 0;
        /**
         * Borrows the stored index-buffer Ref unchanged, including a present
         * buffer with zero elements. Requires setIndexBuffer first. Debug
         * asserts on an unset pointer; Release assertions provide no guard.
         */
		virtual Ref<IndexBuffer>& getIndexBuffer() = 0;
		static Ref<VertexArray> create();
	};
}