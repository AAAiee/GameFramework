#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Rubber{

	/// Owns a projection matrix; callers supply a valid, invertible matrix when
	/// requesting its inverse. No singular-matrix validation is performed.
	class Camera {
	public:
		Camera() = default;
		virtual ~Camera() = default;
		Camera(const glm::mat4& projection); 

		/// Copies the supplied projection into camera-owned storage.
		inline void setProjection(const glm::mat4& projectionMatrix) {
			this->m_ProjectionMatrix = projectionMatrix;
		}

		/// Borrows camera storage; writes through this reference change the projection.
		inline glm::mat4& getProjection(){
			return m_ProjectionMatrix;
		}

		/// Borrows camera storage, valid for the lifetime of this camera.
		inline const glm::mat4& getProjection() const {
			return m_ProjectionMatrix;
		}

		/// Returns an owned inverse snapshot without modifying the projection.
		/// The result remains valid after this camera changes or is destroyed.
		inline glm::mat4 getInverseProjection() const {
			return glm::inverse(m_ProjectionMatrix);
		}
	protected:
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
	};
}
