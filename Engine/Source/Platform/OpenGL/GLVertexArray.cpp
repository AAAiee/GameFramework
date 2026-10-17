#include "pch.h"
#include "GLVertexArray.h"
#include "Rubber/Renderer/BufferLayout.h"
#include "Rubber/Renderer/Buffers.h"
#include <glad/glad.h>

namespace Rubber {
    // removed eventually, quick hack
    static GLenum ShaderTypeToOpenGLType(ShaderType type) {
        switch (type)
        {
        case ShaderType::Float:   return GL_FLOAT;
        case ShaderType::Float2:  return GL_FLOAT;
        case ShaderType::Float3:  return GL_FLOAT;
        case ShaderType::Float4:  return GL_FLOAT;
        case ShaderType::Mat2:    return GL_FLOAT;
        case ShaderType::Mat3:    return GL_FLOAT;
        case ShaderType::Mat4:    return GL_FLOAT;
        case ShaderType::Int:     return GL_INT;
        case ShaderType::Int2:    return GL_INT;
        case ShaderType::Int3:    return GL_INT;
        case ShaderType::Int4:    return GL_INT;
        case ShaderType::Uint:	  return GL_UNSIGNED_INT;
        case ShaderType::Uint2:	  return GL_UNSIGNED_INT;
        case ShaderType::Uint3:	  return GL_UNSIGNED_INT;
        case ShaderType::Uint4:	  return GL_UNSIGNED_INT;
        }
        RB_CORE_ASSERT(false, "Unknown Shader Type");
        return 0;
    }

    GLVertexArray::GLVertexArray()
        : m_IndexBuffer(nullptr), m_VertexBuffers()
    {
		RB_PROFILE_FUNC();

        glCreateVertexArrays(1, &m_RendererID);
    }

    GLVertexArray::~GLVertexArray()
    {
		RB_PROFILE_FUNC();

        glDeleteVertexArrays(1, &m_RendererID);
    }

    void Rubber::GLVertexArray::bind() const
    {
		RB_PROFILE_FUNC();

        glBindVertexArray(this->m_RendererID);
    }

    void Rubber::GLVertexArray::unbind() const
    {
		RB_PROFILE_FUNC();

        glBindVertexArray(0);
    }

    void Rubber::GLVertexArray::addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
    {	
		RB_PROFILE_FUNC();

        RB_CORE_ASSERT(vertexBuffer->getLayout().getElementBuffer().size(),
            "Layout is not set yet, make sure you set layout before add it to the vertex array");

        // remember to first bind vertex array before you actually configure vertex buffer
        glBindVertexArray(m_RendererID);
        vertexBuffer->bind();
        // configuring vertex attrib
        int i = 0;
        for (const auto& it : vertexBuffer->getLayout()) {
            glVertexAttribPointer(
                i,
                it.getComponentCount(),
                ShaderTypeToOpenGLType(it.type),
                it.normalized,
                vertexBuffer->getLayout().getStride(),
                (const void*)(static_cast<uintptr_t>(it.offset)));
            glEnableVertexAttribArray(i);
            ++i;
        }

        this->m_VertexBuffers.push_back(vertexBuffer);
    }

    void GLVertexArray::setIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
    {
		RB_PROFILE_FUNC();

        glBindVertexArray(m_RendererID);
        indexBuffer->bind();
        this->m_IndexBuffer = indexBuffer;
    }

    Vector<Ref<VertexBuffer>>& Rubber::GLVertexArray::getVertexBuffers()
    {
		RB_PROFILE_FUNC();

        RB_CORE_ASSERT(this->m_VertexBuffers.size(), "Nothing is in VertexBuffer, make sure add before you get it");
        return this->m_VertexBuffers;
    }

    Ref<IndexBuffer>& Rubber::GLVertexArray::getIndexBuffer()
    {
		RB_PROFILE_FUNC();

        RB_CORE_ASSERT(m_IndexBuffer, "index buffer is not set yet, make sure you add it before get it");
        return m_IndexBuffer;
    }
}
