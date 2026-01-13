#include "yobot_paint.h"
#include <spdlog/spdlog.h>
#include <SDL3/SDL_init.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <ranges>
#include <future>

namespace yobot {

    enum class PaintEvent
    {
        MOUSE_BUTTON_UP = SDL_EVENT_MOUSE_BUTTON_UP,
        QUIT = SDL_EVENT_QUIT,
        DRAW_PROCESS = SDL_EVENT_USER + 1,
    };

    using SDLIOStreamDeleter = GenericDeleter<SDL_IOStream, SDL_CloseIO>;
    using unique_sdl_iostream = std::unique_ptr<SDL_IOStream, SDLIOStreamDeleter>;

    using SDLTextDeleter = GenericDeleter<TTF_Text, TTF_DestroyText>;
    using unique_sdl_text = std::unique_ptr<TTF_Text, SDLTextDeleter>;

    using SDLFontDeleter = decltype([](TTF_Font* font) noexcept { TTF_CloseFont(font); });
    using unique_sdl_font = std::unique_ptr<TTF_Font, SDLFontDeleter>;

    inline bool SDLSetDrawColor(SDL_Renderer* renderer, const SDL_Color& color) noexcept
    {
        return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    inline bool SDLSetTextColor(TTF_Text* text, const SDL_Color& color) noexcept
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
        : m_window(nullptr)
        , m_renderer(nullptr)
        , m_textEngine(nullptr)
        , m_panel(nullptr)
    {
        auto sdlInitRet = SDL_Init(SDL_INIT_VIDEO);
        auto ttfInitRet = TTF_Init();
        m_window.reset(SDL_CreateWindow(PROJECT_NAME, windowSize.x, windowSize.y, /*SDL_WINDOW_HIDDEN |*/ SDL_WINDOW_TRANSPARENT));
        m_renderer.reset(SDL_CreateRenderer(m_window.get(), nullptr));
        m_textEngine.reset(TTF_CreateRendererTextEngine(m_renderer.get()));
        SPDLOG_INFO("SDL_Init {} TTF_Init {} window:{} renderer:{}", sdlInitRet, ttfInitRet, SDL_GetWindowID(m_window.get()), SDL_GetRendererName(m_renderer.get()));
        int driverNum = SDL_GetNumRenderDrivers();
        for (int i = 0; i < driverNum; i++)
        {
            SPDLOG_INFO(SDL_GetRenderDriver(i));
        }
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

    std::string paint::savePNGBuffer(unique_sdl_surface&& surface)
    {
        auto ostream = SDL_IOFromDynamicMem();
        IMG_SavePNG_IO(surface.get(), ostream, false);
        auto buff = std::string(SDL_GetIOSize(ostream), 0);
        SDL_SeekIO(ostream, 0, SDL_IO_SEEK_SET);
        SDL_ReadIO(ostream, buff.data(), buff.size());
        SDL_CloseIO(ostream);
        SPDLOG_INFO("SAVE {} bytes", buff.size());
        return buff;
    }

    paint& paint::preparePanel()
    {
        SDLSetDrawColor(m_renderer.get(), halfTransparent);
        SDL_RenderClear(m_renderer.get());
        SDLSetDrawColor(m_renderer.get(), white);
        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        SDL_RenderFillRect(m_renderer.get(), nullptr);
        auto texture0 = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(texture0->w / 8 * 5),(float)(texture0->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        auto font = unique_sdl_font(TTF_OpenFont("font/NotoSansSC-Regular.ttf", 12));
        auto lapFont = unique_sdl_font(TTF_CopyFont(font.get()));
        TTF_SetFontStyle(lapFont.get(), TTF_STYLE_BOLD);
        TTF_SetFontHinting(lapFont.get(), TTF_HINTING_LIGHT_SUBPIXEL);
        TTF_SetFontSize(lapFont.get(), 18);
        auto lapText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), lapFont.get(), "周目", 6));
        auto titleFont = unique_sdl_font(TTF_CopyFont(font.get()));
        TTF_SetFontSize(titleFont.get(), 24);
        TTF_SetFontHinting(titleFont.get(), TTF_HINTING_LIGHT_SUBPIXEL);
        auto phaseText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), titleFont.get(), "阶段", 6));
        for (int i = 5; i > 0; i--)
        {
            SDLSetDrawColor(m_renderer.get(), halfTransparent);
            iconRect.y -= (float)(iconRect.h + margin.x * 2);
            SDL_RenderLine(m_renderer.get(), panelRect.x, iconRect.y, panelRect.x + panelRect.w, iconRect.y);
            iconRect.y += (float)margin.x;
            SDL_RenderTexture(m_renderer.get(), texture0.get(), nullptr, &iconRect);
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
        SDLSetDrawColor(m_renderer.get(), transparent);
        SDL_RenderClear(m_renderer.get());
        SDL_SetRenderViewport(m_renderer.get(), nullptr);
        SDL_SetTextureBlendMode(m_panel.get(), SDL_BLENDMODE_NONE);
        SDL_RenderTexture(m_renderer.get(), m_panel.get(), nullptr, nullptr);
        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        auto texture0 = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(texture0->w / 8 * 5),(float)(texture0->h / 8 * 5) };

        for (auto&& id : iconIds | std::views::reverse)
        {
            auto path = std::format("icon/{:06}.webp", id);
            auto texture = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), path.c_str()));
            iconRect.y -= iconRect.h + (float)margin.x;
            SDL_RenderTexture(m_renderer.get(), texture.get(), nullptr, &iconRect);
            iconRect.y -= (float)margin.x;
        }
        
        return *this;
    }

    paint& paint::save()
    {
        SDL_SetRenderViewport(m_renderer.get(), nullptr);
        auto surface = unique_sdl_surface(SDL_RenderReadPixels(m_renderer.get(), nullptr));
        m_panel.reset(SDL_CreateTextureFromSurface(m_renderer.get(), surface.get()));
        return *this;
    }

    paint& paint::refreshBackground(SDL_Color color)
    {
        SDL_SetRenderViewport(m_renderer.get(), nullptr);
        SDLSetDrawColor(m_renderer.get(), color);
        SDL_RenderClear(m_renderer.get());
        SDL_RenderTexture(m_renderer.get(), m_panel.get(), nullptr, nullptr);
        return *this;
    }

    paint& paint::refreshTotalProgress(const std::array<Progress, 2>& progresses)
    {
        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        auto texture0 = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(texture0->w / 8 * 5),(float)(texture0->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        auto phaseRect = SDL_FRect{ iconRect.x, (float)(margin.x), iconRect.w, iconRect.y - margin.x * 2 };
        auto progressRect = SDL_FRect{ HPRect.x, phaseRect.y - margin.x / 5 * 2, HPRect.w, phaseRect.h / 2 };
        SDL_RenderFillRect(m_renderer.get(), &progressRect);
        progressRect.y += margin.x / 5 * 4 + progressRect.h;
        SDL_RenderFillRect(m_renderer.get(), &progressRect);
        return *this;
    }

    paint& paint::refreshBossProgress(const std::array<std::uint64_t, 5>& laps, const std::array<Progress, 5>& progresses)
    {
        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        auto texture0 = unique_sdl_texture(IMG_LoadTexture(m_renderer.get(), "icon/000000.webp"));
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(texture0->w / 8 * 5),(float)(texture0->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        auto lapFont = unique_sdl_font(TTF_OpenFont("font/NotoSansSC-Regular.ttf", 18));
        TTF_SetFontStyle(lapFont.get(), TTF_STYLE_BOLD);
        TTF_SetFontHinting(lapFont.get(), TTF_HINTING_LIGHT_SUBPIXEL);
        auto HPFont = unique_sdl_font(TTF_CopyFont(lapFont.get()));
        TTF_SetFontSize(HPFont.get(), 12);
        auto HPTextMiddleX = HPRect.x + HPRect.w / 2;
        auto HPText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), HPFont.get(), nullptr, 0));
        auto lapText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), lapFont.get(), nullptr, 0));
        for (int i = 4; i >= 0; i--)
        {
            iconRect.y -= (float)(iconRect.h + margin.x);
            HPRect.y = iconRect.y + iconRect.h / 5 * 2;
            SDLSetDrawColor(m_renderer.get(), halfTransparent);
            SDL_RenderFillRect(m_renderer.get(), &HPRect);
            auto HPProgress = HPRect;
            HPProgress.w = HPProgress.w / progresses[i].second * progresses[i].first;
            SDLSetDrawColor(m_renderer.get(), {192,0,0,255});
            SDL_RenderFillRect(m_renderer.get(), &HPProgress);
            auto HPStr = std::format("{}/{}", progresses[i].first, progresses[i].second);
            TTF_SetTextString(HPText.get(), HPStr.c_str(), HPStr.length());
            int w, h;
            TTF_GetTextSize(HPText.get(), &w, &h);
            TTF_DrawRendererText(HPText.get(), HPTextMiddleX - w / 2.0f, HPRect.y);
            iconRect.y -= (float)margin.x;
            auto lapRect = SDL_FRect{ HPRect.x, iconRect.y + margin.x + 4 , 18 * 4,22 };
            SDLSetDrawColor(m_renderer.get(), transparent);
            //SDL_RenderFillRect(m_renderer.get(), &lapRect);
            auto lapStr = std::format("周目{}", laps[i]);
            TTF_SetTextString(lapText.get(), lapStr.c_str(), lapStr.length());
            TTF_DrawRendererText(lapText.get(), HPRect.x + margin.x / 2, iconRect.y + margin.x);
        }
        return *this;
    }

    paint& paint::show()
    {
        SDL_RenderPresent(m_renderer.get());
        return *this;
    }

    void paint::mainLoop()
    {
        SDL_Event e{};
        while (SDL_WaitEvent(&e))
        {
            switch ((PaintEvent)e.type)
            {
                case PaintEvent::DRAW_PROCESS:
                {
                    std::invoke(*(static_cast<std::function<void()>*>(e.user.data1)));
                    SDL_SetRenderViewport(m_renderer.get(), nullptr);
                    auto surface = unique_sdl_surface(SDL_RenderReadPixels(m_renderer.get(), nullptr));
                    static_cast<std::promise<unique_sdl_surface>*>(e.user.data2)->set_value(std::move(surface));
                    break;
                }
                case PaintEvent::MOUSE_BUTTON_UP:
                {
                    break;
                }
                case PaintEvent::QUIT:
                    return;
                default:
                    break;
            }
        }
        SPDLOG_ERROR("{}", SDL_GetError());
    }

    bool paint::postDrawProcess(std::function<void()>& process, std::promise<unique_sdl_surface>& promise)
    {
        SDL_Event e{
            .user{
                .type = (std::uint32_t)PaintEvent::DRAW_PROCESS,
                .data1 = &process,
                .data2 = &promise
            },
        };
        return SDL_PushEvent(&e);
    }

    bool paint::postQuit()
    {
        SDL_Event e{ SDL_EVENT_QUIT };
        return SDL_PushEvent(&e);
    }
    
}