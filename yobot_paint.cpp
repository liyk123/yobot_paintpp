#include "yobot_paint.h"
#include <spdlog/spdlog.h>
#include <SDL3/SDL_init.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace yobot {

    auto SDLSufaceDeleter = [](SDL_Surface* surface) { surface && (SDL_DestroySurface(surface), 1); };
    using unique_sdl_surface = std::unique_ptr<SDL_Surface, decltype(SDLSufaceDeleter)>;

    auto SDLIOStreamDeleter = [](SDL_IOStream* stream) { stream && (SDL_CloseIO(stream), 1); };
    using unique_sdl_iostream = std::unique_ptr<SDL_IOStream, decltype(SDLIOStreamDeleter)>;

    auto SDLTextureDeleter = [](SDL_Texture* texture) { texture && (SDL_DestroyTexture(texture), 1); };
    using unique_sdl_texture = std::unique_ptr<SDL_Texture, decltype(SDLTextureDeleter)>;

    auto SDLTextDeleter = [](TTF_Text* text) { text && (TTF_DestroyText(text), 1); };
    using unique_sdl_text = std::unique_ptr<TTF_Text, decltype(SDLTextDeleter)>;

    auto SDLFontDeleter = [](TTF_Font* font) { font && (TTF_CloseFont(font), 1); };
    using unique_sdl_font = std::unique_ptr<TTF_Font, decltype(SDLFontDeleter)>;

    inline bool SDLSetDrawColor(SDL_Renderer* renderer, const SDL_Color& color)
    {
        return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    constexpr auto windowSize = SDL_Point{ 480,640 };
    constexpr auto white = SDL_Color{ 255,255,255,255 };
    constexpr auto halfTransparent = SDL_Color{ 0,0,0,128 };
    constexpr auto margin = SDL_Point{ 10,30 };
    constexpr auto clipRect = SDL_Rect{ margin.x, margin.y, windowSize.x - margin.x * 2, windowSize.y - margin.y * 2 };

    paint::paint() 
        : m_window(nullptr, &SDL_DestroyWindow)
        , m_renderer(nullptr, &SDL_DestroyRenderer)
        , m_textEngine(nullptr, &TTF_DestroyRendererTextEngine)
    {
        auto sdlInitRet = SDL_Init(SDL_INIT_VIDEO);
        auto ttfInitRet = TTF_Init();
        m_window.reset(SDL_CreateWindow(PROJECT_NAME, windowSize.x, windowSize.y, /*SDL_WINDOW_HIDDEN |*/ SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP));
        m_renderer.reset(SDL_CreateRenderer(m_window.get(), nullptr));
        m_textEngine.reset(TTF_CreateRendererTextEngine(m_renderer.get()));
        SPDLOG_INFO("SDL_Init {} TTF_Init {} window:{} renderer:{}", sdlInitRet, ttfInitRet, SDL_GetWindowID(m_window.get()), SDL_GetRendererName(m_renderer.get()));
    }
    paint::~paint()
    {
        m_textEngine = nullptr;
        m_renderer = nullptr;
        m_window = nullptr;
        TTF_Quit();
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
        SDLSetDrawColor(m_renderer.get(), halfTransparent);
        SDL_RenderClear(m_renderer.get());
        SDLSetDrawColor(m_renderer.get(), white);
        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        SDL_RenderFillRect(m_renderer.get(), nullptr);
        auto panelRect = SDL_FRect{0.0f,0.0f,(float)clipRect.w,(float)clipRect.h};
        auto texture = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(texture->w / 8 * 5),(float)(texture->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        auto font = unique_sdl_font(TTF_OpenFont("font/NotoSansSC-VariableFont_wght.ttf", 16));
        SPDLOG_INFO("TTF_GetFontWeight {}", TTF_GetFontWeight(font.get()));
        auto text = unique_sdl_text(TTF_CreateText(m_textEngine.get(), font.get(), "test", 4));
        for (int i = 5; i > 0; i--)
        {
            SDLSetDrawColor(m_renderer.get(), halfTransparent);
            iconRect.y -= (float)(iconRect.h + margin.x * 2);
            SDL_RenderLine(m_renderer.get(), panelRect.x, iconRect.y, panelRect.x + panelRect.w, iconRect.y);
            iconRect.y += (float)margin.x;
            SDL_RenderTexture(m_renderer.get(), texture.get(), nullptr, &iconRect);
            HPRect.y = iconRect.y + iconRect.h / 5 * 2;
            SDL_RenderFillRect(m_renderer.get(), &HPRect);
            iconRect.y -= (float)margin.x;
            SDLSetDrawColor(m_renderer.get(), white);
            TTF_DrawRendererText(text.get(), HPRect.x + HPRect.w / 3, HPRect.y);
        }
        SDL_SetRenderViewport(m_renderer.get(), nullptr);
        auto oSurface = unique_sdl_surface(SDL_RenderReadPixels(m_renderer.get(), nullptr));
        IMG_SavePNG(oSurface.get(), "test.png");
        SDL_RenderPresent(m_renderer.get());
        SDL_Delay(5000);
        return 0;
    }
}