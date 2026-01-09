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

    template <typename T, void (*Destructor)(T*)>
    struct GenericDeleter
    {
        void operator()(T* ptr) const noexcept
        {
            if (ptr) Destructor(ptr);
        }
    };

    using SDLSurfaceDeleter = GenericDeleter<SDL_Surface, SDL_DestroySurface>;
    using unique_sdl_surface = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

    using SDLTextureDeleter = decltype([](SDL_Texture* texture) noexcept { SDL_DestroyTexture(texture); });
    using unique_sdl_texture = std::unique_ptr<SDL_Texture, SDLTextureDeleter>;

    using SDLWindowDeleter = decltype([](SDL_Window* window) noexcept { SDL_DestroyWindow(window); });
    using SDLRendererDeleter = decltype([](SDL_Renderer* renderer) noexcept { SDL_DestroyRenderer(renderer); });
    using SDLRendererTextEngineDeleter = decltype([](TTF_TextEngine* textEngine) noexcept { TTF_DestroyRendererTextEngine(textEngine); });

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
        paint& preparePanel();
        paint& refreshPanelIcons(std::array<std::uint64_t, 5> iconIds);
        paint& save();
        paint& refreshBackground(SDL_Color color);
        paint& refreshTotalProgress(const std::array<Progress, 2>& progresses);
        paint& refreshBossProgress(const std::array<Progress, 5>& progresses);
        paint& show();
        void mainLoop();
        bool postDrawProcess(std::function<void()>& process, std::promise<unique_sdl_surface>& promise);
    private:
        std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window;
        std::unique_ptr<SDL_Renderer, SDLRendererDeleter> m_renderer;
        std::unique_ptr<TTF_TextEngine, SDLRendererTextEngineDeleter> m_textEngine;
        unique_sdl_texture m_panel;
    };
}

