#pragma once
#include <memory>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <array>
#include <string>
#include <future>
#include <functional>

namespace yobot {

    template <typename T, auto (*Destructor)(T*)>
    struct GenericDeleter
    {
        void operator()(T* ptr) const noexcept
        {
            if (ptr) Destructor(ptr);
        }
    };

    using SDLSurfaceDeleter = GenericDeleter<SDL_Surface, SDL_DestroySurface>;
    using unique_sdl_surface = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

    using SDLTextureDeleter = GenericDeleter<SDL_Texture, SDL_DestroyTexture>;
    using unique_sdl_texture = std::unique_ptr<SDL_Texture, SDLTextureDeleter>;

    using SDLWindowDeleter = GenericDeleter<SDL_Window, SDL_DestroyWindow>;
    using SDLRendererDeleter = GenericDeleter<SDL_Renderer, SDL_DestroyRenderer>;
    using SDLRendererTextEngineDeleter = GenericDeleter<TTF_TextEngine, TTF_DestroyRendererTextEngine>;

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
    public:
        paint& preparePanel(const std::array<std::uint64_t, 5>& iconIds);
        paint& refreshBackground(SDL_Color color);
        paint& refreshTotalProgress(const char phase, const std::array<Progress, 2>& progresses);
        paint& refreshBossProgress(const std::array<std::uint64_t, 5>& laps, const std::array<Progress, 5>& progresses);
        paint& show();
        unique_sdl_surface saveSurface();
        void mainLoop();
        bool postDrawProcess(std::function<void()>& process, std::promise<unique_sdl_surface>& promise);
        bool postQuit();
    private:
        std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window;
        unique_sdl_surface m_windowSurafce;
        std::unique_ptr<SDL_Renderer, SDLRendererDeleter> m_renderer;
        std::unique_ptr<TTF_TextEngine, SDLRendererTextEngineDeleter> m_textEngine;
        unique_sdl_texture m_panel;
    };
}

