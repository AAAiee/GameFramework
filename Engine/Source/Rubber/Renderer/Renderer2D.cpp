#include <pch.h>
#include "Renderer2D.h"
#include "Rubber/Renderer/BufferLayout.h"
#include "Rubber/Renderer/Shaders.h"
#include "Rubber/Renderer/Buffers.h"
#include "Rubber/Renderer/VertexArray.h"
#include "Rubber/Renderer/Texture.h"
#include "Rubber/Renderer/OrthoCamera.h"
#include "Rubber/Renderer/RendererCommand.h"

#include "Rubber/Renderer/Subtexture2D.h"

namespace Rubber {

	struct Vertex{
		glm::vec4 m_Position;
		glm::vec4 m_Color;
		glm::vec2 m_TexCoord;
		float m_TexIndex = 0.0f;
		float m_TillingFactor = 0.0f;
		glm::vec4 m_TintFacor;
	};

	struct RendererData{
		static constexpr const uint32_t  MAX_QUAD_PER_DRAW = 10000;
		static constexpr const uint32_t  MAX_VERTEX_PER_DRAW = MAX_QUAD_PER_DRAW * 4;
		static constexpr const uint32_t  MAX_INDEX_NUMBER_PER_DRAW = MAX_QUAD_PER_DRAW * 6;
		static constexpr const uint32_t  MAX_TEXTURE_SLOTS = 32; // 32 for now, latter changed for renderer cap
		static constexpr const glm::vec4 UNIT_QUAD_POS[4] = {
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f,  0.5f, 0.0f, 1.0f},
			{ -0.5f, 0.5f, 0.0f, 1.0f}
		};
		static constexpr const glm::vec2 UNIT_QUAD_TEX_COORD[4] = {
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 1.0f},
			{0.0f, 1.0f}
		};

		std::array<Ref<Texture>, MAX_TEXTURE_SLOTS> m_TexturesMap;
		uint32_t  m_ValidTextureCount = 0ui32;

		Ref<VertexArray> m_VAO = nullptr;
		Ref<VertexBuffer> m_VBO = nullptr;
		Ref<IndexBuffer> m_IBO = nullptr;
		Ref<ShaderLib> m_ShaderLib = nullptr;
		Ref<Texture2D> m_WhiteTexture = nullptr;

		Vertex* m_Vertices = nullptr;
		Vertex* m_VerticesInsertPosPtr = nullptr;

		uint32_t* m_Indices = nullptr;
		uint32_t m_ValidIndexCount = 0ui32;
	
		~RendererData() = default;
	};

	RendererData s_Data = RendererData();
#if ENABLE_RENDERER_STATS
	Renderer2D::RendererStats Renderer2D::s_Stats;
