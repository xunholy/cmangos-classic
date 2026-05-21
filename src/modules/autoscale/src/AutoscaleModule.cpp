#include "AutoscaleModule.h"

#include "Chat/Chat.h"
#include "Entities/Creature.h"
#include "Entities/Player.h"
#include "Log/Log.h"
#include "Maps/Map.h"
#include "Maps/MapManager.h"
#include "Maps/MapRefManager.h"
#include "World/World.h"

#include <algorithm>
#include <cmath>

namespace cmangos_module
{
    AutoscaleModule::AutoscaleModule()
    : Module("Autoscale", new AutoscaleModuleConfig())
    {
    }

    const AutoscaleModuleConfig* AutoscaleModule::GetConfig() const
    {
        return (const AutoscaleModuleConfig*)Module::GetConfig();
    }

    bool AutoscaleModule::IsEnabled() const
    {
        return GetConfig()->enabled;
    }

    void AutoscaleModule::OnInitialize()
    {
        // OnUpdate walks sMapMgr.Maps() and mutates Creature::MaxHealth from the
        // world thread. That's safe with single-threaded map updates (Maps.NumThreads = 0,
        // the cmangos default), but unsafe when map updates run on a thread pool —
        // a map thread could be ticking the same creature we're rescaling. We don't
        // disable the module here because operators may know their workload is
        // tolerant (e.g., no concurrent rescale + combat). Just warn loudly.
        if (sWorld.getConfig(CONFIG_UINT32_NUM_MAP_THREADS) > 0)
        {
            sLog.outError(
                "[Autoscale] Map updates are multi-threaded (NumThreads=%u). "
                "OnUpdate's cross-map rescale walks creatures without map-thread "
                "synchronisation — expect rare races on MaxHealth/Health. The "
                "OnAddToWorld path (which fires on the correct map thread) is "
                "always safe.",
                sWorld.getConfig(CONFIG_UINT32_NUM_MAP_THREADS));
        }
    }

    bool AutoscaleModule::ShouldScaleCreature(const Creature* creature) const
    {
        if (!creature || !creature->IsInWorld())
            return false;

        const Map* map = creature->GetMap();
        if (!map || !(map->IsDungeon() || map->IsRaid()))
            return false;

        // Skip player-allied creatures — scaling these would weaken players.
        if (creature->IsPet() || creature->IsTotem() || creature->IsTemporarySummon())
            return false;

        // Skip harmless mobs that don't engage.
        if (creature->IsCivilian())
            return false;

        // Honour the per-map blacklist (scripted boss phases, PvP-only maps, etc.).
        const uint32_t mapId = map->GetId();
        for (uint32_t blackId : GetConfig()->mapBlacklist)
            if (blackId == mapId)
                return false;

        return true;
    }

    uint32_t AutoscaleModule::ResolveBaseline(const Map* map) const
    {
        if (!map)
            return 0;
        const auto& cfg = *GetConfig();
        auto it = cfg.mapBaselines.find(map->GetId());
        if (it != cfg.mapBaselines.end())
            return it->second;
        return map->IsRaid() ? cfg.baselineRaidDefault : cfg.baselineDungeon;
    }

    float AutoscaleModule::ComputeScale(uint32_t playerCount, uint32_t baseline) const
    {
        if (playerCount == 0 || baseline == 0)
            return 1.0f;
        const float ratio = static_cast<float>(playerCount) / static_cast<float>(baseline);
        float scaled = std::pow(ratio, GetConfig()->hpExponent);
        scaled = std::clamp(scaled, GetConfig()->minScale, GetConfig()->maxScale);
        return scaled;
    }

    float AutoscaleModule::ComputeDmgScale(uint32_t playerCount, uint32_t baseline) const
    {
        if (playerCount == 0 || baseline == 0)
            return 1.0f;
        const float ratio = static_cast<float>(playerCount) / static_cast<float>(baseline);
        float scaled = std::pow(ratio, GetConfig()->dmgExponent);
        scaled = std::clamp(scaled, GetConfig()->minDmgScale, GetConfig()->maxDmgScale);
        return scaled;
    }

    uint64_t AutoscaleModule::MakeMapKey(const Map* map)
    {
        return (static_cast<uint64_t>(map->GetId()) << 32)
             | static_cast<uint64_t>(map->GetInstanceId());
    }

    bool AutoscaleModule::ScaleCreature(Creature* creature, float hpScale)
    {
        if (!creature || !ShouldScaleCreature(creature))
            return false;

        // Base HP comes from the creature template — NOT current MaxHealth.
        // Always recomputing against the template means re-scaling never
        // chain-multiplies (a creature scaled to 0.5 then 0.7 ends up at
        // template * 0.7, not template * 0.35).
        const CreatureInfo* info = creature->GetCreatureInfo();
        if (!info)
            return false;
        uint32_t baseHp = info->MaxLevelHealth;
        if (baseHp == 0)
            baseHp = creature->GetMaxHealth(); // template missing data; least-bad fallback

        uint32_t scaledMax = static_cast<uint32_t>(static_cast<float>(baseHp) * hpScale);
        if (scaledMax < 1)
            scaledMax = 1;

        // Preserve the current-HP fraction so a mob at 30% stays at 30% of
        // its new max (otherwise rescaling a wounded mob would suddenly
        // heal or kill it).
        const uint32_t curMax = creature->GetMaxHealth();
        const float pct = curMax > 0
            ? static_cast<float>(creature->GetHealth()) / static_cast<float>(curMax)
            : 1.0f;

        creature->SetMaxHealth(scaledMax);
        creature->SetHealth(static_cast<uint32_t>(static_cast<float>(scaledMax) * pct));
        return true;
    }

