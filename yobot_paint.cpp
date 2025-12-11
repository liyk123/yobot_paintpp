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

    inline bool SDLSetDrawColor(SDL_Renderer* renderer, const SDL_Color& color)
    {
        return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    constexpr auto windowSize = SDL_Point{ 480,640 };

    paint::paint() 
        : m_window(nullptr, &SDL_DestroyWindow)
        , m_renderer(nullptr, &SDL_DestroyRenderer)
    {
        auto ret = SDL_Init(SDL_INIT_VIDEO);
        m_window.reset(SDL_CreateWindow(PROJECT_NAME, windowSize.x, windowSize.y, /*SDL_WINDOW_HIDDEN |*/ SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_HIGH_PIXEL_DENSITY));
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
        constexpr auto white = SDL_Color{ 255,255,255,255 };
        constexpr auto halfTransparent = SDL_Color{ 0,0,0,128 };
        SDLSetDrawColor(m_renderer.get(), halfTransparent);
        SDL_RenderClear(m_renderer.get());
        constexpr auto margin = SDL_Point{ 10,30 };
        constexpr auto clipRect = SDL_Rect{ margin.x, margin.y, windowSize.x - margin.x * 2, windowSize.y - margin.y * 2 };
        SDLSetDrawColor(m_renderer.get(), white);
        SDL_SetRenderClipRect(m_renderer.get(), &clipRect);
        SDL_RenderFillRect(m_renderer.get(), nullptr);
        SDLSetDrawColor(m_renderer.get(), halfTransparent);
        auto panelRect = SDL_FRect{};
        SDL_RectToFRect(&clipRect, &panelRect);
        auto texture = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto targetRect = SDL_FRect{ margin.x * 2.0f,0.0f,(float)(texture->w / 8 * 5),(float)(texture->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 4 + targetRect.w,0.0f,panelRect.w - targetRect.w - 50.0f,targetRect.h / 4 };
        for (int i = 0; i < 5; i++)
        {
            targetRect.y = (float)(100 + 100 * i + margin.x);
            SDL_RenderLine(m_renderer.get(), panelRect.x, targetRect.y, panelRect.x + panelRect.w, targetRect.y);
            targetRect.y += panelRect.x;
            SDL_RenderTexture(m_renderer.get(), texture.get(), nullptr, &targetRect);
            HPRect.y = targetRect.y + targetRect.h / 5 * 2;
            SDL_RenderFillRect(m_renderer.get(), &HPRect);
        }
        auto oSurface = unique_sdl_surface(SDL_RenderReadPixels(m_renderer.get(), nullptr));
        IMG_SavePNG(oSurface.get(), "test.png");
        SDL_RenderPresent(m_renderer.get());
        SDL_Delay(5000);
        return 0;
    }
}