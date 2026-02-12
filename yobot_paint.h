#pragma once
#include <memory>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <array>
#include <string>
#include <future>
#include <functional>
#include "tools.hpp"

constexpr char IconDir[] = "icon";
constexpr char FontDir[] = "font";
constexpr char DefaultIcon[] = "000000.webp";
constexpr char DefaultFont[] = "NotoSansSC-Regular.ttf";
constexpr auto DefaultIconPath = FixedString(IconDir) + FixedString("/") + FixedString(DefaultIcon);
constexpr auto DefaultFontPath = FixedString(FontDir) + FixedString("/") + FixedString(DefaultFont);

namespace yobot {

    using SDLSurfaceDeleter = GenericDeleter<SDL_Surface, SDL_DestroySurface>;
    using unique_sdl_surface = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

    using SDLTextureDeleter = GenericDeleter<SDL_Texture, SDL_DestroyTexture>;
    using unique_sdl_texture = std::unique_ptr<SDL_Texture, SDLTextureDeleter>;

    using SDLWindowDeleter = GenericDeleter<SDL_Window, SDL_DestroyWindow>;
    using SDLRendererDeleter = GenericDeleter<SDL_Renderer, SDL_DestroyRenderer>;
    using SDLRendererTextEngineDeleter = GenericDeleter<TTF_TextEngine, TTF_DestroyRendererTextEngine>;

    using SDLFontDeleter = GenericDeleter<TTF_Font, TTF_CloseFont>;
    using unique_sdl_font = std::unique_ptr<TTF_Font, SDLFontDeleter>;

    using Progress = std::pair<std::uint64_t, std::uint64_t>;

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
        static std::string savePNGBuffer(unique_sdl_surface&& surface);
    private:
        void loadRes();
    public:
        paint& preparePanel(const std::array<std::uint64_t, 5>& iconIds);
        paint& refreshBackground(const char phase);
        paint& refreshTotalProgress(const char phase, const std::array<Progress, 2>& progresses);
        paint& refreshBossProgress(const std::uint64_t lap, const std::array<bool, 5>& lapFlags, const std::array<Progress, 5>& progresses);
        paint& show();
        void mainLoop();
        bool postDrawProcess(std::function<void()>& process, std::promise<unique_sdl_surface>& promise);
        bool postQuit();
    private:
        std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window;
        unique_sdl_surface m_windowSurafce;
        std::unique_ptr<SDL_Renderer, SDLRendererDeleter> m_renderer;
        std::unique_ptr<TTF_TextEngine, SDLRendererTextEngineDeleter> m_textEngine;
        unique_sdl_texture m_panel;
        unique_sdl_texture m_texture0;
        unique_sdl_font m_titleFont;
        unique_sdl_font m_lapFont;
        unique_sdl_font m_hpFont;
    };
}