#endif 


	void Renderer2D::init()
	{
		BufferLayout VBLayout = {
			{ShaderType::Float4, "aPos"      ,     false},
			{ShaderType::Float4, "aColor"    ,     false},
			{ShaderType::Float2, "aTexCoord" ,     false},
			{ShaderType::Float,  "aTexIndex" ,     false},
			{ShaderType::Float,  "aTillingFactor", false},
			{ShaderType::Float4, "aTintColor",     false }
		};

		s_Data.m_VAO = VertexArray::create();
		s_Data.m_VAO->bind();
		s_Data.m_VBO = VertexBuffer::create(sizeof(Vertex) * s_Data.MAX_VERTEX_PER_DRAW);

		s_Data.m_VBO->setLayout(VBLayout);
		s_Data.m_IBO = IndexBuffer::create(sizeof(uint32_t) * s_Data.MAX_INDEX_NUMBER_PER_DRAW);

		s_Data.m_VAO->addVertexBuffer(s_Data.m_VBO);
		s_Data.m_VAO->setIndexBuffer(s_Data.m_IBO);

		s_Data.m_ShaderLib = makeRef<ShaderLib>();
		s_Data.m_ShaderLib->load("assets/shader/CommonShader.glsl");

		s_Data.m_WhiteTexture = Texture2D::create(1,1);

		uint32_t whiteTexture = 0xFFFFFFFF;
		s_Data.m_WhiteTexture->setData(&whiteTexture, 4);

		// set up a buffer to store all vertices ..
		s_Data.m_Vertices = new Vertex[RendererData::MAX_VERTEX_PER_DRAW];

		//set up a buffer to store all indices 
		s_Data.m_Indices = new uint32_t[RendererData::MAX_INDEX_NUMBER_PER_DRAW];

		// fill in index for maxQuad
		for (uint32_t i = 0, offset = 0; i + 5 < s_Data.MAX_INDEX_NUMBER_PER_DRAW; i += 6) {
			s_Data.m_Indices[i] = offset + 0;
			s_Data.m_Indices[i + 1] = offset + 1;
			s_Data.m_Indices[i + 2] = offset + 2;
			s_Data.m_Indices[i + 3] = offset + 0;
			s_Data.m_Indices[i + 4] = offset + 2;
			s_Data.m_Indices[i + 5] = offset + 3;
			offset += 4;
		}

		s_Data.m_IBO->uploadIndexData(s_Data.m_Indices, s_Data.MAX_INDEX_NUMBER_PER_DRAW * sizeof(uint32_t));
		delete[] s_Data.m_Indices;

		// texture
		s_Data.m_TexturesMap[0] = s_Data.m_WhiteTexture;
		int samplers[s_Data.MAX_TEXTURE_SLOTS];
		for (int i = 0; i < s_Data.MAX_TEXTURE_SLOTS; ++i) {
			samplers[i] = i;
		}

		s_Data.m_ShaderLib->getShader("CommonShader")->setIntArray("u_Textures", samplers, s_Data.MAX_TEXTURE_SLOTS);

		s_Data.m_ValidTextureCount++;
		s_Data.m_VerticesInsertPosPtr = s_Data.m_Vertices;

#if ENABLE_RENDERER_STATS
		std::memset(&s_Stats, 0ui32, sizeof(RendererStats));
#endif 
	}

	void Renderer2D::shutdown()
	{
		s_Data.m_ShaderLib.reset();
		s_Data.m_VAO.reset();
		s_Data.m_VBO.reset();
		s_Data.m_IBO.reset();
		s_Data.m_WhiteTexture.reset();

		for (auto& t : s_Data.m_TexturesMap) t.reset();

		delete[] s_Data.m_Vertices;
		s_Data.m_Vertices = nullptr;
		s_Data.m_VerticesInsertPosPtr = nullptr;

		s_Data.m_ValidIndexCount = 0;
		s_Data.m_ValidTextureCount = 0;
	}

	void Renderer2D::beginScene(const glm::mat4& vpMatrix)
	{
		RB_PROFILE_FUNC();
		Ref<Shader> shader = s_Data.m_ShaderLib->getShader("CommonShader");
		shader->setMat4("u_ViewProjectionMatrix", vpMatrix);
	}

	void Renderer2D::beginScene(const OrthoCamera& camera)
	{
		RB_PROFILE_FUNC();
		Ref<Shader> shader = s_Data.m_ShaderLib->getShader("CommonShader");
		shader->setMat4("u_ViewProjectionMatrix", camera.getVpMatrix());
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& scale, const glm::vec4& color, const float tillingFactor, const glm::vec4& tintColor)
	{
		RB_PROFILE_FUNC();

		glm::mat4 tsMatrix = glm::scale(glm::translate(glm::mat4(1.0f), position), { scale.x, scale.y, 1.0f });
		drawColorQuad(tsMatrix, color, tillingFactor, tintColor);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& scale, const Ref<Texture2D>& texture, const float tillingFactor, const glm::vec4& tintColor)
	{
		RB_PROFILE_FUNC();

	
		glm::mat4 tsMatrix = glm::translate(glm::mat4(1), position)   
			* glm::scale(glm::mat4(1), { scale.x, scale.y, 1 });
		drawTextureQuad(tsMatrix, texture, RendererData::UNIT_QUAD_TEX_COORD,tillingFactor, tintColor);
	}
	

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& scale, const Ref<SubTexture2D>& subTexture, const float tillingFactor /*= 1.f*/, const glm::vec4& tintColor /*= glm::vec4(1.0f)*/)
	{
		RB_PROFILE_FUNC();

		glm::mat4 tsMatrix = glm::translate(glm::mat4(1), position)
			* glm::scale(glm::mat4(1), { scale.x, scale.y, 1 });

		const Ref<Texture2D>& spriteSheetTexture = subTexture->getSpriteSheetTexture();
		drawTextureQuad(tsMatrix, spriteSheetTexture, subTexture->getTexCoord(), tillingFactor, tintColor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& scale, const float radians, const Ref<Texture2D>& texture, const float tillingFactor, const glm::vec4& tintColor)
    {  
       RB_PROFILE_FUNC();  

	   glm::mat4 tsMatrix = glm::scale(
		   glm::rotate(
			   glm::translate(glm::mat4(1.0f), position),
			   radians, glm::vec3(0, 0, 1)),
		   glm::vec3(scale.x, scale.y, 1.0f));

	   drawTextureQuad(tsMatrix, texture, RendererData::UNIT_QUAD_TEX_COORD, tillingFactor, tintColor);
    }

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& scale, const float radians, const glm::vec4& color, const float TillingFactor /*= 1.f*/, const glm::vec4& tintColor /*= glm::vec4(1.0f)*/)
	{
		RB_PROFILE_FUNC();

		glm::mat4 tsMatrix = glm::scale(                                  
				glm::rotate(                             
					glm::translate(glm::mat4(1.0f), position),  
					radians, glm::vec3(0, 0, 1)),
				glm::vec3(scale.x, scale.y, 1.0f));

		drawColorQuad(tsMatrix, color, TillingFactor,tintColor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& scale, const float radians, const Ref<SubTexture2D>& subTexture, const float TillingFactor /*= 1.f*/, const glm::vec4& tintColor /*= glm::vec4(1.0f)*/)
	{

		RB_PROFILE_FUNC();

		glm::mat4 tsMatrix = glm::scale(
			glm::rotate(
				glm::translate(glm::mat4(1.0f), position),
				radians, glm::vec3(0, 0, 1)),
			glm::vec3(scale.x, scale.y, 1.0f));

		Ref<Texture2D> spriteSheetTexture = subTexture->getSpriteSheetTexture();
		drawTextureQuad(tsMatrix, spriteSheetTexture, subTexture->getTexCoord(), TillingFactor, tintColor);
	}

	void Renderer2D::endScene()
	{
		flush();
	}

	void Renderer2D::flush()
	{
		RB_PROFILE_FUNC();
		if (s_Data.m_ValidIndexCount == 0) {
			return;
		}

		s_Data.m_VBO->uploadVertexData(s_Data.m_Vertices, static_cast<uint32_t>((s_Data.m_VerticesInsertPosPtr - s_Data.m_Vertices)) * static_cast<uint32_t> (sizeof(Vertex)));
		for(uint32_t i = 0;  i < s_Data.m_ValidTextureCount ; ++i){
			s_Data.m_TexturesMap[i]->bind(i);
		}


		RendererCommand::drawIndexed(s_Data.m_VAO, s_Data.m_ValidIndexCount);

		// clean up for next batch
		const uint32_t  oldCount = s_Data.m_ValidTextureCount;
		s_Data.m_VerticesInsertPosPtr = s_Data.m_Vertices;
		s_Data.m_ValidIndexCount = 0;
		s_Data.m_ValidTextureCount = 1;

		for (uint32_t i = 1; i < oldCount; ++i) {
			s_Data.m_TexturesMap[i].reset();
		}


#if ENABLE_RENDERER_STATS 
	s_Stats.m_DrawCallCount++;
#endif
	}


	void Renderer2D::drawTextureQuad(const glm::mat4& transformation, const Ref<Texture2D>& texture, const glm::vec2* textCoord,  const float tillingFactor, const glm::vec4& tintColor)
	{
		RB_PROFILE_FUNC();

		// Resolve slots only after a quad-capacity flush has reset the batch.
		if (s_Data.m_ValidIndexCount >= s_Data.MAX_INDEX_NUMBER_PER_DRAW) {
			flush();
		}

		float texIndex = 0.0f;
		// check if we already have this texture, if we do, reuse
		for (uint32_t i = 1; i < s_Data.m_ValidTextureCount; i++) {
			if (*texture == *(s_Data.m_TexturesMap[i])) {
				texIndex = static_cast<float>(i);
				break;
			}
		}

		if (texIndex == 0.0f) { // new texture has been passed in, store it in a new texture slot
			if (s_Data.m_ValidTextureCount == s_Data.MAX_TEXTURE_SLOTS) {
				flush();
			}
			s_Data.m_TexturesMap[s_Data.m_ValidTextureCount] = texture;
			texIndex = static_cast<float> (s_Data.m_ValidTextureCount);
			s_Data.m_ValidTextureCount++;
		}

		RB_CORE_ASSERT(texIndex != 0.0f, "Texture is not binded sucessfully!");

		const glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; //white for default, since we draw texture instead of a specific color
		if  (textCoord == nullptr){
			textCoord = RendererData::UNIT_QUAD_TEX_COORD;
		}
		
		for (int i = 0; i < 4; i++) {
			s_Data.m_VerticesInsertPosPtr->m_Position = transformation * RendererData::UNIT_QUAD_POS[i];
			s_Data.m_VerticesInsertPosPtr->m_Color = color;
			s_Data.m_VerticesInsertPosPtr->m_TexCoord = textCoord[i];
			s_Data.m_VerticesInsertPosPtr->m_TexIndex = { texIndex };
			s_Data.m_VerticesInsertPosPtr->m_TillingFactor = tillingFactor;
			s_Data.m_VerticesInsertPosPtr->m_TintFacor = tintColor;
			s_Data.m_VerticesInsertPosPtr++;
		}
		s_Data.m_ValidIndexCount += 6;

#if ENABLE_RENDERER_STATS
		s_Stats.m_QuadNumber++;
#endif 
	}


	void Renderer2D::drawColorQuad(const glm::mat4& transformation, const glm::vec4& color, float  tillingFacor /*= 1.f*/, const glm::vec4& tintColor /*= glm::vec4(1.0f)*/)
	{
		RB_PROFILE_FUNC();
		if (s_Data.m_ValidIndexCount >= s_Data.MAX_INDEX_NUMBER_PER_DRAW) {
			flush();
		}

		//scale->translation
		const float texIndex = 0.0f; // in this function we only draw color, no texture (white texture by default)
		for (int i = 0; i < 4; i++) {
			s_Data.m_VerticesInsertPosPtr->m_Position = transformation * s_Data.UNIT_QUAD_POS[i];
			s_Data.m_VerticesInsertPosPtr->m_Color = color;
			s_Data.m_VerticesInsertPosPtr->m_TexCoord = s_Data.UNIT_QUAD_TEX_COORD[i];
			s_Data.m_VerticesInsertPosPtr->m_TexIndex = { texIndex };
			s_Data.m_VerticesInsertPosPtr->m_TillingFactor = tillingFacor;
			s_Data.m_VerticesInsertPosPtr->m_TintFacor = tintColor;
			s_Data.m_VerticesInsertPosPtr++;
		}
		s_Data.m_ValidIndexCount += 6;
#if ENABLE_RENDERER_STATS 
		s_Stats.m_QuadNumber++;
#endif
	}




#if ENABLE_RENDERER_STATS
	
	uint32_t Renderer2D::getDrawCallCount()
	{
		return s_Stats.m_DrawCallCount;
	}

	uint32_t Renderer2D::getQuadCount()
	{
		return s_Stats.m_QuadNumber;

	}

	uint32_t Renderer2D::getVertexCount()
	{
		return s_Stats.getVertexCount();

	}

	uint32_t Renderer2D::getIndexCount()
	{
		return s_Stats.getIndexCount();
	}

	void Renderer2D::resetRendererStat()
	{
		std::memset(&s_Stats, 0ui32, sizeof(RendererStats));
	}

#endif

}
