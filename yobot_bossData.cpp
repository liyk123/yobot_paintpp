#include <fstream>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <tbb/tbb.h>
#include <spdlog/spdlog.h>
#include "yobot_bossData.h"
#include <chrono>

constexpr auto IconDir = "icon";

namespace yobot {
    using BossData = std::tuple<std::string_view, json::array_t, json::array_t, json::array_t, json::array_t, json::array_t>;

    template<typename T, typename... Args>
    static auto& GetThreadBuffers(Args&&... args)
    {
        static tbb::enumerable_thread_specific<T> buffers([...args = std::forward<Args>(args)]() {
            return T(args...);
        });
        return buffers;
    }

    template<std::size_t N>
    static auto& GetLimitedArena()
    {
        static tbb::task_arena arena(N);
        return arena;
    }

    inline std::int64_t toSeconds(const json &t)
    {
        std::chrono::sys_time<std::chrono::seconds> ret;
        std::istringstream(t.get<std::string>())
            >> std::chrono::parse("%FT%T%Ez", ret);
        return ret.time_since_epoch().count();
    }

    inline void fetchBossData(BossData& bossData, tbb::concurrent_unordered_set<json::number_integer_t>& idSet)
    {
        auto&& [itArea, itBossHP, itLapRange, itBossId, itBossName, itTimeRange] = bossData;
        httplib::Client client("https://pcr.satroki.tech");
        client.set_follow_location(true);
        auto&& buff = GetThreadBuffers<std::string>().local();
        auto result = client.Get("/api/Quest/GetClanBattleInfos?s=" + std::string(itArea), [&buff](const char* data, size_t data_length) {
            buff.append(data, data_length);
            return true;
        });
        if (result && result->status == httplib::OK_200)
        {
            auto clanBattleInfo = json::parse(std::string_view(buff));
            auto& lastInfo = *(clanBattleInfo.rbegin());
            auto& phases = lastInfo["phases"];
            if (phases.is_array())
            {
                auto ait = phases.begin();
                for (auto&& boss : (*ait)["bosses"])
                {
                    auto id = boss["unitId"].get<json::number_integer_t>();
                    idSet.insert(id);
                    itBossId.emplace_back(id);
                    itBossName.emplace_back(boss["name"]);
                }
                for (; ait != phases.end(); ait++)
                {
                    json::array_t bossHP;
                    for (auto&& boss : (*ait)["bosses"])
                    {
                        bossHP.emplace_back(boss["hp"]);
                    }
                    itBossHP.emplace_back(std::move(bossHP));
                    itLapRange.emplace_back(json::array({ (*ait)["lapFrom"], (*ait)["lapTo"] }));
                }
                *(itLapRange.rbegin()->rbegin()) = 999;
                itTimeRange = { toSeconds(lastInfo["startTime"]), toSeconds(lastInfo["endTime"]) };
            }
        }
        buff.clear();
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
                auto ofs = std::ofstream(filepath, std::ios::binary);
                auto result = client.Get("/icon/unit/" + filename, [&ofs](const char* data, size_t data_length) {
                    ofs.write(data, data_length);
                    return true;
                });
                if (result && result->status == httplib::OK_200)
                {
                    SPDLOG_INFO("{} {}", filepath.generic_string(), (std::size_t)ofs.tellp());
                }
            }
        }
    }

    void updateBossData(json &bossData)
    {
        std::array<yobot::BossData, 3> vBossData = { {
            {area::cn, {}, {}, {}, {}, {}},
            {area::tw, {}, {}, {}, {}, {}},
            {area::jp, {}, {}, {}, {}, {}}
        } };
        tbb::concurrent_unordered_set<json::number_integer_t> idSet;
        GetLimitedArena<vBossData.size()>().execute([&] {
            tbb::parallel_for(std::size_t(0), vBossData.size(), [&](std::size_t it) {
                fetchBossData(vBossData[it], idSet);
            });
            tbb::parallel_for(idSet.range(), [](auto&& range) {
                fetchBossIcon(range);
            });
        });
        for (auto&& x : vBossData)
        {
            auto&& [a, b, c, d, e, f] = x;
            auto&& timeRange = bossData["time_range"][a];
            if (!bossData.is_null() && !timeRange.is_null() && *timeRange.begin() == *f.begin())
            {
                continue;
            }
            bossData["boss_hp"][a] = b;
            bossData["lap_range"][a] = c;
            bossData["boss_id"][a] = d;
            bossData["boss_name"][a] = e;
            timeRange = f;
        }
    }

    std::int8_t getPhase(const json& bossData, const std::int64_t lap, const std::string_view& gameServer)
    {
        char ret = 0;
        auto& phaseList = bossData["lap_range"][gameServer].get_ref<const json::array_t&>();
        for (auto&& range : phaseList)
        {
            if (lap >= range[0] && lap <= range[1])
            {
                break;
            }
            ret++;
        }
        return ret;
    }
}