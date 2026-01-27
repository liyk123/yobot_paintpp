#pragma once
#include <nlohmann/json.hpp>

using nlohmann::json;
using nlohmann::ordered_json;

namespace yobot {
    constexpr auto IconDir = "icon";
    namespace area {
        constexpr std::string_view cn = "cn";
        constexpr std::string_view tw = "tw";
        constexpr std::string_view jp = "jp";
    }

    ordered_json updateBossData();
}