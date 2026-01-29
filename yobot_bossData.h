#pragma once
#include <nlohmann/json.hpp>

using nlohmann::json;
using nlohmann::ordered_json;

namespace yobot {
    namespace area {
        constexpr std::string_view cn = "cn";
        constexpr std::string_view tw = "tw";
        constexpr std::string_view jp = "jp";
    }

    json updateBossData();

    std::int8_t getPhase(const json& bossData, const std::int64_t lap, const std::string_view& gameServer);
}
