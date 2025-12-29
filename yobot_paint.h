#pragma once
#include <memory>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <array>

namespace yobot {

    constexpr inline auto SDLTextureDeleter = [](SDL_Texture* texture) { SDL_DestroyTexture(texture); };
    using unique_sdl_texture = std::unique_ptr<SDL_Texture, decltype(SDLTextureDeleter)>;

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
        paint& refreshPanelIcons(std::array<std::uint64_t, 5> iconIds);
        paint& save();
        paint& refreshBackground(SDL_Color color);
        paint& show();
    private:
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_window;
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> m_renderer;
        std::unique_ptr<TTF_TextEngine, decltype(&TTF_DestroyRendererTextEngine)> m_textEngine;
        unique_sdl_texture m_panel;
    };
}

