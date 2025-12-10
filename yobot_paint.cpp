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
        SDL_SetRenderDrawColor(m_renderer.get(), 255, 255, 255, 255);
        SDL_RenderClear(m_renderer.get());
        SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 128);
        auto targetRect = SDL_FRect{ 10.0f,30.0f,460.0f,580.0f };
        SDL_RenderRect(m_renderer.get(), &targetRect);
        auto texture = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/312501.webp"));
        targetRect = SDL_FRect{ 20.0f,20.0f,(float)(texture->w / 8 * 5),(float)(texture->h / 8 * 5) };
        auto HPRect = SDL_FRect{ 40.0f + targetRect.w,0,targetRect.w * 4,targetRect.h / 3 };
        for (int i = 0; i < 5; i++)
        {
            targetRect.y = (float)(110 + 100 * i);
            SDL_RenderLine(m_renderer.get(), 10.0f, targetRect.y, 469.0f, targetRect.y);
            targetRect.y += 10.0f;
            SDL_RenderTexture(m_renderer.get(), texture.get(), nullptr, &targetRect);
            HPRect.y = targetRect.y + targetRect.h / 3;
            SDL_RenderRect(m_renderer.get(), &HPRect);
        }
        auto oSurface = unique_sdl_surface(SDL_RenderReadPixels(m_renderer.get(), nullptr));
        IMG_SavePNG(oSurface.get(), "test.png");
        //SDL_RenderPresent(m_renderer.get());
        return 0;
    }
}