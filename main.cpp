#include "yobot_paint.h"
#include "yobot_bossData.h"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <shared_mutex>
#include <fstream>

constexpr auto LogPattern = "%m-%d %H:%M:%S.%e [%^%l%$] [thread:%t] [%s:%#] %v";
constexpr auto DefaultArea = yobot::area::cn;
constexpr auto DefaultHost = "0.0.0.0";
constexpr auto DefaultPort = 9540;

inline bool DownloadBinaryFile(const std::string_view& host, const std::string_view& getPath, const std::string_view& savePath)
{
    httplib::Client client(host.data());
    client.set_follow_location(true);
    auto result = client.Get(getPath.data());
    auto ret = result && result->status == httplib::OK_200;
    if (ret)
    {
        SPDLOG_INFO("{} {}", savePath, result->body.size());
        std::ofstream(savePath.data(), std::ios::binary) << result->body;
    }
    return ret;
}

inline void InitEnv()
{
    spdlog::default_logger()->set_pattern(LogPattern);
    std::filesystem::create_directory(IconDir);
    if (!std::filesystem::exists(DefaultIconPath.data))
    {
        constexpr auto getPath = FixedString("/icon/unit/") + FixedString(DefaultIcon);
        DownloadBinaryFile("https://redive.estertion.win", getPath.data, DefaultIconPath.data);
    }
    std::filesystem::create_directory(FontDir);
    if (!std::filesystem::exists(DefaultFontPath.data))
    {
        constexpr auto getPath = FixedString("/jsntn/webfonts/raw/refs/heads/master/") + FixedString(DefaultFont);
        DownloadBinaryFile("https://github.com", getPath.data, DefaultFontPath.data);
    }
}

inline void Update(json& bossData)
{
    bossData = yobot::updateBossData();
    SPDLOG_INFO(bossData["boss_id"][DefaultArea].dump());
    std::promise<yobot::unique_sdl_surface> drawPromise;
    std::function<void()> drawProcess = [&] {
        yobot::paint::getInstance().preparePanel(bossData["boss_id"][DefaultArea]);
    };
    yobot::paint::getInstance().postDrawProcess(drawProcess, drawPromise);
    drawPromise.get_future().wait();
}

inline auto PrepareRenderData(const json& statusData, const json& bossData)
{
    auto lap = statusData.at("lap").get<json::number_integer_t>();
    auto phase = yobot::getPhase(bossData, lap, DefaultArea);
    auto&& lapRange = bossData["lap_range"][DefaultArea][phase];
    auto lapMin = lapRange[0].get<json::number_integer_t>();
    auto lapMax = lapRange[1].get<json::number_integer_t>();
    const std::array<bool, 5> &lapFlags = statusData.at("lap_flags");
    auto&& bossHPs = statusData.at("boss_hps");
    auto&& bossFullHPs = bossData["boss_hp"][DefaultArea][phase];
    std::array<yobot::Progress, 5> bossProgreses;
    for (size_t i = 0; i < bossProgreses.size(); i++)
    {
        bossProgreses[i] = { bossHPs.at(i),bossFullHPs[i] };
    }
    auto currentTime = std::chrono::system_clock::now().time_since_epoch().count();
    auto startTime = bossData["time_range"][DefaultArea][0].get<json::number_integer_t>();
    auto endTime = bossData["time_range"][DefaultArea][1].get<json::number_integer_t>();
    std::array<yobot::Progress, 2> totalProgesses = { {
        {(endTime > currentTime ? endTime - currentTime : 0),endTime - startTime},
        {(lapMax == 999 ? 0 : lapMax - lap + 1),lapMax - lapMin + 1}
    } };
    phase += 'A';
    return std::make_tuple(lap, lapFlags, phase, totalProgesses, bossProgreses);
}

inline std::string Progress(const json& statusData, const json& bossData)
{
    auto&& [lap, lapFlags, phase, totalProgesses, bossProgreses] = PrepareRenderData(statusData, bossData);
    std::promise<yobot::unique_sdl_surface> drawPromise;
    std::function<void()> drawProcess = [&] {
        yobot::paint::getInstance()
            .refreshBackground(phase)
            .refreshTotalProgress(phase, totalProgesses)
            .refreshBossProgress(lap, lapFlags, bossProgreses);
    };
    yobot::paint::getInstance().postDrawProcess(drawProcess, drawPromise);
    return yobot::paint::savePNGBuffer(drawPromise.get_future().get());
}

int main(int argc, char const *argv[])
{
    InitEnv();
    json bossData = yobot::updateBossData();
    std::shared_mutex mtBossData;
    SPDLOG_INFO(bossData["boss_id"][DefaultArea].dump());
    yobot::paint::getInstance()
        .preparePanel(bossData["boss_id"][DefaultArea])
        .show();
    httplib::Server server;
    std::jthread httpServer([&]() {
        server.set_logger([](const httplib::Request& req, const httplib::Response& resp) {
            SPDLOG_INFO("[{}] {} {} status: {} bytes: {}", req.method, req.path, json(req.params).dump(), resp.status, resp.body.size());
        }).Get("/update", [&](const httplib::Request& req, httplib::Response& resp) {
            std::unique_lock lock(mtBossData);
            Update(bossData);
        }).Get("/progress", [&](const httplib::Request& req, httplib::Response& resp) {
            std::shared_lock lock(mtBossData);
            auto data = json::parse(req.params.find("data")->second);
            resp.body = Progress(data, bossData);
        }).Get("/quit", [](const httplib::Request& req, httplib::Response& resp) {
            yobot::paint::getInstance().postQuit();
        }).listen(DefaultHost, DefaultPort);
    });
    yobot::paint::getInstance().mainLoop();
    server.stop();
    return 0;
}
