#pragma once
#include "Module.h"
#include "AutoscaleModuleConfig.h"

#include <cstdint>
#include <unordered_map>

class Creature;
class Map;

namespace cmangos_module
{
    class AutoscaleModule : public Module
    {
    public:
        AutoscaleModule();
        const AutoscaleModuleConfig* GetConfig() const;
        bool IsEnabled() const;

        // Hooks
        void OnInitialize() override;
        void OnAddToWorld(Creature* creature) override;
        void OnUpdate(uint32 elapsed) override;

    private:
        // Decide whether a given creature is a scale candidate (instance map,
        // not a pet/totem/summon, not blacklisted, etc.).
        bool ShouldScaleCreature(const Creature* creature) const;

        // For a given map, what's the designed party/raid size? Falls back to
        // baselineDungeon / baselineRaidDefault if no per-map override.
        uint32_t ResolveBaseline(const Map* map) const;

        // pow(playerCount / baseline, hpExponent) clamped to [min,max].
        float ComputeScale(uint32_t playerCount, uint32_t baseline) const;

        // Apply hpScale to a creature using its CreatureInfo baseline (so
        // re-scaling is computed against the original spawn HP, not chain-
        // multiplied). Preserves the current-HP fraction.
        // Returns true if scaling was applied.
        bool ScaleCreature(Creature* creature, float hpScale);

        // Per-instance state: last known player count. Keyed by either the
        // instance id (for instanced maps) or the map id (for continents,
        // though we don't scale those).
        struct MapScaleState { uint32_t lastPlayerCount = 0; };
        std::unordered_map<uint64_t, MapScaleState> m_mapState;

        // Polling accumulator vs cfg.rescanIntervalMs.
        uint32_t m_timeSinceLastTick = 0;
    };
}
