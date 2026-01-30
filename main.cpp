#include "yobot_paint.h"
#include "yobot_bossData.h"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <shared_mutex>
#include <fstream>

constexpr auto LogPattern = "%m-%d %H:%M:%S.%e [%^%l%$] [thread:%t] [%s:%#] %v";

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
    SPDLOG_INFO(bossData["boss_id"][yobot::area::cn].dump());
    std::promise<yobot::unique_sdl_surface> drawPromise;
    std::function<void()> drawProcess = [&] {
        yobot::paint::getInstance().preparePanel(bossData["boss_id"][yobot::area::cn]);
    };
    yobot::paint::getInstance().postDrawProcess(drawProcess, drawPromise);
    drawPromise.get_future().wait();
}

inline bool isStatusLegal(const json& statusData)
{
    return statusData.is_object()
        && statusData.contains("lap")
        && statusData["lap"].is_number_integer()
        && statusData.contains("boss_hps")
        && statusData["boss_hps"].is_array()
        && statusData.contains("lap_flags")
        && statusData["lap_flags"].is_array();
}

inline std::string Process(const json& statusData, const json& bossData)
{
    auto lap = statusData["lap"].get<json::number_integer_t>();
    auto phase = yobot::getPhase(bossData, lap, yobot::area::cn);
    auto lapMax = bossData["lap_range"][yobot::area::cn][phase][1].get<json::number_integer_t>();
    const std::array<bool, 5>& lapFlags = statusData["lap_flags"];
    auto&& bossHPs = statusData["boss_hps"];
    auto&& bossFullHPs = bossData["boss_hp"][yobot::area::cn][phase];
    std::array<yobot::Progress, 5> bossProgreses;
    for (size_t i = 0; i < bossProgreses.size(); i++)
    {
        bossProgreses[i] = { bossHPs[i],bossFullHPs[i] };
    }
    auto currentTime = std::chrono::system_clock::now().time_since_epoch().count();
    auto startTime = bossData["time_range"][yobot::area::cn][0].get<json::number_integer_t>();
    auto endTime = bossData["time_range"][yobot::area::cn][1].get<json::number_integer_t>();
    std::array<yobot::Progress, 2> totalProgesses = { {
        {(endTime > currentTime ? endTime - currentTime : 0),endTime - startTime},
        {lap,lapMax}
    } };
    phase += 'A';
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
    SPDLOG_INFO(bossData["boss_id"]["cn"].dump());
    yobot::paint::getInstance()
        .preparePanel(bossData["boss_id"]["cn"])
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
            resp.status = 500;
            auto data = json::parse(req.params.find("data")->second);
            if (isStatusLegal(data))
            {
                resp.body = Process(data, bossData);
                resp.status = 200;
            }
        }).Get("/quit", [](const httplib::Request& req, httplib::Response& resp) {
            yobot::paint::getInstance().postQuit();
        }).listen("127.0.0.1", 8080);
    });
    yobot::paint::getInstance().mainLoop();
    server.stop();
    return 0;
}
