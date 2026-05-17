#include "VipModuleConfig.h"

#include <sstream>
#include <string>

namespace cmangos_module
{
    VipModuleConfig::VipModuleConfig()
    : ModuleConfig("vip.conf")
    , enabled(false)
    , masterSpellId(91200)
    {
    }

    bool VipModuleConfig::OnLoad()
    {
        enabled       = config.GetBoolDefault("Vip.Enable", false);
        masterSpellId = config.GetIntDefault("Vip.MasterSpellId", 91200);

        // CSV of spell IDs the master cascades into. Empty / unset → built-in
        // default list covering the standard world buffs + a few headline
        // consumables. Matches the Wayfarer's Boon design.
        std::string raw = config.GetStringDefault("Vip.BundledSpellIds", "");

        bundledSpellIds.clear();
        if (raw.empty())
        {
            // Vanilla 1.12 spell IDs:
            //   22888 Rallying Cry of the Dragonslayer (Onyxia/Nef head buff)
            //   24425 Spirit of Zandalar                (ZG zone-up buff)
            //   23735 Mol'dar's Moxie                   (DM tribute: +3% crit, +stam)
            //   23736 Fengus' Ferocity                  (DM tribute: +200 AP)
            //   23737 Slip'kik's Savvy                  (DM tribute: +3% spell crit)
            //   15366 Songflower Serenade               (+15 all stats, +5% crit)
            //   16609 Warchief's Blessing               (Horde counterpart of Onyxia)
            //   17626 Greater Arcane Elixir             (+35 spell dmg)
            //   17627 Distilled Wisdom (Flask)          (+65 intellect)
            //   17628 Supreme Power (Flask)             (+150 spell dmg)
            bundledSpellIds = {
                22888, 24425, 23735, 23736, 23737,
                15366, 16609, 17626, 17627, 17628
            };
        }
        else
        {
            std::stringstream ss(raw);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                // trim whitespace
                size_t start = token.find_first_not_of(" \t");
                size_t end   = token.find_last_not_of(" \t");
                if (start == std::string::npos)
                    continue;
                token = token.substr(start, end - start + 1);
                try
                {
                    uint32_t id = static_cast<uint32_t>(std::stoul(token));
                    if (id)
                        bundledSpellIds.push_back(id);
                }
                catch (...) {}
            }
        }

        return true;
    }
}
