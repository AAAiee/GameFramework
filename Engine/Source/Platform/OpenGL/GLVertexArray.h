#pragma once
#include "Rubber/Renderer/VertexArray.h"
#include "Rubber/Core/Utility.h"

namespace Rubber {
	class GLVertexArray : public VertexArray{
	public:
		
		GLVertexArray();
		virtual ~GLVertexArray() ;
		virtual void bind() const final override;
		virtual void unbind() const final override ;

		virtual void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) final override ;
		virtual void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) final override;

		virtual Vector<Ref<VertexBuffer>>& getVertexBuffers() final override ;
		virtual Ref<IndexBuffer>& getIndexBuffer() final override ;

	private:
		uint32_t m_RendererID;
		Vector<Ref<VertexBuffer>> m_VertexBuffers;
		Ref<IndexBuffer> m_IndexBuffer;
	};
}