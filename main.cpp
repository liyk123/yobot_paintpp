#include "yobot_paint.h"
#include <httplib.h>
#include <spdlog/spdlog.h>

constexpr auto logPattern = "%m-%d %H:%M:%S.%e [%^%l%$] [thread:%t] [%s:%#] %v";

inline void InitEnv()
{
    spdlog::default_logger()->set_pattern(logPattern);
}

int main(int argc, char const *argv[])
{
    InitEnv();
    yobot::paint::getInstance()
        .preparePanel({ 312501,316600,300701,316102,302600 })
        .show();
    httplib::Server server;
    std::jthread httpServer([&]() {
        server.set_logger([](const httplib::Request& req, const httplib::Response& resp) {
            SPDLOG_INFO("[{}] {} status: {} bytes: {}", req.method, req.path, resp.status, resp.body.size());
        }).Get("/update", [](const httplib::Request& req, httplib::Response& resp) {

        }).Get("/progress", [](const httplib::Request& req, httplib::Response& resp) {
            if (req.params.contains("data"))
            {
                auto data = req.params.find("data")->second;
                SPDLOG_INFO("{}", data);
                std::promise<yobot::unique_sdl_surface> drawPromise;
                std::function<void()> drawProcess = [] {
                    SPDLOG_INFO("Begin");
                    yobot::paint::getInstance()
                        .refreshBackground('B')
                        .refreshTotalProgress('B', { {{4,5},{1,9}} })
                        .refreshBossProgress(1,{ false,false,false,false,false }, { {{1e9,1e9},{5e8,1e9},{1e9,1e9},{1e9,1e9},{1e9,1e9}} })
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
