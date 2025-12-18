#pragma once
#include <memory>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>

namespace yobot {

    constexpr inline auto SDLSurfaceDeleter = [](SDL_Surface* surface) { SDL_DestroySurface(surface); };
    using unique_sdl_surface = std::unique_ptr<SDL_Surface, decltype(SDLSurfaceDeleter)> ;

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
        paint& preparePanel();
        paint& refreshPanelIcons(const char* data);
    private:
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_window;
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> m_renderer;
        std::unique_ptr<TTF_TextEngine, decltype(&TTF_DestroyRendererTextEngine)> m_textEngine;
        unique_sdl_surface m_panel;
    };
}

