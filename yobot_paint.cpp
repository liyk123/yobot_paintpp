#include "yobot_paint.h"
#include <spdlog/spdlog.h>
#include <SDL3/SDL_init.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <ranges>

namespace yobot {

    constexpr inline auto SDLIOStreamDeleter = [](SDL_IOStream* stream) { SDL_CloseIO(stream); };
    using unique_sdl_iostream = std::unique_ptr<SDL_IOStream, decltype(SDLIOStreamDeleter)>;

    constexpr inline auto SDLSurfaceDeleter = [](SDL_Surface* surface) { puts(std::format("0x{:X}", (long long)surface).c_str()); SDL_DestroySurface(surface); };
    using unique_sdl_surface = std::unique_ptr<SDL_Surface, decltype(SDLSurfaceDeleter)>;

    constexpr inline auto SDLTextDeleter = [](TTF_Text* text) { TTF_DestroyText(text); };
    using unique_sdl_text = std::unique_ptr<TTF_Text, decltype(SDLTextDeleter)>;

    constexpr inline auto SDLFontDeleter = [](TTF_Font* font) { TTF_CloseFont(font); };
    using unique_sdl_font = std::unique_ptr<TTF_Font, decltype(SDLFontDeleter)>;

    inline bool SDLSetDrawColor(SDL_Renderer* renderer, const SDL_Color& color)
    {
        return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    inline bool SDLSetTextColor(TTF_Text* text, const SDL_Color& color)
    {
        return TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
    }

    constexpr auto windowSize = SDL_Point{ 480,640 };
    constexpr auto white = SDL_Color{ 255,255,255,128 };
    constexpr auto halfTransparent = SDL_Color{ 0,0,0,128 };
    constexpr auto transparent = SDL_Color{ 0,0,0,0 };
    constexpr auto black = SDL_Color{ 0,0,0,255 };
    constexpr auto margin = SDL_Point{ 10,30 };
    constexpr auto clipRect = SDL_Rect{ margin.x, margin.y, windowSize.x - margin.x * 2, windowSize.y - margin.y * 2 };
    constexpr auto panelRect = SDL_FRect{ 0.0f,0.0f,(float)clipRect.w,(float)clipRect.h };

    paint::paint() 
        : m_window(nullptr, &SDL_DestroyWindow)
        , m_renderer(nullptr, &SDL_DestroyRenderer)
        , m_textEngine(nullptr, &TTF_DestroyRendererTextEngine)
        , m_panel(nullptr)
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
        m_panel = nullptr;
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
    paint& paint::preparePanel()
    {
        SDLSetDrawColor(m_renderer.get(), halfTransparent);
        SDL_RenderClear(m_renderer.get());
        SDLSetDrawColor(m_renderer.get(), white);
        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        SDL_RenderFillRect(m_renderer.get(), nullptr);
        auto texture = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(texture->w / 8 * 5),(float)(texture->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        auto font = unique_sdl_font(TTF_OpenFont("font/NotoSansSC-Regular.ttf", 12));
        TTF_SetFontHinting(font.get(), TTF_HINTING_LIGHT_SUBPIXEL);
        TTF_SetFontSDF(font.get(), true);
        auto lapFont = unique_sdl_font(TTF_CopyFont(font.get()));
        TTF_SetFontStyle(lapFont.get(), TTF_STYLE_BOLD);
        TTF_SetFontSize(lapFont.get(), 18);
        auto lapText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), lapFont.get(), "周目", 6));
        auto titleFont = unique_sdl_font(TTF_CopyFont(font.get()));
        TTF_SetFontSize(titleFont.get(), 24);
        auto phaseText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), titleFont.get(), "阶段", 6));
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
            auto lapRect = SDL_FRect{ HPRect.x, iconRect.y + margin.x + 4 , 18 * 4,22 };
            SDLSetDrawColor(m_renderer.get(), transparent);
            SDL_RenderFillRect(m_renderer.get(), &lapRect);
            TTF_DrawRendererText(lapText.get(), HPRect.x + margin.x / 2, iconRect.y + margin.x);
        }
        auto phaseRect = SDL_FRect{ iconRect.x, (float)(margin.x), iconRect.w, iconRect.y - margin.x * 2 };
        SDLSetDrawColor(m_renderer.get(), transparent);
        SDL_RenderFillRect(m_renderer.get(), &phaseRect);
        TTF_DrawRendererText(phaseText.get(), panelRect.x + margin.x * 5 / 2, panelRect.y + margin.x / 2 + 2);
        auto progressRect = SDL_FRect{ HPRect.x, phaseRect.y - margin.x / 5 * 2, HPRect.w, phaseRect.h / 2 };
        SDL_RenderFillRect(m_renderer.get(), &progressRect);
        progressRect.y += margin.x / 5 * 4 + progressRect.h;
        SDL_RenderFillRect(m_renderer.get(), &progressRect);
        return *this;
    }
    paint& paint::refreshPanelIcons(std::array<std::uint64_t, 5> iconIds)
    {
        auto texture0 = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(texture0->w / 8 * 5),(float)(texture0->h / 8 * 5) };

        for (auto&& id : iconIds | std::views::reverse)
        {
            auto path = std::format("icon/{}.webp", id);
            auto texture = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), path.c_str()));
            iconRect.y -= iconRect.h + (float)margin.x;
            SDL_RenderTexture(m_renderer.get(), texture.get(), nullptr, &iconRect);
            iconRect.y -= (float)margin.x;
        }
        SDL_SetRenderViewport(m_renderer.get(), nullptr);
        
        return *this;
    }

    paint& paint::save()
    {
        SDL_SetRenderViewport(m_renderer.get(), nullptr);
        auto surface = unique_sdl_surface(SDL_RenderReadPixels(m_renderer.get(), nullptr));
        m_panel.reset(SDL_CreateTextureFromSurface(m_renderer.get(), surface.get()));
        IMG_SavePNG(surface.get(), "test.png");
        return *this;
    }

    paint& paint::refreshBackground(SDL_Color color)
    {
        SDLSetDrawColor(m_renderer.get(), color);
        SDL_RenderClear(m_renderer.get());
        SDL_SetRenderDrawBlendMode(m_renderer.get(), SDL_BLENDMODE_BLEND);
        SDL_RenderTexture(m_renderer.get(), m_panel.get(), nullptr, nullptr);
        return *this;
    }

    paint& paint::show()
    {
        SDL_RenderPresent(m_renderer.get());
        SDL_Delay(5000);
        return *this;
    }
    
}