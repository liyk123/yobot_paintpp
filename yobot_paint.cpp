#include "yobot_paint.h"
#include <spdlog/spdlog.h>
#include <SDL3/SDL_init.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_iostream.h>

namespace yobot {

    auto SDLSufaceDeleter = [](SDL_Surface* surface) { surface && (SDL_DestroySurface(surface), 1); };
    using unique_sdl_surface = std::unique_ptr<SDL_Surface, decltype(SDLSufaceDeleter)>;

    auto SDLIOStreamDeleter = [](SDL_IOStream* stream) { stream && (SDL_CloseIO(stream), 1); };
    using unique_sdl_iostream = std::unique_ptr<SDL_IOStream, decltype(SDLIOStreamDeleter)>;

    auto SDLTextureDeleter = [](SDL_Texture* texture) { texture && (SDL_DestroyTexture(texture), 1); };
    using unique_sdl_texture = std::unique_ptr<SDL_Texture, decltype(SDLTextureDeleter)>;

    paint::paint() 
        : m_window(nullptr, &SDL_DestroyWindow)
        , m_renderer(nullptr, &SDL_DestroyRenderer)
    {
        auto ret = SDL_Init(SDL_INIT_VIDEO);
        m_window.reset(SDL_CreateWindow(PROJECT_NAME, 480, 640, SDL_WINDOW_HIDDEN | SDL_WINDOW_TRANSPARENT));
        m_renderer.reset(SDL_CreateRenderer(m_window.get(), nullptr));
        SPDLOG_INFO("SDL_Init {} window:{} renderer:{}", ret, SDL_GetWindowID(m_window.get()), SDL_GetRendererName(m_renderer.get()));
    }
    paint::~paint()
    {
        m_renderer = nullptr;
        m_window = nullptr;
        SDL_Quit();
        SPDLOG_INFO("SDL_Quit");
    }
    paint& paint::getInstance()
    {
        static paint instance{};
        return instance;
    }
    int paint::draw()
    {
        SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 0);
        SDL_RenderClear(m_renderer.get());
        auto iStream = unique_sdl_iostream(SDL_IOFromFile("icon/312501.webp", "r"));
        auto iSurface = unique_sdl_surface(IMG_LoadWEBP_IO(iStream.get()));
        auto texture = unique_sdl_texture(SDL_CreateTextureFromSurface(m_renderer.get(), iSurface.get()));
        SDL_RenderTexture(m_renderer.get(), texture.get(), NULL, NULL);
        auto oSurface = unique_sdl_surface(SDL_RenderReadPixels(m_renderer.get(), nullptr));
        IMG_SavePNG(oSurface.get(), "test.png");
        return 0;
    }
}