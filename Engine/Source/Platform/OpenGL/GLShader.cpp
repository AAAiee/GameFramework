#include <pch.h>
#include <glad/glad.h>
#include "Platform/OpenGL/GLShader.h"
#include "glm/gtc/type_ptr.hpp"


namespace Rubber {

	// helper function to covert a type name to a given glEnum represents the type of the shader
	static GLint ShaderTypeStrToEnum(std::string_view typeName){
		RB_PROFILE_FUNC();

		if (typeName == "vertex"){
			return GL_VERTEX_SHADER;
		}else if (typeName == "fragment" || typeName == "pixel"){
			return GL_FRAGMENT_SHADER;
		} // possible add more

		RB_CORE_ASSERT(false, "Non-supported Shader Type");
		return -1;
	}

	GLShader::GLShader(std::string_view name, std::string_view vertexSrc, std::string_view fragmentSrc) :m_Name(name)
	{
		RB_PROFILE_FUNC();

		//<ShaderType, ShaderSrcStr>
		std::unordered_map<GLint, std::string> shaderSrcMap;
		shaderSrcMap[GL_VERTEX_SHADER] = vertexSrc;
		shaderSrcMap[GL_FRAGMENT_SHADER] = fragmentSrc;
		compile(shaderSrcMap);
	}

	GLShader::GLShader(std::string_view filepath)
	{
		RB_PROFILE_FUNC();

		// extract the file's name
		std::filesystem::path path(filepath);
		m_Name = path.stem().string();

		// read 
		std::string fileContent = readShaderFile(filepath);
		// reprocess 
		auto shaderSrcMap = preProcess(fileContent);
		//compile
		compile(shaderSrcMap);
	}

	GLShader::~GLShader()
	{
		glDeleteProgram(this->m_ShaderID);
	}

	void Rubber::GLShader::bind() const
	{
		glUseProgram(this->m_ShaderID);

	}

	void Rubber::GLShader::unbind() const
	{
		glUseProgram(0);
	}


	void GLShader::setInt(std::string_view name,const int value)
	{
		RB_PROFILE_FUNC();

		setUniform<int>(name, value);
	}

	void GLShader::setIntArray(std::string_view name, int* const values, uint32_t count)
	{
		RB_PROFILE_FUNC();

		setUniformnv<int*>(name, values, count);
	}

	void GLShader::setFloat(std::string_view name, const float value)
	{
		RB_PROFILE_FUNC();

		setUniform<float>(name, value);
	}

	void GLShader::setFloat2(std::string_view name, const glm::vec2& value)
	{
		RB_PROFILE_FUNC();

		setUniform<glm::vec2>(name, value);
	}

	void GLShader::setFloat3(std::string_view name, const glm::vec3& value)
	{
		RB_PROFILE_FUNC();

		setUniform<glm::vec3>(name, value);

	}

	void GLShader::setFloat4(std::string_view name, const glm::vec4& value)
	{
		RB_PROFILE_FUNC();

		setUniform<glm::vec4>(name, value);

	}

	void GLShader::setMat3(std::string_view name, const glm::mat3& value)
	{
		RB_PROFILE_FUNC();

		setUniform<glm::mat3>(name, value);
	}

	void GLShader::setMat4(std::string_view name, const glm::mat4& value)
	{
		RB_PROFILE_FUNC();

		setUniform<glm::mat4>(name, value);
	}

	GLint GLShader::getUniformLocation(std::string_view name) const
	{
		RB_PROFILE_FUNC();

		auto it = m_UniformCache.find(name);
		if (it != m_UniformCache.end())
			return it->second;

		const std::string terminatedName(name);
		GLint loc = glGetUniformLocation(m_ShaderID, terminatedName.c_str());
		RB_CORE_ASSERT(loc != -1, "Uniform  not found");
		m_UniformCache.emplace(std::string(name), loc);
		return loc;
	}

	// read in the whole shader file as a single string
	std::string GLShader::readShaderFile(std::string_view filepath)
	{
		RB_PROFILE_FUNC();

		std::string result;
		const std::string terminatedPath(filepath);
		std::ifstream in(terminatedPath.c_str(), std::ios::in | std::ios::binary);
		if (in) {
			in.seekg(0, in.end);
			size_t size = in.tellg();
			result.resize(size);
			in.seekg(0, in.beg);
			in.read(result.data(), size);
		}
		else {
			RB_CORE_ASSERT(false, "Can not open the shader file!");
		}
		return result;
	}

