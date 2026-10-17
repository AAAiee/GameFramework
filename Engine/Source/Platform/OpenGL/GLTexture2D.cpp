#include <pch.h>
#include "GLTexture2D.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <glm/glm.hpp> 
#include <glm/gtc/integer.hpp>


namespace Rubber{
	namespace {
		// stb and setData supply contiguous rows, independent of the caller's row alignment.
		class TightUnpackRows {
		public:
			TightUnpackRows() {
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &m_Previous);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			}
			~TightUnpackRows() { glPixelStorei(GL_UNPACK_ALIGNMENT, m_Previous); }
			TightUnpackRows(const TightUnpackRows&) = delete;
			TightUnpackRows& operator=(const TightUnpackRows&) = delete;
		private:
			GLint m_Previous = 4;
		};
	}

	GLTexture2D::GLTexture2D(std::string_view path)
		:m_Path(path)
	{
		RB_PROFILE_FUNC();

		stbi_set_flip_vertically_on_load(true);

		unsigned char* data = nullptr;
		{// load
			RB_PROFILE_SCOPE("StbiLoadImage");
				data = stbi_load(m_Path.c_str(), &this->m_Width, &this->m_Height, &this->m_Channels, 0);
			if (!data) {
				RB_CORE_ASSERT(false, "Fail to load the texture image!");
			}
		}// load

		// Internal format can be different compared to the data format
		GLint internalFormat = 0, dataFormat = 0;
		if (m_Channels == 3){
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}else if ( m_Channels == 4){
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}

		RB_CORE_ASSERT(internalFormat && dataFormat, "only support rgba and rgb for now");

		m_DataFormat = dataFormat;
		m_InternalFormat = internalFormat;

		// OpenGL 3.3 legacy code
		//glGenTextures(1, &this->m_RendererID);
		//glBindTexture(GL_TEXTURE_2D, this->m_RendererID);
		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		uint32_t mipMapCount = static_cast<uint32_t>(1.0f + glm::floor(glm::log2(std::max((float)m_Width, (float)m_Height))));
		glTextureStorage2D(m_RendererID, mipMapCount, internalFormat, this->m_Width, this->m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		{
			TightUnpackRows unpackRows;
			glTextureSubImage2D(m_RendererID, 0, 0, 0, this->m_Width, this->m_Height, dataFormat, GL_UNSIGNED_BYTE, data);
		}
		glGenerateTextureMipmap(m_RendererID);

		stbi_image_free(data);
	}

	GLTexture2D::GLTexture2D(uint16_t width, uint16_t height)
		:m_Width(width), m_Height(height),m_Channels(0)
	{
		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		uint32_t mipMapCount = static_cast<uint32_t>(1.0f + glm::floor(glm::log2(std::max((float)m_Width, (float)m_Height))));
		glTextureStorage2D(m_RendererID, mipMapCount, m_InternalFormat, width, height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	GLTexture2D::~GLTexture2D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void GLTexture2D::bind(uint16_t slot) const
	{
		RB_PROFILE_FUNC();

		// 3.3 glActiveTexture(GL_TEXTURE0 + slot);
		glBindTextureUnit(slot, this->m_RendererID);
	}

	void GLTexture2D::unBind() const
	{
		RB_PROFILE_FUNC();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GLTexture2D::setData(void* data, uint32_t size)
	{
		RB_PROFILE_FUNC();

		uint16_t bytePerPixel = m_DataFormat == GL_RGB ? 3 : 4;
		RB_CORE_ASSERT(size == bytePerPixel * m_Width * m_Height, "Size of data must be the size of the full texture!");
		{
			TightUnpackRows unpackRows;
			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
		}
		glGenerateTextureMipmap(m_RendererID);
	}

}


