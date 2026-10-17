#pragma once
#include <string>
#include <Rubber/Core/Core.h>
#include <utility>
#include <Rubber/Core/Search.h>
#include <glm/glm.hpp>

/**
* @file Shaders.h
* @brief Shaders.h is the abstract base class for the shader program, the specific implementation
* for it is decided at run time by the static create method.
*/


//TODO: PARSE?
//TODO: Set Uniforms for various formats
//TODO: instead of writing all virtual functions  in shader, maybe just dynamic_cast it to the corresponding object?
namespace Rubber {

	// Creation, uniform updates, binding and destruction require a compatible
	// graphics context current on the calling thread. Paths and uniform names
	// accept bounded slices without a trailing terminator (no embedded NULs).
	// getName() borrows shader-owned storage until destruction.
	class Shader {
	public:
		virtual ~Shader() {};
		virtual void bind() const = 0; 
		virtual void unbind() const = 0;
		virtual std::string_view getName() const = 0;

		// -------------------------------------------------------------------
		// Uniform-setting API: pure virtual overloads for each supported type
	    // -------------------------------------------------------------------

		virtual void setInt(std::string_view name, const int value) = 0;
		virtual void setIntArray(std::string_view name, int* const values, uint32_t count) = 0;
		virtual void setFloat(std::string_view name, const float value) = 0;
		virtual void setFloat2(std::string_view name, const glm::vec2& value) = 0;
		virtual void setFloat3(std::string_view name, const glm::vec3& value) = 0;
		virtual void setFloat4(std::string_view name, const glm::vec4& value) = 0;
		virtual void setMat3(std::string_view name, const glm::mat3& matrix) = 0;
		virtual void setMat4(std::string_view name, const glm::mat4& matrix) = 0;

		// -------------------------------------------------------------------
		// Factory
	    // -------------------------------------------------------------------
		static Scope<Shader> create(std::string_view name,std::string_view vertexSrc, std::string_view fragmentSrc);
		static Scope<Shader> create(std::string_view path);
	};


	class ShaderLib {
	public:
		ShaderLib() = default;

		// add with given name
		void add(std::string_view name, const Ref<Shader>& shader);

		// add with automatically parsed name
	    void add(const Ref<Shader>& shader);

		// load and store as the 
		void load(std::string_view  path);

		// get a shader by name 
		const Ref<Shader> getShader(std::string_view name) const ;

	private:
		bool exists(std::string_view name) const;

	private:
		std::unordered_map<std::string, Ref<Shader>, stringHash, std::equal_to<>> m_AllShaders;
	};


}
