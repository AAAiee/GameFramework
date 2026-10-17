#pragma once
#include <glm/glm.hpp>
#include <memory>

namespace  Rubber {
	class VertexArray;

	enum class  RendererPolygonMode: uint8_t {
		FILL = 0,
		LINE
	};
	/**
	* Renderer API class, the base class for all API implementation
	* For example, an OpenGL Implementation will be OpenGLRendererAPI which
	* inherits from this class.
	*/
	class RendererAPI {
	public:
		enum class API : unsigned char {
			NONE = 0, OpenGL = 1
		};

	public:
		virtual void init() = 0;
		virtual ~RendererAPI() {};
		virtual void setViewPort(int width, int height) = 0;
		virtual void clearColor(const glm::vec4& color) = 0;
		virtual void clear() = 0; 
        /** Draws count uint32_t index elements, not bytes; zero-default semantics remain deferred. */
		virtual void drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count =0) = 0; 
		virtual void setPolygonMode(RendererPolygonMode mode) = 0;
		static inline RendererAPI::API getAPI() {
			return s_API;
		};

	private:
		static RendererAPI::API s_API;
	};
}