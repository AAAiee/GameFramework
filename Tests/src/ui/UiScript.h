#pragma once

#include "EditorLayer.h"
#include "../EditorDriver.h"

#include <coroutine>
#include <cstddef>
#include <exception>
#include <string_view>
#include <utility>

namespace Rubber {
    class Application;

    namespace UiScript {

        /** Per-frame state shared by a script and the fixture that resumes it. */
        struct Context {
            EditorDriver& driver;
            EditorLayer& editor;
            bool dialogSeen = false;
            bool closing = false;
        };

        /** A script that suspends at each co_await settle(ctx) and resumes on the next idle frame. */
        class Script {
        public:
            struct promise_type {
                std::exception_ptr error;
                Script get_return_object();
                std::suspend_always initial_suspend() noexcept { return {}; }
                std::suspend_always final_suspend() noexcept { return {}; }
                void return_void() noexcept {}
                void unhandled_exception() noexcept { error = std::current_exception(); }
            };

            using handle_type = std::coroutine_handle<promise_type>;

            explicit Script(handle_type handle) : m_Handle(handle) {}
            Script(Script&& other) noexcept : m_Handle(std::exchange(other.m_Handle, {})) {}
            Script(const Script&) = delete;
            Script& operator=(const Script&) = delete;
            ~Script() {
                if (m_Handle) m_Handle.destroy();
            }

            void resume() {
                if (m_Handle && !m_Handle.done()) m_Handle.resume();
            }
            bool done() const { return !m_Handle || m_Handle.done(); }
            void rethrow() const {
                if (m_Handle && m_Handle.promise().error)
                    std::rethrow_exception(m_Handle.promise().error);
            }

        private:
            handle_type m_Handle{};
        };

        inline Script Script::promise_type::get_return_object() {
            return Script{handle_type::from_promise(*this)};
        }

        class Settle {
        public:
            Settle(EditorDriver& driver, int frames) { driver.waitFrames(frames); }
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<>) const noexcept {}
            void await_resume() const noexcept {}
        };

        inline Settle settle(Context& context, int frames = 2) {
            return Settle(context.driver, frames);
        }

        void require(bool condition, std::string_view message);
        std::size_t taggedCount(Scene& scene);
        Entity firstTagged(Scene& scene);
        void expect(std::string_view path, int occurrence = 0);

        /** Runs one script against the production EditorLayer until it completes or the app closes. */
        void run(Application& application, Script (*entry)(Context&));
    }
}
