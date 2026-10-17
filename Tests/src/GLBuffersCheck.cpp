#include "pch.h"
#include "GLBuffersCheck.h"
// Deliberate repeated includes: this translation unit is the header-guard check.
#include "Platform/OpenGL/GLBuffers.h"
#include "Platform/OpenGL/GLBuffers.h"
#include "Platform/OpenGL/GLVertexArray.h"
#include "Platform/OpenGL/GLVertexArray.h"
#include <glad/glad.h>
#include <stdexcept>

#ifdef RB_ENABLE_ASSERTS
#include <windows.h>
#include <spdlog/sinks/ostream_sink.h>
#endif

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

#ifdef RB_ENABLE_ASSERTS
    // Keep SEH in a leaf with no C++ objects requiring unwinding (MSVC C2712).
    // Catch only the expected break and the original defect's access violation;
    // an access violation is reported to the caller as a failed oracle.
    DWORD unsetIndexException(Rubber::GLVertexArray* array) {
        __try {
            array->getIndexBuffer();
        } __except (GetExceptionCode() == EXCEPTION_BREAKPOINT ||
                    GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                    ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            return GetExceptionCode();
        }
        return 0;
    }

    struct DiagnosticCapture {
        std::ostringstream output;
        std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink =
            std::make_shared<spdlog::sinks::ostream_sink_mt>(output);
        std::shared_ptr<spdlog::logger> logger = Rubber::Logger::GetCoreLogger();
        DiagnosticCapture() { logger->sinks().push_back(sink); }
        ~DiagnosticCapture() {
            auto& sinks = logger->sinks();
            sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
        }
    };
#endif

    struct VertexArrayBinding {
        GLint previous = 0;
        VertexArrayBinding() { glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previous); }
        ~VertexArrayBinding() { glBindVertexArray(static_cast<GLuint>(previous)); }
    };
}

namespace Rubber {
    void checkGLBuffers() {
        VertexArrayBinding binding;
        GLVertexArray array;
#ifdef RB_ENABLE_ASSERTS
        {
            DiagnosticCapture diagnostic;
            require(unsetIndexException(&array) == EXCEPTION_BREAKPOINT,
                "Unset index buffer must diagnose with a Debug breakpoint, not an access violation");
            require(diagnostic.output.str().find(
                "Assertion Failed: index buffer is not set yet, make sure you add it before get it") != std::string::npos,
                "Unset index buffer did not emit the intended assertion diagnostic");
        }
#endif
        uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
        Ref<IndexBuffer> index = std::make_shared<GLIndexBuffer>(indices, sizeof(indices));
        array.setIndexBuffer(index);
        auto& returned = array.getIndexBuffer();
        require(returned.get() == index.get(), "Index accessor changed the supplied buffer");
        require(&returned == &array.getIndexBuffer(), "Index accessor must borrow the stored Ref");
        require(returned->getCount() == 6, "Index allocation size must be bytes, count must be elements");
        GLint bytes = 0;
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes);
        require(bytes == sizeof(indices), "Index allocation has the wrong byte size");
        index->uploadIndexData(indices, 3 * sizeof(uint32_t));
        require(array.getIndexBuffer().get() == index.get() && index->getCount() == 3,
            "Upload must retain buffer identity and update the element count");
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes);
        require(bytes == 3 * sizeof(uint32_t), "Index upload has the wrong byte size");

        std::weak_ptr<IndexBuffer> retained = index;
        index.reset();
        require(!retained.expired() && array.getIndexBuffer().get() == returned.get(),
            "Vertex array did not retain shared ownership of its index buffer");

        // A present buffer with zero elements is distinct from an unset pointer.
        Ref<IndexBuffer> empty = std::make_shared<GLIndexBuffer>(0);
        array.setIndexBuffer(empty);
        require(array.getIndexBuffer().get() == empty.get() && empty->getCount() == 0,
            "A present empty index buffer must be returned unchanged");
        require(retained.expired(), "Replacing the index buffer did not release the previous owner");
        require(glGetError() == GL_NO_ERROR, "Buffer fixture produced an OpenGL error");
    }
}
