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

    inline bool SDLSetDrawColor(SDL_Renderer* renderer, const SDL_Color& color) noexcept
    {
        return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    inline bool SDLSetTextColor(TTF_Text* text, const SDL_Color& color) noexcept
    {
        return TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
    }

    inline SDL_FPoint GetCenterPos(TTF_Text* text, const SDL_FRect& rect)
    {
        int w, h;
        TTF_GetTextSize(text, &w, &h);
        return { rect.x + rect.w / 2 - w / 2.0f,rect.y + rect.h / 2 - h / 2.0f };
    }

    inline SDL_FPoint GetLeftCenterPos(TTF_Text* text, const SDL_FRect& rect)
    {
        int w, h;
        TTF_GetTextSize(text, &w, &h);
        return { rect.x, rect.y + rect.h / 2 - h / 2.0f };
    }

    inline auto toOKFAILED(bool flag)
    {
        return flag ? "\033[1;32mOK\033[0m" : "\033[1;31mFAILED\033[0m";
    }

    constexpr auto windowSize = SDL_Point{ 480,640 };
    constexpr auto white = SDL_Color{ 255,255,255,128 };
    constexpr auto halfTransparent = SDL_Color{ 0,0,0,128 };
    constexpr auto transparent = SDL_Color{ 0,0,0,0 };
    constexpr auto black = SDL_Color{ 0,0,0,255 };
    constexpr auto margin = SDL_Point{ 10,30 };
    constexpr auto clipRect = SDL_Rect{ margin.x, margin.y, windowSize.x - margin.x * 2, windowSize.y - margin.y * 2 };
    constexpr auto panelRect = SDL_FRect{ 0.0f,0.0f,(float)clipRect.w,(float)clipRect.h };
    constexpr auto red = SDL_Color{ 192,0,0,255 };
    constexpr auto blue = SDL_Color{ 0,153,255,255 };
    constexpr SDL_Color lapColor[] = { {228, 94, 104, 255}, {106, 152, 243, 255} };
    constexpr SDL_Color phaseColor[] = { 
        { 132, 1, 244, 255 }, 
        { 115, 166, 231, 255 }, 
        { 206, 105, 165, 255 }, 
        { 206, 80, 66, 255 }, 
        { 181, 105, 206, 255 }
    };

    paint::paint()
        : m_window(nullptr)
        , m_windowSurafce(nullptr)
        , m_renderer(nullptr)
        , m_textEngine(nullptr)
        , m_panel(nullptr)
        , m_texture0(nullptr)
        , m_titleFont(nullptr)
        , m_lapFont(nullptr)
        , m_hpFont(nullptr)
    {
        auto sdlInitRet = SDL_Init(SDL_INIT_VIDEO);
        auto ttfInitRet = TTF_Init();
        m_window.reset(SDL_CreateWindow(PROJECT_NAME, windowSize.x, windowSize.y, SDL_WINDOW_HIDDEN | SDL_WINDOW_TRANSPARENT));
        if (m_window)
        {
            m_renderer.reset(SDL_CreateRenderer(m_window.get(), nullptr));
        }
        if (!m_renderer || std::string_view(SDL_GetRendererName(m_renderer.get())) == std::string_view("software"))
        {
            m_window = nullptr;
            m_windowSurafce.reset(SDL_CreateSurface(windowSize.x, windowSize.y, SDL_PIXELFORMAT_ARGB8888));
            m_renderer.reset(SDL_CreateSoftwareRenderer(m_windowSurafce.get()));
        }
        m_textEngine.reset(TTF_CreateRendererTextEngine(m_renderer.get()));
        SPDLOG_INFO("SDL_Init:{} TTF_Init:{} window:{} renderer:{}", toOKFAILED(sdlInitRet), toOKFAILED(ttfInitRet), SDL_GetWindowID(m_window.get()), SDL_GetRendererName(m_renderer.get()));
        loadRes();
    }

    paint::~paint()
    {
        m_hpFont = nullptr;
        m_lapFont = nullptr;
        m_titleFont = nullptr;
        m_texture0 = nullptr;
        m_panel = nullptr;
        m_textEngine = nullptr;
        m_renderer = nullptr;
        m_windowSurafce = nullptr;
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
        auto props = SDL_GetIOProperties(ostream);
        auto internal_ptr = (char*)SDL_GetPointerProperty(props, SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, nullptr);
        auto buff = std::string(internal_ptr, SDL_GetIOSize(ostream));
        SDL_CloseIO(ostream);
        return buff;
    }

    void paint::loadRes()
    {
        m_texture0.reset(IMG_LoadTexture(m_renderer.get(), DefaultIconPath.data));
        auto font = unique_sdl_font(TTF_OpenFont(DefaultFontPath.data, 12));
        TTF_SetFontHinting(font.get(), TTF_HINTING_LIGHT_SUBPIXEL);
        m_titleFont.reset(TTF_CopyFont(font.get()));
        TTF_SetFontSize(m_titleFont.get(), 20);
        TTF_SetFontWrapAlignment(m_titleFont.get(), TTF_HORIZONTAL_ALIGN_CENTER);
        m_lapFont.reset(TTF_CopyFont(font.get()));
        TTF_SetFontSize(m_lapFont.get(), 18);
        TTF_SetFontStyle(m_lapFont.get(), TTF_STYLE_BOLD);
        m_hpFont.reset(TTF_CopyFont(font.get()));
        TTF_SetFontStyle(m_hpFont.get(), TTF_STYLE_BOLD);
    }

    inline unique_sdl_surface SaveSurface(SDL_Renderer* renderer)
    {
        SDL_SetRenderViewport(renderer, nullptr);
        return unique_sdl_surface(SDL_RenderReadPixels(renderer, nullptr));
    }

    inline void ClearPanel(SDL_Renderer* renderer)
    {
        SDLSetDrawColor(renderer, halfTransparent);
        SDL_RenderClear(renderer);
        SDLSetDrawColor(renderer, white);
        SDL_SetRenderViewport(renderer, &clipRect);
        SDL_RenderFillRect(renderer, nullptr);
    }

    inline void RenderPanelRow(SDL_Renderer* renderer, SDL_FRect& iconRect, const uint64_t& id, SDL_FRect& HPRect)
    {
        SDLSetDrawColor(renderer, halfTransparent);
        iconRect.y -= (float)(iconRect.h + margin.x * 2);
        SDL_RenderLine(renderer, panelRect.x, iconRect.y, panelRect.x + panelRect.w, iconRect.y);
        iconRect.y += (float)margin.x;
        auto path = std::format("icon/{:06}.webp", id);
        auto texture = unique_sdl_texture(IMG_LoadTexture(renderer, path.c_str()));
        SDL_RenderTexture(renderer, texture.get(), nullptr, &iconRect);
        HPRect.y = iconRect.y + iconRect.h / 5 * 2;
        SDL_RenderFillRect(renderer, &HPRect);
        iconRect.y -= (float)margin.x;
        auto lapRect = SDL_FRect{ HPRect.x, iconRect.y + margin.x + 4 , 18 * 4,22 };
        SDLSetDrawColor(renderer, transparent);
        SDL_RenderFillRect(renderer, &lapRect);
    }

    inline void RenderPanelHeader(SDL_Renderer* renderer, TTF_TextEngine* textEngine, const SDL_FRect& iconRect, const SDL_FRect& HPRect)
    {
        auto phaseRect = SDL_FRect{ iconRect.x, (float)(margin.x), iconRect.w, iconRect.y - margin.x * 2 };
        SDLSetDrawColor(renderer, transparent);
        SDL_RenderFillRect(renderer, &phaseRect);
        auto progressRect = SDL_FRect{ HPRect.x, phaseRect.y - margin.x / 5 * 2, HPRect.w, phaseRect.h / 2 };
        SDL_RenderFillRect(renderer, &progressRect);
        progressRect.y += margin.x / 5 * 4 + progressRect.h;
        SDL_RenderFillRect(renderer, &progressRect);
    }

    paint& paint::preparePanel(const std::array<std::uint64_t, 5>& iconIds)
    {
        ClearPanel(m_renderer.get());
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(m_texture0->w / 8 * 5),(float)(m_texture0->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        for (auto&& id : iconIds | std::views::reverse)
        {
            RenderPanelRow(m_renderer.get(), iconRect, id, HPRect);
        }
        RenderPanelHeader(m_renderer.get(), m_textEngine.get(), iconRect, HPRect);
        m_panel.reset(SDL_CreateTextureFromSurface(m_renderer.get(), SaveSurface(m_renderer.get()).get()));
        return *this;
    }

    paint& paint::refreshBackground(const char phase)
    {
        SDL_SetRenderViewport(m_renderer.get(), nullptr);
        SDLSetDrawColor(m_renderer.get(), phaseColor[phase - 'A']);
        SDL_RenderClear(m_renderer.get());
        SDL_RenderTexture(m_renderer.get(), m_panel.get(), nullptr, nullptr);
        return *this;
    }

    inline std::string getCountDownStr(std::uint64_t t)
    {
        auto sec = std::chrono::seconds(t);
        if (auto d = std::chrono::floor<std::chrono::days>(sec); d.count() != 0)
        {
            return std::format("{}天", d.count());
        }
        if (auto h = std::chrono::floor<std::chrono::hours>(sec); h.count() != 0)
        {
            return std::format("{}小时", h.count());
        }
        if (auto m = std::chrono::floor<std::chrono::minutes>(sec); m.count() != 0)
        {
            return std::format("{}分钟", m.count());
        }
        return std::format("{}秒", std::chrono::floor<std::chrono::seconds>(sec).count());
    }

    paint& paint::refreshTotalProgress(const char phase, const std::array<Progress, 2>& progresses)
    {
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(m_texture0->w / 8 * 5),(float)(m_texture0->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        auto phaseRect = SDL_FRect{ iconRect.x, (float)(margin.x), iconRect.w, iconRect.y - margin.x * 12 - iconRect.h * 5};
        auto scheduleStr = "距离会战结束还剩" + getCountDownStr(progresses[0].first);
        auto lapRangeStr = progresses[1].first == 0 ? "∞" : std::format("{}/{}", progresses[1].first, progresses[1].second);
        auto phaseStr = std::format("阶段\n{}", phase);
        auto phaseText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), m_titleFont.get(), phaseStr.c_str(), phaseStr.length()));
        std::array<unique_sdl_text, 2> texts{
            unique_sdl_text(TTF_CreateText(m_textEngine.get(), m_titleFont.get(), scheduleStr.c_str(), scheduleStr.length())),
            unique_sdl_text(TTF_CreateText(m_textEngine.get(), m_titleFont.get(), lapRangeStr.c_str(), lapRangeStr.length()))
        };
        std::array<SDL_FPoint, 2> textPositions;
        std::array<SDL_FRect,2> rects;
        for (std::size_t i = 0; i < rects.size(); i++)
        {
            rects[i].w = HPRect.w * (progresses[i].second - progresses[i].first) / progresses[i].second;
            rects[i].x = HPRect.x + HPRect.w - rects[i].w;
            rects[i].h = phaseRect.h / 2;
            rects[i].y = phaseRect.y - margin.x / 5 * 2 + (margin.x / 5 * 4 + rects[i].h) * i;
            textPositions[i] = GetCenterPos(texts[i].get(), { HPRect.x,rects[i].y,HPRect.w,rects[i].h });
        }

        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        SDLSetDrawColor(m_renderer.get(), halfTransparent);
        SDL_RenderFillRects(m_renderer.get(), rects.data(), (int)rects.size());
        auto phasePos = GetCenterPos(phaseText.get(), phaseRect);
        TTF_DrawRendererText(phaseText.get(), phasePos.x, phasePos.y);
        for (std::size_t i = 0; i < texts.size(); i++)
        {
            TTF_DrawRendererText(texts[i].get(), textPositions[i].x, textPositions[i].y);
        }
        return *this;
    }

    paint& paint::refreshBossProgress(const std::uint64_t lap, const std::array<bool, 5>& lapFlags, const std::array<Progress, 5>& progresses)
    {
        SDL_SetRenderViewport(m_renderer.get(), &clipRect);
        auto iconRect = SDL_FRect{ (float)margin.x,panelRect.h,(float)(m_texture0->w / 8 * 5),(float)(m_texture0->h / 8 * 5) };
        auto HPRect = SDL_FRect{ margin.x * 3 + iconRect.w,0.0f,panelRect.w - iconRect.w - (float)(margin.x * 4),iconRect.h / 4 };
        auto HPTextMiddleX = HPRect.x + HPRect.w / 2;
        auto HPText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), m_hpFont.get(), nullptr, 0));
        auto lapText = unique_sdl_text(TTF_CreateText(m_textEngine.get(), m_lapFont.get(), nullptr, 0));
        for (int i = 4; i >= 0; i--)
        {
            iconRect.y -= (float)(iconRect.h + margin.x);
            HPRect.y = iconRect.y + iconRect.h / 5 * 2;
            SDLSetDrawColor(m_renderer.get(), halfTransparent);
            SDL_RenderFillRect(m_renderer.get(), &HPRect);
            auto HPProgress = HPRect;
            HPProgress.w = HPProgress.w / progresses[i].second * progresses[i].first;
            HPProgress.w = HPProgress.w < 1.0f && HPProgress.w > 0 ? 1.0f : HPProgress.w;
            SDLSetDrawColor(m_renderer.get(), red);
            SDL_RenderFillRect(m_renderer.get(), &HPProgress);
            auto HPStr = std::format("{}/{}", progresses[i].first, progresses[i].second);
            TTF_SetTextString(HPText.get(), HPStr.c_str(), HPStr.length());
            auto pos = GetCenterPos(HPText.get(), HPRect);
            TTF_DrawRendererText(HPText.get(), pos.x, pos.y);
            iconRect.y -= (float)margin.x;
            auto lapRect = SDL_FRect{ HPRect.x, iconRect.y + margin.x + 4 , 18 * 4,22 };
            SDLSetDrawColor(m_renderer.get(), lapColor[lapFlags[i]]);
            SDL_RenderFillRect(m_renderer.get(), &lapRect);
            auto lapStr = std::format("周目{}", lap + lapFlags[i]);
            TTF_SetTextString(lapText.get(), lapStr.c_str(), lapStr.length());
            auto lapPos = GetLeftCenterPos(lapText.get(), lapRect);
            TTF_DrawRendererText(lapText.get(), lapPos.x + margin.x / 2, lapPos.y);
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
                    static_cast<std::promise<unique_sdl_surface>*>(e.user.data2)->set_value(SaveSurface(m_renderer.get()));
                    SDL_RenderPresent(m_renderer.get());
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