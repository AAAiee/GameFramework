#pragma once
#include <string>
#include "Rubber/Core/Utility.h"

namespace Rubber {

	// Types available in shaders defined in a abstract way
	enum class ShaderType : unsigned char {
		None = 0,
		Float, Float2, Float3, Float4, Mat2, Mat3, Mat4, Int, Int2, Int3, Int4,
		Uint, Uint2, Uint3, Uint4
	};

	// helper functions  
	uint32_t getShaderTypeSize(ShaderType type);

    /** Describes one attribute; size and offset are bytes within a vertex. */
	struct BufferElement {
		std::string   name;
		uint32_t      size;
		uint32_t	  offset;
		ShaderType    type;
		bool		  normalized;

		BufferElement()
			:name(""), size(0), offset(0), type(ShaderType::None), normalized(false)
		{

		}
		// offset can only be initialized once we see the whole layout
		BufferElement(ShaderType type, const std::string name, bool normalized=false)
			:name(name), type(type), normalized(normalized), offset(0)
		{
			this->size = getShaderTypeSize(type);
		}

		uint32_t getComponentCount() const;
		BufferElement(const BufferElement& other) = default;
		BufferElement& operator= (const BufferElement& other) = default;
		~BufferElement() = default;
	};

    /**
     * Owns attribute descriptions and computes byte offsets and byte stride.
     * Layout describes the caller's vertex memory; it does not own vertex data.
     * The GL path's broader integer/matrix/multiple-buffer layout support is
     * deferred; this contract does not expand its supported attribute formats.
     */
	class BufferLayout {
	public:
		inline const Vector<BufferElement>& getElementBuffer() const {
			return this->m_ElementBuffer;
		}

		BufferLayout() = default;
		BufferLayout(const std::initializer_list<BufferElement>& layout);
		~BufferLayout() = default;
		BufferLayout(const BufferLayout& other) = default;
		BufferLayout& operator= (const BufferLayout& other) = default;


		inline uint32_t getStride() const {
			return this->m_Stride;
		}
		Vector<BufferElement>::iterator begin(){
			return this->m_ElementBuffer.begin();
		}
		Vector<BufferElement>::iterator end() {
			return this->m_ElementBuffer.end();
		}
		Vector<BufferElement>::const_iterator begin() const {
			return this-> m_ElementBuffer.cbegin();
		}
		Vector<BufferElement>::const_iterator end() const {
			return this->m_ElementBuffer.cend();
		}
	private:
		Vector<BufferElement> m_ElementBuffer;
		uint32_t m_Stride = 0;
	};




}


