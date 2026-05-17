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

        // List of spell IDs taught when a character is granted VIP. Defaults
        // bundle the vanilla world buffs + a handful of consumable buffs so
        // a freshly-flagged VIP has the full min-maxer kit in their book.
        // Configurable as a CSV string in vip.conf so the operator can
        // tweak without recompiling.
        std::vector<uint32_t> grantedSpellIds;
    };
}
