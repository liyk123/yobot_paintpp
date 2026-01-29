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

void inline Update(json& bossData)
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

inline std::string Process(const std::string_view& statusData, const json& bossData)
{
    SPDLOG_INFO("{}", statusData);
    auto data = json::parse(statusData);
    auto lap = data["lap"].get<json::number_integer_t>();
    auto phase = yobot::getPhase(bossData, lap, yobot::area::cn);
    auto lapMax = bossData["lap_range"][yobot::area::cn][phase][1].get<json::number_integer_t>();
    auto&& lapFlags = data["lap_flags"];
    auto&& bossHPs = data["boos_hps"];
    auto&& bossFullHPs = bossData["boss_hp"][yobot::area::cn][phase];
    std::array<yobot::Progress, 5> bossProgreses;
    for (size_t i = 0; i < bossProgreses.size(); i++)
    {
        bossProgreses[i] = { bossHPs[i],bossFullHPs[i] };
    }
    phase += 'A';
    std::promise<yobot::unique_sdl_surface> drawPromise;
    std::function<void()> drawProcess = [&] {
        yobot::paint::getInstance()
            .refreshBackground(phase)
            .refreshTotalProgress(phase, { {{1,1},{lap,lapMax} } })
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
            SPDLOG_INFO("[{}] {} status: {} bytes: {}", req.method, req.path, resp.status, resp.body.size());
        }).Get("/update", [&](const httplib::Request& req, httplib::Response& resp) {
            std::unique_lock lock(mtBossData);
            Update(bossData);
            resp.status = httplib::OK_200;
        }).Get("/progress", [&](const httplib::Request& req, httplib::Response& resp) {
            std::shared_lock lock(mtBossData);
            if (req.params.contains("data"))
            {
                resp.body = Process(req.params.find("data")->second, bossData);
                resp.status = httplib::OK_200;
            }
        }).Get("/quit", [](const httplib::Request& req, httplib::Response& resp) {
            yobot::paint::getInstance().postQuit();
            resp.status = httplib::OK_200;
        }).listen("127.0.0.1", 8080);
    });
    yobot::paint::getInstance().mainLoop();
    server.stop();
    return 0;
}
