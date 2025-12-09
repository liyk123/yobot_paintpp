#pragma once
#include <memory>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>

namespace yobot {
    class paint
    {
    private:
        paint();
        ~paint();
    public:
        paint(paint&) = delete;
        paint(paint&&) = delete;
        static paint& getInstance();
    public:
        int draw();
    private:
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_window;
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> m_renderer;
    };
}