    void AutoscaleModule::OnAddToWorld(Creature* creature)
    {
        if (!IsEnabled() || !ShouldScaleCreature(creature))
            return;

        const Map* map = creature->GetMap();
        const uint32_t playerCount = map->GetPlayers().getSize();
        const uint32_t baseline = ResolveBaseline(map);
        const float hpScale = ComputeScale(playerCount, baseline);

        // Prime the dmg-scale cache so OnPreDealDamage doesn't have to lazy-
        // compute on first hit in a fresh instance. Stored even when hpScale
        // is a no-op because the two curves can be tuned independently.
        m_mapState[MakeMapKey(map)].dmgScale = ComputeDmgScale(playerCount, baseline);

        // No-op above baseline — we only scale down, never up.
        if (hpScale >= 0.999f)
            return;

        ScaleCreature(creature, hpScale);
    }

    bool AutoscaleModule::OnPreDealDamage(Unit* dealer, Unit* /*victim*/, uint32& outDamage)
    {
        if (!IsEnabled() || !dealer || outDamage == 0)
            return false;

        // Only mob damage gets scaled — players, pets, totems, and temporary
        // summons are excluded by ShouldScaleCreature.
        if (dealer->GetTypeId() != TYPEID_UNIT)
            return false;

        const Creature* c = static_cast<const Creature*>(dealer);
        if (!ShouldScaleCreature(c))
            return false;

        const Map* map = c->GetMap();
        auto it = m_mapState.find(MakeMapKey(map));
        if (it == m_mapState.end())
            return false;  // not yet observed — leave damage untouched

        const float dmgScale = it->second.dmgScale;
        if (dmgScale >= 0.999f)
            return false;

        // Never floor outgoing damage to 0 — that would break rage gen on hits
        // and any encounter logic that fires on a successful damage event.
        uint32_t scaled = static_cast<uint32_t>(static_cast<float>(outDamage) * dmgScale);
        if (scaled < 1)
            scaled = 1;
        outDamage = scaled;
        return true;
    }

    void AutoscaleModule::OnUpdate(uint32 elapsed)
    {
        if (!IsEnabled())
            return;

        m_timeSinceLastTick += elapsed;
        if (m_timeSinceLastTick < GetConfig()->rescanIntervalMs)
            return;
        m_timeSinceLastTick = 0;

        // Walk every loaded map; for instance maps, rescale on player-count
        // change. Out-of-combat creatures only — touching MaxHealth mid-fight
        // would break enrage timers and DPS pacing in ways players notice.
        for (const auto& kv : sMapMgr.Maps())
        {
            Map* map = kv.second.get();
            if (!map || !(map->IsDungeon() || map->IsRaid()))
                continue;

            const uint32_t playerCount = map->GetPlayers().getSize();
            auto& state = m_mapState[MakeMapKey(map)];
            if (playerCount == state.lastPlayerCount)
                continue;
            state.lastPlayerCount = playerCount;

            // Empty instance — leave state recorded, don't bother scaling.
            // Next time a player enters we'll hit the OnAddToWorld path for
            // new spawns and re-trigger this branch for existing ones.
            if (playerCount == 0)
            {
                state.dmgScale = 1.0f;
                continue;
            }

            const uint32_t baseline = ResolveBaseline(map);
            const float hpScale = ComputeScale(playerCount, baseline);
            state.dmgScale = ComputeDmgScale(playerCount, baseline);

            uint32_t rescaled = 0;
            auto& store = map->GetObjectsStore();
            for (auto it = store.begin<Creature>(); it != store.end<Creature>(); ++it)
            {
                Creature* c = it->second;
                if (!c || c->IsInCombat())
                    continue;
                if (ScaleCreature(c, hpScale))
                    ++rescaled;
            }

            if (GetConfig()->announce && rescaled > 0)
            {
                for (const auto& ref : map->GetPlayers())
                {
                    if (Player* p = const_cast<MapReference&>(ref).getSource())
                    {
                        ChatHandler(p).PSendSysMessage(
                            "|cffffd200[Autoscale]|r Difficulty adjusted for %u player%s — mobs now at %.0f%% HP, %.0f%% damage.",
                            playerCount, playerCount == 1 ? "" : "s",
                            hpScale * 100.0f, state.dmgScale * 100.0f);
                    }
                }
            }
        }
    }
}
