#include <fstream>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <tbb/tbb.h>
#include <spdlog/spdlog.h>
#include "yobot_bossData.h"

constexpr auto IconDir = "icon";

namespace yobot {
    using BoosData = std::tuple<std::string_view, json::array_t, json::array_t, json::array_t, json::array_t>;

    inline void fetchBossData(BoosData& bossData, tbb::concurrent_unordered_set<json::number_integer_t>& idSet)
    {
        auto&& [itArea, itBossHP, itLapRange, itBossId, itBossName] = bossData;
        httplib::Client client("https://pcr.satroki.tech");
        client.set_follow_location(true);
        auto result = client.Get("/api/Quest/GetClanBattleInfos?s=" + std::string(itArea));
        if (result && result->status == httplib::OK_200)
        {
            auto clanBattleInfo = json::parse(std::string_view(result->body));
            auto& lastInfo = *(clanBattleInfo.rbegin());
            auto& phases = lastInfo["phases"];
            if (phases.is_array())
            {
                auto ait = phases.begin();
                for (auto&& boss : (*ait)["bosses"])
                {
                    auto id = boss["unitId"].get<json::number_integer_t>();
                    idSet.insert(id);
                    itBossId.push_back(id);
                    itBossName.push_back(boss["name"]);
                }
                for (; ait != phases.end(); ait++)
                {
                    json::array_t bossHP;
                    for (auto&& boss : (*ait)["bosses"])
                    {
                        bossHP.push_back(boss["hp"]);
                    }
                    itBossHP.push_back(bossHP);
                    itLapRange.push_back(json::array({ (*ait)["lapFrom"], (*ait)["lapTo"] }));
                }
                *(itLapRange.rbegin()->rbegin()) = 999;
            }
        }
    }

    inline void fetchBossIcon(tbb::concurrent_unordered_set<json::number_integer_t>::range_type range)
    {
        httplib::Client client("https://redive.estertion.win");
        client.set_follow_location(true);
        for (auto&& id : range)
        {
            auto filename = std::to_string(id) + ".webp";
            auto filepath = std::filesystem::path(IconDir) / filename;
            if (!std::filesystem::exists(filepath))
            {
                auto result = client.Get("/icon/unit/" + filename);
                if (result && result->status == httplib::OK_200)
                {
                    SPDLOG_INFO("{} {}", filename, result->body.size());
                    std::ofstream(filepath, std::ios::binary) << result->body;
                }
            }
        }
    }

    ordered_json updateBossData()
    {
        std::vector<yobot::BoosData> vBossData = {
            {area::cn, {}, {}, {}, {}},
            {area::tw, {}, {}, {}, {}},
            {area::jp, {}, {}, {}, {}}
        };
        tbb::concurrent_unordered_set<json::number_integer_t> idSet;
        tbb::parallel_for(0ULL, vBossData.size(), [&](std::size_t it) {
            fetchBossData(vBossData[it], idSet);
        });
        tbb::parallel_for(idSet.range(), [](auto&& range) {
            fetchBossIcon(range);
        });
        ordered_json jbossData;
        for (auto&& x : vBossData)
        {
            auto& [a, b, c, d, e] = x;
            jbossData["boss_hp"][a] = b;
            jbossData["lap_range"][a] = c;
            jbossData["boss_id"][a] = d;
            jbossData["boss_name"][a] = e;
        }
        return jbossData;
    }
}
