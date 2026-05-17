#pragma once
#include "ModuleConfig.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace cmangos_module
{
    class AutoscaleModuleConfig : public ModuleConfig
    {
    public:
        AutoscaleModuleConfig();
        bool OnLoad() override;

        bool enabled;

        // Polling cadence for the rescale-on-player-change pass. Stored in ms
        // to compare directly against the elapsed value OnUpdate provides.
        uint32_t rescanIntervalMs;

        // HP = baseHp * pow(playerCount / baseline, hpExponent), clamped to
        // [minScale, maxScale]. Exponent < 1 dampens the curve so under-staffed
        // groups still face meaningful HP; exponent = 1 is linear.
        float hpExponent;
        float minScale;
        float maxScale;

        // Default baselines by content type. Per-map overrides (mapBaselines)
        // take precedence over these.
        uint32_t baselineDungeon;     // default for IsDungeon() && !IsRaid()
        uint32_t baselineRaidDefault; // default for IsRaid()

        // mapId → designed party/raid size. Override here for non-default
        // raid sizes (e.g. Zul'Gurub is 20-man, not 40).
        std::unordered_map<uint32_t, uint32_t> mapBaselines;

        // Map IDs that should never be scaled (e.g. instances with scripted
        // HP-locked boss phases the scaling would corrupt).
        std::vector<uint32_t> mapBlacklist;

        // If true, broadcast a chat message to all players in an instance
        // when the scaling factor changes.
        bool announce;
    };
}
