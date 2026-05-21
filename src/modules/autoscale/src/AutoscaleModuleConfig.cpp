#include "AutoscaleModuleConfig.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace cmangos_module
{
    AutoscaleModuleConfig::AutoscaleModuleConfig()
    : ModuleConfig("autoscale.conf")
    , enabled(false)
    , rescanIntervalMs(10000)
    , hpExponent(0.85f)
    , minScale(0.20f)
    , maxScale(1.0f)
    , dmgExponent(0.85f)
    , minDmgScale(0.20f)
    , maxDmgScale(1.0f)
    , baselineDungeon(5)
    , baselineRaidDefault(40)
    , announce(true)
    {
    }

    // Parse "key:value,key:value,..." into the given map. Whitespace ignored.
    // Invalid entries are skipped silently.
    static void ParseMapBaselines(const std::string& raw,
                                  std::unordered_map<uint32_t, uint32_t>& out)
    {
        out.clear();
        std::stringstream ss(raw);
        std::string token;
        while (std::getline(ss, token, ','))
        {
            auto colon = token.find(':');
            if (colon == std::string::npos)
                continue;
            try
            {
                std::string keyStr = token.substr(0, colon);
                std::string valStr = token.substr(colon + 1);
                keyStr.erase(std::remove_if(keyStr.begin(), keyStr.end(), ::isspace), keyStr.end());
                valStr.erase(std::remove_if(valStr.begin(), valStr.end(), ::isspace), valStr.end());
                if (keyStr.empty() || valStr.empty())
                    continue;
                uint32_t mapId = static_cast<uint32_t>(std::stoul(keyStr));
                uint32_t size  = static_cast<uint32_t>(std::stoul(valStr));
                if (mapId && size)
                    out[mapId] = size;
            }
            catch (...) {}
        }
    }

    static void ParseIdCsv(const std::string& raw, std::vector<uint32_t>& out)
    {
        out.clear();
        std::stringstream ss(raw);
        std::string token;
        while (std::getline(ss, token, ','))
        {
            token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
            if (token.empty())
                continue;
            try
            {
                uint32_t id = static_cast<uint32_t>(std::stoul(token));
                if (id)
                    out.push_back(id);
            }
            catch (...) {}
        }
    }

    bool AutoscaleModuleConfig::OnLoad()
    {
        enabled              = config.GetBoolDefault ("Autoscale.Enable",                false);
        rescanIntervalMs     = config.GetIntDefault  ("Autoscale.RescanIntervalSeconds", 10) * 1000;
        hpExponent           = config.GetFloatDefault("Autoscale.HpExponent",            0.85f);
        minScale             = config.GetFloatDefault("Autoscale.MinScale",              0.20f);
        maxScale             = config.GetFloatDefault("Autoscale.MaxScale",              1.0f);
        dmgExponent          = config.GetFloatDefault("Autoscale.DmgExponent",           0.85f);
        minDmgScale          = config.GetFloatDefault("Autoscale.MinDmgScale",           0.20f);
        maxDmgScale          = config.GetFloatDefault("Autoscale.MaxDmgScale",           1.0f);
        baselineDungeon      = config.GetIntDefault  ("Autoscale.Baseline.Dungeon",      5);
        baselineRaidDefault  = config.GetIntDefault  ("Autoscale.Baseline.RaidDefault", 40);
        announce             = config.GetBoolDefault ("Autoscale.Announce",              true);

        // Per-map overrides. Defaults cover the vanilla 20-man raids whose
        // baseline differs from the raid-default of 40.
        //   309 Zul'Gurub          (20-man)
        //   509 Ruins of Ahn'Qiraj (20-man, AQ20)
        std::string baselinesRaw = config.GetStringDefault(
            "Autoscale.MapBaselines", "309:20,509:20");
        ParseMapBaselines(baselinesRaw, mapBaselines);

        std::string blacklistRaw = config.GetStringDefault("Autoscale.MapBlacklist", "");
        ParseIdCsv(blacklistRaw, mapBlacklist);

        // Sanity clamps so a typo in the conf can't produce a degenerate result.
        if (hpExponent < 0.0f) hpExponent = 0.0f;
        if (hpExponent > 4.0f) hpExponent = 4.0f;
        if (minScale   < 0.0f) minScale   = 0.0f;
        if (maxScale   < minScale) maxScale = minScale;
        if (dmgExponent < 0.0f) dmgExponent = 0.0f;
        if (dmgExponent > 4.0f) dmgExponent = 4.0f;
        if (minDmgScale < 0.0f) minDmgScale = 0.0f;
        if (maxDmgScale < minDmgScale) maxDmgScale = minDmgScale;
        if (rescanIntervalMs < 1000) rescanIntervalMs = 1000;
        if (baselineDungeon == 0)     baselineDungeon = 5;
        if (baselineRaidDefault == 0) baselineRaidDefault = 40;

        return true;
    }
}
