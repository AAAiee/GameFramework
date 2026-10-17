#pragma once
#include "Rubber/Renderer/RendererAPI.h"
#include "Rubber/Core/Core.h"

namespace Rubber {
	enum class RendererDrawMode;

	// mid layer between renderer and actually renderer API
	class RendererCommand {
	public:
		static void init();
		static void shutdown();
		static void setViewPort(int width, int height);
		static void clearColor(const glm::vec4& color);
		static void clear();
        /** Borrows the vertex array for a draw of count index elements, not bytes. Zero is forwarded unchanged. */
		static void drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0);
		static void setPolygonMode(RendererPolygonMode mode);
	private:
		static RendererAPI* s_RendererAPI;
	};
}
