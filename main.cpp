#include "yobot_paint.h"
#include <httplib.h>
#include <fstream>
#include <spdlog/spdlog.h>

int main(int argc, char const *argv[])
{
    yobot::paint::getInstance()
        .preparePanel()
        .save()
        .refreshPanelIcons({ 312501,316600,300701,316102,302600 })
        .save()
        .show();
    httplib::Server server;
    std::jthread httpServer([&]() {
        server.set_logger([](const httplib::Request& req, const httplib::Response& resp) {
            SPDLOG_INFO("[{}] {} status: {} bytes: {}", req.method, req.path, resp.status, resp.body.size());
        }).Get("/red", [](const httplib::Request& req, httplib::Response& resp) {
            std::promise<yobot::unique_sdl_surface> drawPromise;
            auto drawFuture = drawPromise.get_future();
            std::function<void()> drawProcess = [] {
                yobot::paint::getInstance().refreshBackground({ 192,0,0,255 }).show();
            };
            yobot::paint::getInstance().postDrawProcess(drawProcess, drawPromise);
            resp.body = yobot::paint::savePNGBuffer(drawFuture.get());
        }).listen("127.0.0.1", 8080);
    });
    yobot::paint::getInstance().mainLoop();
    server.stop();
    return 0;
}