	std::unordered_map<GLint, std::string> GLShader::preProcess(std::string_view fileContent)
	{
		RB_PROFILE_FUNC();

		//<ShaderType, ShaderSrcStr>
		std::unordered_map<GLint, std::string> shaderSrcMap;

		// process content such that we can extract each pair of ShaderType : CorrespondingSrcStr
		const char* token = "#type";
		size_t pos = fileContent.find(token, 0);
		while (pos != std::string::npos) {

			//parse out the type name, #type XXX->get XXX
			// find the first of either "\r\n" or "\n"
			size_t eol = fileContent.find_first_of("\r\n", pos);
			if (eol == std::string_view::npos) {
				RB_CORE_ASSERT(false, "format wrong, make sure you mark \"#type shaderTypeName\"");
			}

			// 1 for the space in between #type XXX
			size_t typeBegin = pos + strlen(token) + 1;
			std::string_view shaderTypeInStr = fileContent.substr(typeBegin,  eol - typeBegin);

			GLint shaderType = ShaderTypeStrToEnum(shaderTypeInStr);

			// new Pos for next #type
			size_t newPos = fileContent.find(token, eol + 1);

			std::string_view curShaderTypeSrc = fileContent.substr(eol + 1,
				(newPos == std::string::npos ? fileContent.size() : newPos) - (eol + 1));

			shaderSrcMap.emplace(shaderType, curShaderTypeSrc);

			pos = newPos;
		}
		return shaderSrcMap;
	}

	void GLShader::compile(const std::unordered_map<GLint, std::string>& shaderSrcMap)
	{
		RB_PROFILE_FUNC();

		// container that record every created shader
		Vector<GLint> compiledShaders;

		compiledShaders.reserve(shaderSrcMap.size());
		for (auto& [shaderType, shaderSrc] : shaderSrcMap) {
			const char* curShaderTypeSrc = shaderSrc.c_str();
			uint32_t curShader = glCreateShader(shaderType);
			glShaderSource(curShader, 1, &curShaderTypeSrc, nullptr);

			// record what shader had been compiled
			compiledShaders.push_back(curShader);

			//compile vertex shader
			glCompileShader(curShader);
			int isCompiled = 0;
			glGetShaderiv(curShader, GL_COMPILE_STATUS, &isCompiled);
			if (!isCompiled) {
				int maxLength = 0;
				glGetShaderiv(curShader, GL_INFO_LOG_LENGTH, &maxLength);

				Vector<char> infoLog(maxLength);
				glGetShaderInfoLog(curShader, maxLength, &maxLength, infoLog.data());

				//clean up previous compiled shader if current shader fails to compile
				for(auto compiledShaderID: compiledShaders){
					glDeleteShader(compiledShaderID);
				}
				RB_CORE_ASSERT(false, infoLog.data());
			}
		}

		// program creation and linkage
		this->m_ShaderID = glCreateProgram();
		// go through each compiled shader and attach it
		for (auto compiledShaderID : compiledShaders) {
			glAttachShader(this->m_ShaderID, compiledShaderID);
		}

		// link the compiled shader 
		glLinkProgram(m_ShaderID);

		// debug if linkage fails 
		int isLinked = 0;
		glGetProgramiv(this->m_ShaderID, GL_LINK_STATUS, &isLinked);
		if (!isLinked) {
			int maxLength = 0;
			glGetProgramiv(this->m_ShaderID, GL_INFO_LOG_LENGTH, &maxLength);
			Vector<char> infoLog(maxLength);
			glGetProgramInfoLog(this->m_ShaderID, maxLength, &maxLength, infoLog.data());
			RB_CORE_ASSERT(false, infoLog.data());

			// clean up if failed, similar as above
			for (auto compiledShaderID : compiledShaders) {
				glDeleteShader(compiledShaderID);
			}
		}

		//after linked into a program, no need to keep shaders alive
		for (auto compiledShaderID : compiledShaders) {
			glDetachShader(this->m_ShaderID, compiledShaderID);
			glDeleteShader(compiledShaderID);
		}

	}

}
