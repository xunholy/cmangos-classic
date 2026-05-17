#pragma once
#include "ModuleConfig.h"

#include <cstdint>
#include <vector>

namespace cmangos_module
{
    class VipModuleConfig : public ModuleConfig
    {
    public:
        VipModuleConfig();
        bool OnLoad() override;

        bool enabled;

        // ID of the single "Wayfarer's Boon" master spell taught to VIPs.
        // The spell itself is a no-op on the engine side (Effect1=DUMMY);
        // VipModule::OnCast detects this ID and casts every spell in
        // bundledSpellIds on the caster as triggered spells.
        uint32_t masterSpellId;

        // Bundle of spells the master cascades into when cast. Defaults
        // are the vanilla world buffs + a handful of consumable buffs so
        // a freshly-flagged VIP gets the full min-maxer kit in one cast.
        // Configurable as a CSV string in vip.conf so the operator can
        // tweak without recompiling.
        std::vector<uint32_t> bundledSpellIds;
    };
}
