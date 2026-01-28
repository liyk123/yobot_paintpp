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

int main(int argc, char const *argv[])
{
    InitEnv();
    ordered_json bossData = yobot::updateBossData();
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
            bossData = yobot::updateBossData();
        }).Get("/progress", [&](const httplib::Request& req, httplib::Response& resp) {
            std::shared_lock lock(mtBossData);
            if (req.params.contains("data"))
            {
                auto&& data = req.params.find("data")->second;
                SPDLOG_INFO("{}", data);
                std::promise<yobot::unique_sdl_surface> drawPromise;
                std::function<void()> drawProcess = [] {
                    SPDLOG_INFO("Begin");
                    yobot::paint::getInstance()
                        .refreshBackground('B')
                        .refreshTotalProgress('B', { {{4,5},{0,30}} })
                        .refreshBossProgress(
                            1,
                            { false,true,false,true,false }, 
                            { 
                                {
                                    {(std::uint64_t)1e9,(std::uint64_t)1e9},
                                    {(std::uint64_t)5e8,(std::uint64_t)1e9},
                                    {(std::uint64_t)1e9,(std::uint64_t)1e9},
                                    {(std::uint64_t)1e9,(std::uint64_t)1e9},
                                    {(std::uint64_t)1e9,(std::uint64_t)1e9}
                                } 
                            }
                        )
                        ;
                    SPDLOG_INFO("End");
                };
                yobot::paint::getInstance().postDrawProcess(drawProcess, drawPromise);
                resp.body = yobot::paint::savePNGBuffer(drawPromise.get_future().get());
            }
        }).Get("/quit", [](const httplib::Request& req, httplib::Response& resp) {
            yobot::paint::getInstance().postQuit();
            resp.status = 200;
        }).listen("127.0.0.1", 8080);
    });
    yobot::paint::getInstance().mainLoop();
    server.stop();
    return 0;
}
