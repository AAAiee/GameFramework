#pragma  once
#include <glm/glm.hpp>
#include <Rubber/Core/Utility.h>

namespace Rubber{

	class Texture2D;
	// Owns a shared Ref to its sprite sheet and four normalized UV coordinates;
	// no GPU texture is copied. UV order: bottom-left, bottom-right, top-right,
	// top-left (horizontal reversal swaps left/right). Index and cell sizes are
	// measured from the bottom-left, with cell sizes in pixels. Construction from
	// a sheet requires a non-null texture with positive dimensions. Releasing the
	// last sheet owner requires its live graphics context, as for Texture2D.
	// Default construction holds no sheet and zero UVs/dimensions.
	class SubTexture2D {
	public:
		// Borrows the stored Ref; copy it to retain shared ownership independently.
		inline const Ref<Texture2D>& getSpriteSheetTexture()  {
			return m_SpriteSheet;
		}
		// Borrows exactly four coordinates owned by this SubTexture2D; invalidated
		// by destruction. Copy the values to retain them independently.
		inline const glm::vec2* getTexCoord() const {
			return m_TexCoord;
		}

		SubTexture2D(const Ref<Texture2D>& spriteSheet, glm::vec2 minBounds, glm::vec2 maxBounds);
		SubTexture2D() = default;
		~SubTexture2D() = default;

		inline glm::vec2 getSubtextureDimension() const {
			return m_Dimension;
		}


	public:
		static Ref<SubTexture2D> creatFromIndex(const Ref<Texture2D>& SpriteSheet, const glm::vec2 index, const glm::vec2 cellSize, const glm::vec2 spriteSize = { 1, 1 }, bool oppositeDirection = false);

		static Ref<SubTexture2D> createFromTexture(const Ref<Texture2D>& spriteSheet, bool isOppositeDireciton);

		static Ref<SubTexture2D> create();

		
		
	private:
		Ref<Texture2D>  m_SpriteSheet;
		glm::vec2 m_TexCoord[4]{};;
		glm::vec2 m_Dimension{};
	};
}