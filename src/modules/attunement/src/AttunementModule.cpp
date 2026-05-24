#include "AttunementModule.h"
#include "Chat/Chat.h"
#include "Database/DatabaseEnv.h"
#include "Entities/Player.h"
#include "Entities/GossipDef.h"
#include "AI/ScriptDevAI/include/sc_gossip.h"
#include "Globals/ObjectMgr.h"
#include "Globals/ObjectAccessor.h"
#include "Server/DBCStores.h"
#include "Spells/SpellMgr.h"
#include "SystemConfig.h"
#include "World/World.h"

#ifdef ENABLE_PLAYERBOTS
#include "playerbot/PlayerbotAI.h"
#endif

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace cmangos_module
{
    enum AttunementActions
    {
        ACTION_MAIN_MENU       = 100,
        ACTION_CUSTOM_INPUT    = 102,
        ACTION_BOOST_TO_MAX    = 103,  // shows confirmation submenu
        ACTION_BOOST_CONFIRM   = 104,  // user confirmed — actually fire the boost
        ACTION_HARDCORE_MENU   = 105,  // shows the hardcore-challenges submenu

        // Preset rate picks. Action codes encode rate × 100, offset by base.
        // e.g. 1× -> 100, 5× -> 500, 10× -> 1000, 100× -> 10000.
        ACTION_RATE_BASE       = 10000,
    };

    static const uint32 NPC_TEXT_GREETING      = 50930;
    static const uint32 NPC_TEXT_CUSTOM        = 50931;
    static const uint32 NPC_TEXT_BOOST_CONFIRM = 50932;

    static const uint32 ATTUNEMENT_MAX_LEVEL = 60;
    static const uint32 BOOST_GOLD_COPPER    = 500 * 10000; // 500 gold

    // Preset rates surfaced as gossip options. Custom branch covers anything
    // in-between; the level-boost action is a separate branch.
    static const float PRESET_RATES[] = { 1.0f, 2.0f, 5.0f, 10.0f };
    static const size_t PRESET_RATES_COUNT = sizeof(PRESET_RATES) / sizeof(PRESET_RATES[0]);

    // Tier 0 ("dungeon set 1") starter sets per Classic class.
    // 8 pieces each: head, shoulders, chest, wrists, hands, waist, legs, feet.
    // Terminator 0 keeps iteration simple.
    static const uint32 T0_WARRIOR[] = {16730, 16731, 16732, 16733, 16734, 16735, 16736, 16737, 0};
    static const uint32 T0_PALADIN[] = {16722, 16723, 16724, 16725, 16726, 16727, 16728, 16729, 0};
    static const uint32 T0_HUNTER[]  = {16674, 16675, 16676, 16677, 16678, 16679, 16680, 16681, 0};
    static const uint32 T0_ROGUE[]   = {16707, 16708, 16709, 16710, 16711, 16712, 16713, 16721, 0};
    static const uint32 T0_PRIEST[]  = {16690, 16691, 16692, 16693, 16694, 16695, 16696, 16697, 0};
    // Shaman D1 "The Elements" — full 8-piece blue mail set (matches the
    // other classes' T0 sets in quality and item-level).
    static const uint32 T0_SHAMAN[]  = {
        16667, // Coif of Elements (head)
        16669, // Pauldrons of Elements (shoulders)
        16666, // Vest of Elements (chest)
        16668, // Kilt of Elements (legs)
        16670, // Boots of Elements (feet)
        16671, // Bindings of Elements (wrists)
        16672, // Gauntlets of Elements (hands)
        16673, // Cord of Elements (waist)
        0
    };
    static const uint32 T0_MAGE[]    = {16682, 16683, 16684, 16685, 16686, 16687, 16688, 16689, 0};
    static const uint32 T0_WARLOCK[] = {16698, 16699, 16700, 16701, 16702, 16703, 16704, 16705, 0};
    static const uint32 T0_DRUID[]   = {16706, 16714, 16715, 16716, 16717, 16718, 16719, 16720, 0};

    static const uint32* GetTierZeroSet(uint32 classId)
    {
        switch (classId)
        {
            case CLASS_WARRIOR: return T0_WARRIOR;
            case CLASS_PALADIN: return T0_PALADIN;
            case CLASS_HUNTER:  return T0_HUNTER;
            case CLASS_ROGUE:   return T0_ROGUE;
            case CLASS_PRIEST:  return T0_PRIEST;
            case CLASS_SHAMAN:  return T0_SHAMAN;
            case CLASS_MAGE:    return T0_MAGE;
            case CLASS_WARLOCK: return T0_WARLOCK;
            case CLASS_DRUID:   return T0_DRUID;
            default:            return nullptr;
        }
    }

    // Class-specific weapons / off-hands / ranged. All blue (Q3) level 55-60.
    // mainHand / offHand / ranged; 0 means slot is empty for this class.
    struct ClassWeapons { uint32 mainHand; uint32 offHand; uint32 ranged; };

    // Demonshear (2H sword), Hammer of the Titans (2H mace), Eaglehorn Long Bow,
    // Heartseeker (dagger), Fang of the Crystal Spider (dagger), Skullforge Reaver (1H sword),
    // Draconian Deflector (shield), Staff of Hale Magefire, Banshee Finger (wand),
    // Rod of the Ogre Magi (staff), Oblivion's Touch (wand), Whiteout Staff.
    static const ClassWeapons WEAPONS_WARRIOR  = { 13348,     0,     0 };
    static const ClassWeapons WEAPONS_PALADIN  = { 13361, 12602,     0 };
    static const ClassWeapons WEAPONS_HUNTER   = { 13348,     0, 13023 };
    static const ClassWeapons WEAPONS_ROGUE    = { 12783, 13218,     0 };
    static const ClassWeapons WEAPONS_PRIEST   = { 13000,     0, 13534 };
    static const ClassWeapons WEAPONS_SHAMAN   = { 12796,     0,     0 };
    static const ClassWeapons WEAPONS_MAGE     = { 18534,     0, 18761 };
    static const ClassWeapons WEAPONS_WARLOCK  = { 18534,     0, 18761 };
    static const ClassWeapons WEAPONS_DRUID    = { 19101,     0,     0 };

    static const ClassWeapons* GetClassWeapons(uint32 classId)
    {
        switch (classId)
        {
            case CLASS_WARRIOR: return &WEAPONS_WARRIOR;
            case CLASS_PALADIN: return &WEAPONS_PALADIN;
            case CLASS_HUNTER:  return &WEAPONS_HUNTER;
            case CLASS_ROGUE:   return &WEAPONS_ROGUE;
            case CLASS_PRIEST:  return &WEAPONS_PRIEST;
            case CLASS_SHAMAN:  return &WEAPONS_SHAMAN;
            case CLASS_MAGE:    return &WEAPONS_MAGE;
            case CLASS_WARLOCK: return &WEAPONS_WARLOCK;
            case CLASS_DRUID:   return &WEAPONS_DRUID;
            default:            return nullptr;
        }
    }

    // Weapon-skill IDs per class (null-terminated). Lifted from twinkmaster.
    static const uint16 WEAPON_SKILLS_WARRIOR[] = { 43, 44, 45, 46, 54, 55, 136, 160, 162, 172, 173, 176, 226, 229, 473, 0 };
    static const uint16 WEAPON_SKILLS_PALADIN[] = { 43, 44, 54, 55, 160, 162, 229, 0 };
    static const uint16 WEAPON_SKILLS_HUNTER[]  = { 43, 44, 45, 46, 55, 136, 162, 172, 173, 176, 226, 229, 473, 0 };
    static const uint16 WEAPON_SKILLS_ROGUE[]   = { 43, 45, 46, 54, 162, 173, 176, 226, 473, 0 };
    static const uint16 WEAPON_SKILLS_PRIEST[]  = { 54, 136, 162, 173, 228, 0 };
    static const uint16 WEAPON_SKILLS_SHAMAN[]  = { 44, 54, 136, 160, 162, 172, 173, 473, 0 };
    static const uint16 WEAPON_SKILLS_MAGE[]    = { 43, 136, 162, 173, 228, 0 };
    static const uint16 WEAPON_SKILLS_WARLOCK[] = { 43, 136, 162, 173, 228, 0 };
    static const uint16 WEAPON_SKILLS_DRUID[]   = { 54, 136, 160, 162, 173, 473, 0 };

    static const uint16 SKILL_DEFENSE = 95;

    static const uint16* GetWeaponSkillsForClass(uint8 classId)
    {
        switch (classId)
        {
            case CLASS_WARRIOR: return WEAPON_SKILLS_WARRIOR;
            case CLASS_PALADIN: return WEAPON_SKILLS_PALADIN;
            case CLASS_HUNTER:  return WEAPON_SKILLS_HUNTER;
            case CLASS_ROGUE:   return WEAPON_SKILLS_ROGUE;
            case CLASS_PRIEST:  return WEAPON_SKILLS_PRIEST;
            case CLASS_SHAMAN:  return WEAPON_SKILLS_SHAMAN;
            case CLASS_MAGE:    return WEAPON_SKILLS_MAGE;
            case CLASS_WARLOCK: return WEAPON_SKILLS_WARLOCK;
            case CLASS_DRUID:   return WEAPON_SKILLS_DRUID;
            default:            return nullptr;
        }
    }

    // SpellFamilyName per class (Blizzard mapping). Used to filter
    // npc_trainer_template hits to spells that actually belong to this class.
    static const uint32 CLASS_SPELL_FAMILY[] = {
        0, 4, 10, 9, 8, 6, 0, 11, 3, 5, 0, 7
    };

    // Per-class boost accessories. 6 fixed slots:
    //   0 = cloak, 1 = neck, 2 = ring, 3 = ring, 4 = trinket, 5 = trinket.
    // 0 leaves the slot empty (GiveItem no-ops on 0). All picks are blue
    // Q3, lvl 55-60. The previous single class-agnostic list handed
    // Primalist's Seal (caster ring) to Rogues, Hakkari Loa Cloak
    // (healing cloak) to Warriors, etc. — split per class so each
    // archetype receives stat-appropriate gear.
    //
    // Physical pool (AP / agi / crit):
    //   12968 Cape of the Black Baron     +6 agi  +9 sta
    //   12846 Mark of Fordring            +26 AP  +4 def
    //   17713 Painweaver Band             +18 AP  +1% crit
    //   13098 Don Julio's Band            +4 AP   +1% crit
    //   11815 Hand of Justice             +20 AP  +2% chance extra attack
    //   18370 Vigilance Charm             +14 sta
    //   13382 Cannonball Runner           on-use ranged dmg (hunter-only)
    //
    // Caster pool (int / healing / spell power):
    //   19870 Hakkari Loa Cloak           +18 healing  +8 spell dmg
    //   19871 Talisman of Protection      +14 sta neck
    //   19863 Primalist's Seal            +12 sta +12 int +healing/sd
    //   18404 Underworld Band             +8 int   +11 healing / +5 sd
    //   18820 Talisman of Ephemeral Power on-use +40 spell dmg (15s)
    static const size_t ACCESSORY_SLOTS = 6;

    static const uint32 ACCESSORIES_WARRIOR[ACCESSORY_SLOTS] = {12968, 12846, 17713, 13098, 11815, 18370};
    static const uint32 ACCESSORIES_PALADIN[ACCESSORY_SLOTS] = {12968, 12846, 17713, 13098, 11815, 18370};
    static const uint32 ACCESSORIES_HUNTER[ACCESSORY_SLOTS]  = {12968, 12846, 17713, 13098, 11815, 13382};
    static const uint32 ACCESSORIES_ROGUE[ACCESSORY_SLOTS]   = {12968, 12846, 17713, 13098, 11815, 18370};
    static const uint32 ACCESSORIES_PRIEST[ACCESSORY_SLOTS]  = {19870, 19871, 19863, 18404, 18820, 18370};
    static const uint32 ACCESSORIES_SHAMAN[ACCESSORY_SLOTS]  = {12968, 12846, 17713, 13098, 11815, 18370};
    static const uint32 ACCESSORIES_MAGE[ACCESSORY_SLOTS]    = {19870, 19871, 19863, 18404, 18820, 18370};
    static const uint32 ACCESSORIES_WARLOCK[ACCESSORY_SLOTS] = {19870, 19871, 19863, 18404, 18820, 18370};
    static const uint32 ACCESSORIES_DRUID[ACCESSORY_SLOTS]   = {12968, 12846, 17713, 13098, 11815, 18370};

    static const uint32* GetBoostAccessories(uint32 classId)
    {
        switch (classId)
        {
            case CLASS_WARRIOR: return ACCESSORIES_WARRIOR;
            case CLASS_PALADIN: return ACCESSORIES_PALADIN;
            case CLASS_HUNTER:  return ACCESSORIES_HUNTER;
            case CLASS_ROGUE:   return ACCESSORIES_ROGUE;
            case CLASS_PRIEST:  return ACCESSORIES_PRIEST;
            case CLASS_SHAMAN:  return ACCESSORIES_SHAMAN;
            case CLASS_MAGE:    return ACCESSORIES_MAGE;
            case CLASS_WARLOCK: return ACCESSORIES_WARLOCK;
            case CLASS_DRUID:   return ACCESSORIES_DRUID;
            default:            return nullptr;
        }
    }

    static bool IsAttunementNPC(Creature* creature)
    {
        return creature && creature->GetEntry() == ATTUNEMENT_NPC_ENTRY;
    }

    // Mark every faction-appropriate taxinode as discovered. Vanilla taxinode
    // faction gating lives in TaxiNodesEntry::MountCreatureID — index 0 is
    // Horde, index 1 is Alliance (see ObjectMgr.cpp where the same lookup is
    // used to pick the mount NPC). A zero entry for the player's team means
    // that node is unreachable by that faction.
    static void UnlockAllFlightPaths(Player* player)
    {
        if (!player)
            return;
        uint8 teamIdx = player->GetTeam() == ALLIANCE ? 1 : 0;
        for (uint32 id = 1; id < sTaxiNodesStore.GetNumRows(); ++id)
        {
            TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
            if (!node)
                continue;
            if (node->MountCreatureID[teamIdx] == 0)
                continue;
            player->m_taxi.SetTaximaskNode(id);
        }
    }

    // Flip every bit in PLAYER_EXPLORED_ZONES_1..64 so the world map is
    // fully revealed. Cheaper and simpler than walking AreaTable.dbc — the
    // engine just OR's into these fields when an area is entered, so
    // pre-filling them has the same end state.
    static void DiscoverAllZones(Player* player)
    {
        if (!player)
            return;
        for (uint32 i = 0; i < PLAYER_EXPLORED_ZONES_SIZE; ++i)
            player->SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + i, 0xFFFFFFFFu);
    }

    static void GiveItem(Player* player, uint32 itemId)
    {
        if (!itemId)
            return;

        // Prefer auto-equip so a fresh character with a 16-slot backpack
        // doesn't run out of room before all items are granted.
        uint16 eqDest = 0;
        InventoryResult eqRes = player->CanEquipNewItem(NULL_SLOT, eqDest, itemId, false);
        if (eqRes == EQUIP_ERR_OK)
        {
            if (Item* item = player->EquipNewItem(eqDest, itemId, true))
            {
                player->AutoUnequipOffhandIfNeed();
                player->SendNewItem(item, 1, true, false);
                return;
            }
        }

        // Fall back to bag storage if not equippable (or no slot available).
        ItemPosCountVec dest;
        InventoryResult res = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1);
        if (res != EQUIP_ERR_OK)
            return;
        if (Item* item = player->StoreNewItem(dest, itemId, true))
            player->SendNewItem(item, 1, true, false);
    }

    AttunementModule::AttunementModule()
    : Module("Attunement", new AttunementModuleConfig())
    , m_getReactionToInternal(false)
    {
    }

    const AttunementModuleConfig* AttunementModule::GetConfig() const
    {
        return (const AttunementModuleConfig*)Module::GetConfig();
    }

    bool AttunementModule::IsEnabled() const
    {
        return GetConfig()->enabled;
    }

    float AttunementModule::GetXpRate(uint32 guid) const
    {
        auto it = m_playerRates.find(guid);
        return it != m_playerRates.end() ? it->second : GetConfig()->defaultRate;
    }

    float AttunementModule::ClampRate(float rate) const
    {
        const AttunementModuleConfig* cfg = GetConfig();
        if (!std::isfinite(rate) || rate <= 0.0f)
            return cfg->defaultRate;
        if (rate < cfg->minRate)
            rate = cfg->minRate;
        if (cfg->maxRate > 0.0f && rate > cfg->maxRate)
            rate = cfg->maxRate;
        return rate;
    }

    uint32 AttunementModule::PickAuraSpell(float rate) const
    {
        const AttunementModuleConfig* cfg = GetConfig();
        if (rate <= 1.0f)  return cfg->auraTier1SpellId;
        if (rate <= 2.0f)  return cfg->auraTier2SpellId;
        if (rate <= 5.0f)  return cfg->auraTier3SpellId;
        if (rate <= 25.0f) return cfg->auraTier4SpellId;
        return cfg->auraTier5SpellId;
    }

    void AttunementModule::OnInitialize()
    {
        if (GetConfig()->hardcoreEnabled)
        {
            LoadLoot();
            LoadGraves();
        }
    }

    void AttunementModule::OnWorldPreInitialized()
    {
        if (GetConfig()->hardcoreEnabled)
        {
            PreLoadLoot();
            PreLoadGraves();
            m_deathLog.Load();
        }
    }

    void AttunementModule::LearnWeaponSkills(Player* player, uint32 targetLevel)
    {
        uint16 maxSkill = static_cast<uint16>(targetLevel * 5);

        if (const uint16* skills = GetWeaponSkillsForClass(player->getClass()))
            for (const uint16* s = skills; *s; ++s)
                player->SetSkill(*s, maxSkill, maxSkill);

        player->SetSkill(SKILL_DEFENSE, maxSkill, maxSkill);
    }

    // Returns true if the hardcore submenu would render at least one row for
    // this player. Used by OnPreGossipHello to decide whether to surface the
    // "Hardcore challenges..." top-level link. Logic mirrors ShowHardcoreMenu
    // exactly — a toggle appears either because the player is already in the
    // challenge (opt-out always available) or because they can opt in (level
    // 1 + module config has that challenge enabled).
    bool AttunementModule::HasAnyHardcoreOption(Player* player, const HardcorePlayerConfig* playerConfig) const
    {
        const AttunementModuleConfig* moduleConfig = GetConfig();
        if (!moduleConfig->hardcoreEnabled || !playerConfig || !player)
            return false;

        bool canOptIn = (player->GetLevel() == 1);

        if (moduleConfig->reviveDisabled    && (playerConfig->IsReviveDisabled()      || canOptIn)) return true;
        if (moduleConfig->selfFound         && (playerConfig->IsSelfFound()           || canOptIn)) return true;
        if (moduleConfig->IsDropLootEnabled() && (playerConfig->ShouldDropLootOnDeath() || canOptIn)) return true;
        if (moduleConfig->levelDownPct > 0.0f && (playerConfig->ShouldLoseXPOnDeath()   || canOptIn)) return true;
        if (moduleConfig->disablePVP        && (playerConfig->IsPVPDisabled()         || canOptIn)) return true;
        return false;
    }

    void AttunementModule::ShowHardcoreMenu(Player* player, Creature* creature)
    {
        PlayerMenu* playerMenu = player->GetPlayerMenu();
        if (!playerMenu)
            return;

        playerMenu->ClearMenus();

        const AttunementModuleConfig* moduleConfig = GetConfig();
        const HardcorePlayerConfig* playerConfig = GetPlayerConfig(player);

        // Same per-toggle logic as the old inline block in OnPreGossipHello.
        // Opt-out is always visible if currently in the challenge; opt-in is
        // gated on level == 1 (hardcore is a fresh-character decision).
        if (moduleConfig->hardcoreEnabled && playerConfig)
        {
            bool canOptIn = (player->GetLevel() == 1);

            if (moduleConfig->reviveDisabled)
            {
                if (playerConfig->IsReviveDisabled())
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_STOP_HARDCORE_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_STOP_HARDCORE_CHALLENGE, "", 0);
                else if (canOptIn)
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_HARDCORE_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_HARDCORE_CHALLENGE, "", 0);
            }

            if (moduleConfig->selfFound)
            {
                if (playerConfig->IsSelfFound())
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_STOP_SELF_FOUND_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_STOP_SELF_FOUND_CHALLENGE, "", 0);
                else if (canOptIn)
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_SELF_FOUND_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_SELF_FOUND_CHALLENGE, "", 0);
            }

            if (moduleConfig->IsDropLootEnabled())
            {
                if (playerConfig->ShouldDropLootOnDeath())
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_STOP_DROP_LOOT_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_STOP_DROP_LOOT_CHALLENGE, "", 0);
                else if (canOptIn)
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DROP_LOOT_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DROP_LOOT_CHALLENGE, "", 0);
            }

            if (moduleConfig->levelDownPct > 0.0f)
            {
                if (playerConfig->ShouldLoseXPOnDeath())
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_STOP_LOSE_XP_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_STOP_LOSE_XP_CHALLENGE, "", 0);
                else if (canOptIn)
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_LOSE_XP_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_LOSE_XP_CHALLENGE, "", 0);
            }

            if (moduleConfig->disablePVP)
            {
                if (playerConfig->IsPVPDisabled())
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ENABLE_PVP), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ENABLE_PVP, "", 0);
                else if (canOptIn)
                    playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DISABLE_PVP), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DISABLE_PVP, "", 0);
            }
        }

        playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU, "", false);

        playerMenu->SendGossipMenu(NPC_TEXT_GREETING, creature->GetObjectGuid());
    }

    bool AttunementModule::HasAccountBoosted(uint32 accountId) const
    {
        if (accountId == 0)
            return false;

        auto result = CharacterDatabase.PQuery(
            "SELECT 1 FROM `custom_attunement_account_boost` WHERE `account_id` = %u",
            accountId);
        return result != nullptr;
    }

    void AttunementModule::RecordAccountBoost(uint32 accountId, Player* player) const
    {
        if (accountId == 0 || !player)
            return;

        // Escape the player name to be safe against any future renames containing
        // backticks/quotes. mangos PExecute does not auto-escape string params.
        std::string safeName = player->GetName() ? player->GetName() : "";
        CharacterDatabase.escape_string(safeName);

        CharacterDatabase.PExecute(
            "INSERT IGNORE INTO `custom_attunement_account_boost` "
            "(`account_id`, `boosted_guid`, `boosted_name`) VALUES (%u, %u, '%s')",
            accountId, player->GetGUIDLow(), safeName.c_str());
    }

    void AttunementModule::LearnClassSpells(Player* player, uint32 targetLevel)
    {
        uint32 classId = player->getClass();
        uint32 expectedFamily = (classId < 12) ? CLASS_SPELL_FAMILY[classId] : 0;

        auto result = WorldDatabase.PQuery(
            "SELECT DISTINCT ntt.spell FROM npc_trainer_template ntt "
            "JOIN creature_template ct ON ct.trainertemplateid = ntt.entry "
            "WHERE ct.trainer_class = %u AND ntt.reqlevel <= %u AND ntt.reqlevel > 0",
            classId, targetLevel);

        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            uint32 spellId = fields[0].GetUInt32();

            const SpellEntry* spell = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
            if (!spell)
                continue;

            // Skip spells that belong to a different class family
            if (spell->SpellFamilyName != 0 && spell->SpellFamilyName != expectedFamily)
                continue;

            if (!player->HasSpell(spellId))
                player->learnSpell(spellId, false);
        } while (result->NextRow());
    }

    void AttunementModule::OnLoadFromDB(Player* player)
    {
        if (!IsEnabled() || !player)
            return;

        uint32 guid = player->GetGUIDLow();

        auto result = CharacterDatabase.PQuery(
            "SELECT `value` FROM `custom_attunement_player_config` "
            "WHERE `guid` = %u AND `option_key` = 'xp_rate'", guid);

        if (result)
        {
            Field* fields = result->Fetch();
            float rate = ClampRate(fields[0].GetFloat());
            m_playerRates[guid] = rate;
            RefreshAura(player, rate);
        }
    }

    void AttunementModule::OnLogOut(Player* player)
    {
        if (player)
        {
            uint32 guid = player->GetGUIDLow();
            m_playerRates.erase(guid);
            m_lastSelection.erase(guid);
        }
    }

    void AttunementModule::OnDeleteFromDB(uint32 playerId)
    {
        // Attunement: wipe all per-player config when the character is
        // deleted so a recycled GUID doesn't inherit stale flags (e.g.
        // 'boosted').
        CharacterDatabase.PExecute(
            "DELETE FROM `custom_attunement_player_config` WHERE `guid` = %u",
            playerId);

        // Hardcore: drop graves, loot, and player-challenge state.
        if (GetConfig()->removeGraveOnCharacterDeleted)
        {
            auto graveIt = m_playerGraves.find(playerId);
            if (graveIt != m_playerGraves.end())
            {
                graveIt->second.Destroy();
                m_playerGraves.erase(graveIt);
            }
        }

        if (GetConfig()->removeLootOnCharacterDeleted)
        {
            auto playerLootIt = m_playersLoot.find(playerId);
            if (playerLootIt != m_playersLoot.end())
            {
                for (auto lootIt = playerLootIt->second.begin(); lootIt != playerLootIt->second.end(); ++lootIt)
                {
                    lootIt->second.Destroy();
                }

                m_playersLoot.erase(playerLootIt);
            }
        }

        auto playerManagerIt = m_playerManagers.find(playerId);
        if (playerManagerIt != m_playerManagers.end())
        {
            playerManagerIt->second.Destroy();
            m_playerManagers.erase(playerId);
        }
    }

    void AttunementModule::OnRegenerate(Player* player, uint8 /*power*/, uint32 /*diff*/, float& /*addedValue*/)
    {
        if (!IsEnabled() || !player || !player->IsInWorld())
            return;

        uint32 inspectorGuid = player->GetGUIDLow();
        ObjectGuid currentSel = player->GetSelectionGuid();

        auto it = m_lastSelection.find(inspectorGuid);
        ObjectGuid lastSel = (it != m_lastSelection.end()) ? it->second : ObjectGuid();

        if (currentSel == lastSel)
            return;
        m_lastSelection[inspectorGuid] = currentSel;

        if (currentSel.IsEmpty() || currentSel == player->GetObjectGuid())
            return;

        Player* target = sObjectAccessor.FindPlayer(currentSel);
        if (!target)
            return;

        float rate = GetXpRate(target->GetGUIDLow());
        if (rate == GetConfig()->defaultRate)
            return;

        ChatHandler(player).PSendSysMessage(
            "|cff1eff00[Attunement]|r %s walks the path of %.2gx XP.",
            target->GetName(), rate);
    }

    bool AttunementModule::OnPreGiveXP(Player* player, uint32& xp, Creature* /*victim*/)
    {
        if (!IsEnabled() || !player || xp == 0)
            return false;

        float rate = GetXpRate(player->GetGUIDLow());
        if (rate == 1.0f)
            return false;

        // Round to nearest. cmangos returns false to mean "did not override";
        // mutating xp in-place and returning false lets the engine continue
        // with the multiplied value while preserving downstream hooks.
        double scaled = static_cast<double>(xp) * static_cast<double>(rate);
        if (scaled < 0.0)
            scaled = 0.0;
        if (scaled > static_cast<double>(UINT32_MAX))
            scaled = static_cast<double>(UINT32_MAX);
        xp = static_cast<uint32>(scaled + 0.5);
        return false;
    }

    void AttunementModule::SetXpRate(Player* player, float rate)
    {
        if (!player)
            return;

        rate = ClampRate(rate);
        uint32 guid = player->GetGUIDLow();

        if (rate == GetConfig()->defaultRate)
        {
            CharacterDatabase.PExecute(
                "DELETE FROM `custom_attunement_player_config` "
                "WHERE `guid` = %u AND `option_key` = 'xp_rate'", guid);
            m_playerRates.erase(guid);
        }
        else
        {
            CharacterDatabase.PExecute(
                "REPLACE INTO `custom_attunement_player_config` (`guid`, `option_key`, `value`) "
                "VALUES (%u, 'xp_rate', %f)", guid, rate);
            m_playerRates[guid] = rate;
        }

        RefreshAura(player, rate);

        char msg[128];
        snprintf(msg, sizeof(msg), "XP rate set to %.2fx.", rate);
        player->GetSession()->SendNotification("%s", msg);
    }

    void AttunementModule::RefreshAura(Player* player, float rate)
    {
        if (!player)
            return;

        const AttunementModuleConfig* cfg = GetConfig();
        const uint32 allTierSpells[] = {
            cfg->auraTier1SpellId,
            cfg->auraTier2SpellId,
            cfg->auraTier3SpellId,
            cfg->auraTier4SpellId,
            cfg->auraTier5SpellId,
        };

        for (uint32 spellId : allTierSpells)
            if (spellId != 0)
                player->RemoveAurasDueToSpell(spellId);

        uint32 picked = PickAuraSpell(rate);
        if (picked != 0)
            player->CastSpell(player, picked, TRIGGERED_OLD_TRIGGERED);
    }

    bool AttunementModule::OnPreGossipHello(Player* player, Creature* creature)
    {
        if (!player || !creature)
            return false;

        const AttunementModuleConfig* moduleConfig = GetConfig();

        // Auctioneer fallback (formerly handled in the hardcore module): when
        // self-found is enabled, block auction-house gossip with a notification.
        if (!IsAttunementNPC(creature))
        {
            if (moduleConfig->hardcoreEnabled && creature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_AUCTIONEER))
            {
                if (!CanUseAuctionHouse(player, moduleConfig, GetPlayerConfig(player)))
                {
                    std::ostringstream notification;
                    notification << "You can't use the auction house while doing the self found challenge";

                    WorldPacket data;
                    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                    player->SendDirectMessage(data);
                    return true;
                }
            }
            return false;
        }

        // Main NPC: combined attunement + hardcore gossip.
        if (!IsEnabled())
            return false;

#ifdef ENABLE_PLAYERBOTS
        if (sRandomPlayerbotMgr.IsFreeBot(player))
            return false;
#endif

        PlayerMenu* playerMenu = player->GetPlayerMenu();
        if (!playerMenu)
            return false;

        playerMenu->ClearMenus();

        float current = GetXpRate(player->GetGUIDLow());

        // Surface the current rate by tagging the matching preset row with
        // "(current)" — or, if the current rate isn't one of the presets, tag
        // the Custom rate row instead. Avoids a click-to-nothing header row
        // (the original showed "Current XP rate: 1.00x" as a clickable item
        // whose only behaviour was re-rendering the same menu).
        bool currentIsPreset = false;
        for (size_t i = 0; i < PRESET_RATES_COUNT; ++i)
            if (std::abs(PRESET_RATES[i] - current) < 0.01f) { currentIsPreset = true; break; }

        for (size_t i = 0; i < PRESET_RATES_COUNT; ++i)
        {
            char label[64];
            bool isCurrent = std::abs(PRESET_RATES[i] - current) < 0.01f;
            snprintf(label, sizeof(label), "Set rate to %.2gx%s",
                     PRESET_RATES[i], isCurrent ? "  (current)" : "");
            uint32 action = ACTION_RATE_BASE + static_cast<uint32>(PRESET_RATES[i] * 100.0f + 0.5f);
            // INTERACT_1 over BATTLE — setting an XP rate is mundane
            // configuration, not a hero action.
            playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, label, GOSSIP_SENDER_MAIN, action, "", false);
        }

        char customLabel[64];
        if (currentIsPreset)
            snprintf(customLabel, sizeof(customLabel), "Custom rate...");
        else
            snprintf(customLabel, sizeof(customLabel), "Custom rate...  (current: %.2gx)", current);
        playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, customLabel, GOSSIP_SENDER_MAIN, ACTION_CUSTOM_INPUT, "", true);

        if (player->GetLevel() < ATTUNEMENT_MAX_LEVEL)
        {
            // One boost per ACCOUNT: hide the option entirely if any character
            // on this account has already consumed the boost. Persistent across
            // character deletion — the account ledger lives in its own table
            // and is never cleared by character lifecycle events. The
            // account-locked disclosure lives on the confirmation screen
            // (NPC_TEXT_BOOST_CONFIRM) rather than the menu label.
            uint32 accountId = player->GetSession() ? player->GetSession()->GetAccountId() : 0;
            if (!HasAccountBoosted(accountId))
            {
                char boostLabel[64];
                snprintf(boostLabel, sizeof(boostLabel), "Boost me to level %u", ATTUNEMENT_MAX_LEVEL);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_BATTLE, boostLabel, GOSSIP_SENDER_MAIN, ACTION_BOOST_TO_MAX, "", false);
            }
        }

        // Hardcore lives behind a single "Hardcore challenges..." submenu link
        // — keeps the main menu uncluttered and groups the related toggles
        // together. The link only appears when the submenu would actually have
        // content (at least one toggle visible per the level-1 opt-in / always-
        // visible opt-out logic in HasAnyHardcoreOption / ShowHardcoreMenu).
        if (HasAnyHardcoreOption(player, GetPlayerConfig(player)))
            playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_2, "Hardcore challenges...", GOSSIP_SENDER_MAIN, ACTION_HARDCORE_MENU, "", false);

        playerMenu->SendGossipMenu(NPC_TEXT_GREETING, creature->GetObjectGuid());
        return true;
    }

    bool AttunementModule::OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action, const std::string& code, uint32 /*gossipListId*/)
    {
        if (!IsEnabled() || !player || !IsAttunementNPC(creature))
            return false;

        PlayerMenu* playerMenu = player->GetPlayerMenu();
        if (!playerMenu)
            return false;

        // ----- Attunement actions -----

        if (action == ACTION_MAIN_MENU)
        {
            playerMenu->ClearMenus();
            OnPreGossipHello(player, creature);
            return true;
        }

        if (action == ACTION_HARDCORE_MENU)
        {
            ShowHardcoreMenu(player, creature);
            return true;
        }

        if (action == ACTION_CUSTOM_INPUT)
        {
            playerMenu->ClearMenus();
            float rate = static_cast<float>(std::atof(code.c_str()));
            if (!std::isfinite(rate) || rate <= 0.0f)
            {
                player->GetSession()->SendNotification("Invalid rate. Enter a positive number such as 1.5 or 7.");
                playerMenu->CloseGossip();
                return true;
            }
            SetXpRate(player, rate);
            playerMenu->CloseGossip();
            return true;
        }

        if (action == ACTION_BOOST_TO_MAX)
        {
            // Step 1 of two: show confirmation submenu with full disclosure
            // before consuming the account's one-time boost.
            playerMenu->ClearMenus();

            uint32 accountId = player->GetSession() ? player->GetSession()->GetAccountId() : 0;
            if (HasAccountBoosted(accountId))
            {
                player->GetSession()->SendNotification("Another character on your account has already used this boost.");
                playerMenu->CloseGossip();
                return true;
            }

            char yesLabel[96];
            snprintf(yesLabel, sizeof(yesLabel), "Yes, boost me to level %u", ATTUNEMENT_MAX_LEVEL);
            playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_BATTLE,    yesLabel,        GOSSIP_SENDER_MAIN, ACTION_BOOST_CONFIRM, "", false);
            playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, "No, take me back", GOSSIP_SENDER_MAIN, ACTION_MAIN_MENU,     "", false);

            playerMenu->SendGossipMenu(NPC_TEXT_BOOST_CONFIRM, creature->GetObjectGuid());
            return true;
        }

        if (action == ACTION_BOOST_CONFIRM)
        {
            playerMenu->ClearMenus();
            uint32 guid      = player->GetGUIDLow();
            uint32 accountId = player->GetSession() ? player->GetSession()->GetAccountId() : 0;

            // Defense in depth: re-check the account ledger right before we mutate
            // anything. The gossip-side gate already hides the option, but a
            // crafted gossip-select packet could re-enter via ACTION_BOOST_CONFIRM
            // directly. Also closes a TOCTOU between menu render and click.
            if (HasAccountBoosted(accountId))
            {
                player->GetSession()->SendNotification("Another character on your account has already used this boost.");
                playerMenu->CloseGossip();
                return true;
            }

            if (player->GetLevel() < ATTUNEMENT_MAX_LEVEL)
            {
                player->GiveLevel(ATTUNEMENT_MAX_LEVEL);
                player->InitTalentForLevel();
                player->SetUInt32Value(PLAYER_XP, 0);
            }

            // Teach trainer spells (Mail/Plate Specialization, etc.) and cap
            // weapon skills so the auto-equip path that follows doesn't fail
            // with EQUIP_ERR_PROFICIENCY_NEEDED.
            LearnClassSpells(player, ATTUNEMENT_MAX_LEVEL);
            LearnWeaponSkills(player, ATTUNEMENT_MAX_LEVEL);

            player->ModifyMoney(BOOST_GOLD_COPPER);

            uint32 classId = player->getClass();

            // Tier 0 dungeon set (8 pieces of head/shoulders/chest/wrists/hands/waist/legs/feet)
            if (const uint32* gear = GetTierZeroSet(classId))
                for (size_t i = 0; gear[i] != 0; ++i)
                    GiveItem(player, gear[i]);

            // Class-appropriate accessories (cloak, neck, 2 rings, 2 trinkets).
            // Fixed-length walk so empty slots (0) don't terminate iteration.
            if (const uint32* accessories = GetBoostAccessories(classId))
                for (size_t i = 0; i < ACCESSORY_SLOTS; ++i)
                    GiveItem(player, accessories[i]);

            // Class-appropriate weapons (main / off / ranged)
            if (const ClassWeapons* w = GetClassWeapons(classId))
            {
                GiveItem(player, w->mainHand);
                GiveItem(player, w->offHand);
                GiveItem(player, w->ranged);
            }

            // QoL: lvl-60-boosted characters skip the discovery grind.
            // Unlock every faction-appropriate flight path and reveal the
            // entire world map so the character can step straight into 60
            // content (instances, BGs, raids) without re-walking the leveling
            // path to discover taxinodes and zones.
            UnlockAllFlightPaths(player);
            DiscoverAllZones(player);

            CharacterDatabase.PExecute(
                "REPLACE INTO `custom_attunement_player_config` (`guid`, `option_key`, `value`) "
                "VALUES (%u, 'boosted', 1)", guid);

            // Record the boost at the account level so no other character on
            // this account can use it — even after this character is deleted.
            RecordAccountBoost(accountId, player);

            // Force a save immediately so level + gear + money persist together
            // with the boosted flag. Otherwise an early disconnect leaves the
            // boosted flag (direct SQL) committed but the in-memory level
            // change unsaved, locking the character out of re-boosting.
            player->SaveToDB();

            player->GetSession()->SendNotification("Boosted to 60. 500 gold + starter gear equipped, all class spells learned, flight paths unlocked, world map revealed.");
            playerMenu->CloseGossip();
            return true;
        }

        if (action >= ACTION_RATE_BASE && action < HARDCORE_DIALOGUE_OPTION_HARDCORE_CHALLENGE)
        {
            playerMenu->ClearMenus();
            float rate = static_cast<float>(action - ACTION_RATE_BASE) / 100.0f;
            SetXpRate(player, rate);
            playerMenu->CloseGossip();
            return true;
        }

        // ----- Hardcore actions -----

        if (!GetConfig()->hardcoreEnabled)
        {
            playerMenu->CloseGossip();
            return true;
        }

        switch (action)
        {
            case HARDCORE_DIALOGUE_OPTION_HARDCORE_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_HARDCORE_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_HARDCORE_CHALLENGE, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_STOP_HARDCORE_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_HARDCORE_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE_CONFIRM, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_DROP_LOOT_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_DROP_LOOT_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_DROP_LOOT_CHALLENGE, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_STOP_DROP_LOOT_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_DROP_LOOT_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE_CONFIRM, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_LOSE_XP_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_LOSE_XP_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_LOSE_XP_CHALLENGE, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_STOP_LOSE_XP_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_LOSE_XP_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE_CONFIRM, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_SELF_FOUND_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_SELF_FOUND_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_SELF_FOUND_CHALLENGE, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_STOP_SELF_FOUND_CHALLENGE:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_SELF_FOUND_CHALLENGE, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE_CONFIRM, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_HARDCORE_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    if (playerConfig->IsReviveDisabled())
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ALREADY_TAKEN_CHALLENGE, creature->GetObjectGuid());
                    else if (player->GetLevel() == 1)
                    {
                        playerConfig->ToggleReviveDisabled(true);
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ACCEPT_CHALLENGE, creature->GetObjectGuid());
                    }
                    else
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_CANT_TAKE_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_HARDCORE_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    playerConfig->ToggleReviveDisabled(false);
                    playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_DROP_LOOT_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    if (playerConfig->ShouldDropLootOnDeath())
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ALREADY_TAKEN_CHALLENGE, creature->GetObjectGuid());
                    else if (player->GetLevel() == 1)
                    {
                        playerConfig->ToggleDropLootOnDeath(true);
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ACCEPT_CHALLENGE, creature->GetObjectGuid());
                    }
                    else
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_CANT_TAKE_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_DROP_LOOT_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    playerConfig->ToggleDropLootOnDeath(false);
                    playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_LOSE_XP_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    if (playerConfig->ShouldLoseXPOnDeath())
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ALREADY_TAKEN_CHALLENGE, creature->GetObjectGuid());
                    else if (player->GetLevel() == 1)
                    {
                        playerConfig->ToggleLoseXPOnDeath(true);
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ACCEPT_CHALLENGE, creature->GetObjectGuid());
                    }
                    else
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_CANT_TAKE_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_LOSE_XP_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    playerConfig->ToggleLoseXPOnDeath(false);
                    playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT_CHALLENGE + HARDCORE_DIALOGUE_OPTION_SELF_FOUND_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    if (playerConfig->IsSelfFound())
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ALREADY_TAKEN_CHALLENGE, creature->GetObjectGuid());
                    else if (player->GetLevel() == 1)
                    {
                        playerConfig->ToggleSelfFound(true);
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ACCEPT_CHALLENGE, creature->GetObjectGuid());
                    }
                    else
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_CANT_TAKE_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_STOP_SELF_FOUND_CHALLENGE:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    playerConfig->ToggleSelfFound(false);
                    playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_STOP_CHALLENGE, creature->GetObjectGuid());
                }
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_DECLINE_CHALLENGE:
            {
                OnPreGossipHello(player, creature);
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_DISABLE_PVP:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_DISABLE_PVP, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_DISABLE_PVP, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ENABLE_PVP:
            {
                playerMenu->ClearMenus();
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_ACCEPT), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_ENABLE_PVP, "", 0);
                playerMenu->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, player->GetSession()->GetMangosString(HARDCORE_DIALOGUE_OPTION_DECLINE), GOSSIP_SENDER_MAIN, HARDCORE_DIALOGUE_OPTION_DECLINE, "", 0);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ENABLE_PVP, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_DISABLE_PVP:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                {
                    // Same level-1 gate as the other hardcore opt-ins — PvP-off
                    // is a fresh-character decision, not an in-flight toggle.
                    // (Opt-back-in to PvP via ENABLE_PVP has no level gate.)
                    if (player->GetLevel() == 1)
                        playerConfig->TogglePVPDisabled(true);
                    else
                    {
                        playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_CANT_TAKE_CHALLENGE, creature->GetObjectGuid());
                        return true;
                    }
                }
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_DISABLE_PVP_CONFIRM, creature->GetObjectGuid());
                return true;
            }

            case HARDCORE_DIALOGUE_OPTION_ACCEPT + HARDCORE_DIALOGUE_OPTION_ENABLE_PVP:
            {
                playerMenu->ClearMenus();
                if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(player))
                    playerConfig->TogglePVPDisabled(false);
                playerMenu->SendGossipMenu(HARDCORE_DIALOGUE_MESSAGE_ENABLE_PVP_CONFIRM, creature->GetObjectGuid());
                return true;
            }

            default: break;
        }

        playerMenu->CloseGossip();
        return true;
    }

    // ================================================================
    // Hardcore subsystem — Module hook implementations
    // ================================================================

    void AttunementModule::OnCharacterCreated(Player* player)
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->spawnGrave)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();

#ifdef ENABLE_PLAYERBOTS
            Config config;
            if (config.SetSource(SYSCONFDIR"aiplayerbot.conf", ""))
            {
                std::string botPrefix = config.GetStringDefault("AiPlayerbot.RandomBotAccountPrefix", "rndbot");
                std::transform(botPrefix.begin(), botPrefix.end(), botPrefix.begin(), ::toupper);

                uint32 playerAccountId = player->GetSession()->GetAccountId();
                auto result = LoginDatabase.PQuery("SELECT username FROM account WHERE id = '%d'", playerAccountId);
                if (result)
                {
                    Field* fields = result->Fetch();
                    const std::string accountName = fields[0].GetCppString();
                    if (accountName.find(botPrefix) != std::string::npos)
                    {
                        return;
                    }
                }
            }
#endif

            if (m_playerGraves.find(playerId) == m_playerGraves.end())
            {
                m_playerGraves.insert(std::make_pair(playerId, HardcorePlayerGrave::Generate(playerId, player->GetName(), GetConfig())));
            }
        }
    }

    bool AttunementModule::OnPreResurrect(Player* player)
    {
        return IsReviveDisabled(player, GetConfig(), GetPlayerConfig(player));
    }

    void AttunementModule::OnResurrect(Player* player)
    {
        Unit* killer = GetKiller(player);
        LevelDown(player, killer);
        SetKiller(player, nullptr);
    }

    void AttunementModule::OnDeath(Player* player, Unit* killer)
    {
        if (GetConfig()->hardcoreEnabled && player && killer)
        {
            if (killer && killer->IsCreature() && killer->GetOwner())
            {
                killer = killer->GetOwner();
            }

            SetKiller(player, killer);

            CreateLoot(player, killer);
            CreateGrave(player, killer);

            if (IsReviveDisabled(player, GetConfig(), GetPlayerConfig(player)))
            {
                m_deathLog.OnDeath(player, GetConfig(), killer);
            }
        }
    }

    void AttunementModule::OnDeath(Player* player, uint8 environmentalDamageType)
    {
        if (GetConfig()->hardcoreEnabled && player)
        {
            Unit* killer = nullptr;
            SetKiller(player, killer);

            CreateLoot(player, killer);
            CreateGrave(player, killer);

            if (IsReviveDisabled(player, GetConfig(), GetPlayerConfig(player)))
            {
                m_deathLog.OnDeath(player, GetConfig(), killer, environmentalDamageType);
            }
        }
    }

    void AttunementModule::OnReleaseSpirit(Player* player, const WorldSafeLocsEntry* closestGrave)
    {
        const bool teleportedToGraveyard = closestGrave != nullptr;
        if (player && teleportedToGraveyard && ShouldReviveOnGraveyard(player, GetConfig(), GetPlayerConfig(player)))
        {
            player->ResurrectPlayer(1.0f);
            player->SpawnCorpseBones();

            const SpellEntry* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(1020);
            if (spellInfo)
            {
                SpellAuraHolder* holder = CreateSpellAuraHolder(spellInfo, player, player);
                for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                {
                    uint8 eff = spellInfo->Effect[i];
                    if (eff >= MAX_SPELL_EFFECTS)
                    {
                        continue;
                    }

                    if (IsAreaAuraEffect(eff) ||
                        eff == SPELL_EFFECT_APPLY_AURA ||
                        eff == SPELL_EFFECT_PERSISTENT_AREA_AURA)
                    {
                        int32 basePoints = spellInfo->CalculateSimpleValue(SpellEffectIndex(i));
                        int32 damage = basePoints;
                        Aura* aur = CreateAura(spellInfo, SpellEffectIndex(i), &damage, &basePoints, holder, player);
                        holder->AddAura(aur, SpellEffectIndex(i));
                    }
                }

                if (!player->AddSpellAuraHolder(holder))
                {
                    delete holder;
                }
            }
        }
    }

    void AttunementModule::PreLoadLoot()
    {
        if (GetConfig()->IsDropLootEnabled())
        {
            auto result = CharacterDatabase.Query("SELECT id, player, loot_id FROM custom_hardcore_loot_gameobjects");
            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    const uint32 gameObjectId = fields[0].GetUInt32();
                    const uint32 playerId = fields[1].GetUInt32();
                    const uint32 lootId = fields[2].GetUInt32();

                    if (m_playersLoot.find(playerId) == m_playersLoot.end())
                    {
                        m_playersLoot.insert(std::make_pair(playerId, std::map<uint32, HardcorePlayerLoot>()));
                    }

                    auto& playerLoots = m_playersLoot.at(playerId);
                    if (playerLoots.find(lootId) == playerLoots.end())
                    {
                        playerLoots.insert(std::make_pair(lootId, HardcorePlayerLoot(lootId, playerId, this)));
                    }

                    HardcorePlayerLoot& playerLoot = playerLoots.at(lootId);
                    playerLoot.LoadGameObject(gameObjectId);
                }
                while (result->NextRow());
            }
        }
    }

    void AttunementModule::LoadLoot()
    {
        if (GetConfig()->IsDropLootEnabled())
        {
            for (auto& pair : m_playersLoot)
            {
                for (auto& pair2 : pair.second)
                {
                    pair2.second.Spawn();
                }
            }
        }
    }

    void AttunementModule::GenerateMissingGraves()
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->spawnGrave)
        {
            auto result = CharacterDatabase.Query("SELECT guid, account, name FROM characters");
            if (result)
            {
                do
                {
                    bool canGenerateGrave = true;
                    Field* fields = result->Fetch();
                    const uint32 playerId = fields[0].GetUInt32();
                    const uint32 playerAccountId = fields[1].GetUInt32();
                    const std::string playerName = fields[2].GetCppString();

#ifdef ENABLE_PLAYERBOTS
                    Config config;
                    if (config.SetSource(SYSCONFDIR"aiplayerbot.conf", ""))
                    {
                        std::string botPrefix = config.GetStringDefault("AiPlayerbot.RandomBotAccountPrefix", "rndbot");
                        std::transform(botPrefix.begin(), botPrefix.end(), botPrefix.begin(), ::toupper);

                        auto accResult = LoginDatabase.PQuery("SELECT username FROM account WHERE id = '%d'", playerAccountId);
                        if (accResult)
                        {
                            Field* accFields = accResult->Fetch();
                            const std::string accountName = accFields[0].GetCppString();
                            if (accountName.find(botPrefix) != std::string::npos)
                            {
                                canGenerateGrave = false;
                            }
                        }
                    }
#endif

                    if (canGenerateGrave && m_playerGraves.find(playerId) == m_playerGraves.end())
                    {
                        m_playerGraves.insert(std::make_pair(playerId, HardcorePlayerGrave::Generate(playerId, playerName, GetConfig())));
                    }
                }
                while (result->NextRow());
            }
        }
    }

    HardcorePlayerConfig* AttunementModule::GetPlayerConfig(uint32 playerId)
    {
        HardcorePlayerConfig* playerManager = nullptr;
        if (GetConfig()->hardcoreEnabled && GetConfig()->playerConfig && playerId > 0)
        {
            auto playerManagerIt = m_playerManagers.find(playerId);
            if (playerManagerIt != m_playerManagers.end())
            {
                playerManager = &playerManagerIt->second;
            }
            else
            {
                bool isValidPlayer = true;
#ifdef ENABLE_PLAYERBOTS
                const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, playerId);
                if (const Player* player = sObjectMgr.GetPlayer(playerGUID))
                {
                    if (sRandomPlayerbotMgr.IsFreeBot(playerId) || !player->isRealPlayer())
                    {
                        isValidPlayer = false;
                    }
                }
                else
                {
                    isValidPlayer = false;
                }
#endif

                if (isValidPlayer)
                {
                    m_playerManagers.insert(std::make_pair(playerId, HardcorePlayerConfig::Load(playerId)));
                    playerManager = &m_playerManagers.find(playerId)->second;
                }
            }
        }

        return playerManager;
    }

    HardcorePlayerConfig* AttunementModule::GetPlayerConfig(const Player* player)
    {
        const uint32 playerId = player ? player->GetObjectGuid().GetCounter() : 0;
        return GetPlayerConfig(playerId);
    }

    HardcoreLootGameObject* AttunementModule::FindLootGOByGUID(const uint32 guid)
    {
        for (auto it = m_playersLoot.begin(); it != m_playersLoot.end(); ++it)
        {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            {
                HardcoreLootGameObject* lootGameObject = it2->second.FindGameObjectByGUID(guid);
                if (lootGameObject)
                {
                    return lootGameObject;
                }
            }
        }

        return nullptr;
    }

    HardcorePlayerLoot* AttunementModule::FindLootByID(const uint32 playerId, const uint32 lootId)
    {
        auto playerLootsIt = m_playersLoot.find(playerId);
        if (playerLootsIt != m_playersLoot.end())
        {
            std::map<uint32, HardcorePlayerLoot>& playerLoots = playerLootsIt->second;
            auto playerLootIt = playerLoots.find(lootId);
            if (playerLootIt != playerLoots.end())
            {
                return &playerLootIt->second;
            }
        }

        return nullptr;
    }

    void AttunementModule::CreateLoot(Player* player, Unit* killer)
    {
        if (player && ShouldDropLoot(player, killer, GetConfig(), GetPlayerConfig(player)))
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();

            auto playerLootsIt = m_playersLoot.find(playerId);
            if (playerLootsIt != m_playersLoot.end())
            {
                std::map<uint32, HardcorePlayerLoot>& playerLoots = playerLootsIt->second;
                if (playerLoots.size() >= GetMaxPlayerLoot(GetConfig()))
                {
                    HardcorePlayerLoot& playerLoot = playerLoots.begin()->second;
                    RemoveLoot(playerLoot.GetPlayerId(), playerLoot.GetId());
                }
            }

            if (m_playersLoot.find(playerId) == m_playersLoot.end())
            {
                m_playersLoot.insert(std::make_pair(playerId, std::map<uint32, HardcorePlayerLoot>()));
            }

            std::map<uint32, HardcorePlayerLoot>& playerLoots = m_playersLoot.at(playerId);

            uint32 newLootId = 1;
            auto result = CharacterDatabase.PQuery("SELECT loot_id FROM custom_hardcore_loot_gameobjects WHERE player = '%d' ORDER BY loot_id DESC LIMIT 1", playerId);
            if (result)
            {
                Field* fields = result->Fetch();
                newLootId = fields[0].GetUInt32() + 1;
            }

            playerLoots.insert(std::make_pair(newLootId, HardcorePlayerLoot(newLootId, playerId, this)));
            HardcorePlayerLoot& playerLoot = playerLoots.at(newLootId);
            if (!playerLoot.Create())
            {
                playerLoots.erase(newLootId);
            }
        }
    }

    bool AttunementModule::RemoveLoot(uint32 playerId, uint32 lootId)
    {
        HardcorePlayerLoot* playerLoot = FindLootByID(playerId, lootId);
        if (playerLoot)
        {
            std::map<uint32, HardcorePlayerLoot>& playerLoots = m_playersLoot.at(playerId);
            playerLoot->Destroy();
            playerLoots.erase(lootId);

            if (playerLoots.empty())
            {
                m_playersLoot.erase(playerId);
            }

            return true;
        }

        return false;
    }

    void AttunementModule::RemoveAllLoot()
    {
        for (auto& pair : m_playersLoot)
        {
            for (auto& pair2 : pair.second)
            {
                pair2.second.Destroy();
            }
        }

        m_playersLoot.clear();
    }

    bool AttunementModule::OnFillLoot(Loot* loot, Player* /*owner*/)
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->IsDropLootEnabled())
        {
            if (loot && loot->GetLootTarget() && loot->GetLootTarget()->IsGameObject())
            {
                const HardcoreLootGameObject* lootGameObject = FindLootGOByGUID(loot->GetLootTarget()->GetGUIDLow());
                if (lootGameObject)
                {
                    for (const HardcoreLootItem& item : lootGameObject->GetItems())
                    {
                        loot->AddItem(item.m_id, item.m_amount, 0, item.m_randomPropertyId);
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool AttunementModule::OnGenerateMoneyLoot(Loot* loot, uint32& outMoney)
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->IsDropLootEnabled())
        {
            if (loot && loot->GetLootTarget() && loot->GetLootTarget()->IsGameObject())
            {
                HardcoreLootGameObject* lootGameObject = FindLootGOByGUID(loot->GetLootTarget()->GetGUIDLow());
                if (lootGameObject)
                {
                    outMoney = lootGameObject->GetMoney();
                    return true;
                }
            }
        }

        return false;
    }

    void AttunementModule::OnAddItem(Loot* loot, LootItem* lootItem)
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->IsDropLootEnabled())
        {
            if (loot && lootItem && loot->GetLootTarget() && loot->GetLootTarget()->IsGameObject())
            {
                HardcoreLootGameObject* lootGameObject = FindLootGOByGUID(loot->GetLootTarget()->GetGUIDLow());
                if (lootGameObject)
                {
                    lootItem->allowedGuid.clear();
                }
            }
        }
    }

    void AttunementModule::OnSendGold(Loot* loot, Player* /*player*/, uint32 /*gold*/, uint8 /*lootMethod*/)
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->IsDropLootEnabled())
        {
            if (loot && loot->GetLootTarget() && loot->GetLootTarget()->IsGameObject())
            {
                HardcoreLootGameObject* lootGameObject = FindLootGOByGUID(loot->GetLootTarget()->GetGUIDLow());
                if (lootGameObject)
                {
                    lootGameObject->SetMoney(0);
                }
            }
        }
    }

    bool AttunementModule::OnPreInviteMember(Group* /*group*/, Player* player, Player* recipient)
    {
        const AttunementModuleConfig* moduleConfig = GetConfig();
        if (moduleConfig->hardcoreEnabled)
        {
            if (!CanInviteToGroup(player, recipient, moduleConfig, GetPlayerConfig(player), GetPlayerConfig(recipient)))
            {
                std::ostringstream notification;
                notification << "You can't invite other players that are not doing the same challenges as you";

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);
                return true;
            }
        }

        return false;
    }

    void AttunementModule::OnStoreItem(Player* /*player*/, Loot* loot, Item* item)
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->IsDropLootEnabled())
        {
            if (loot && item && loot->GetLootTarget() && loot->GetLootTarget()->IsGameObject())
            {
                HardcoreLootGameObject* lootGameObject = FindLootGOByGUID(loot->GetLootTarget()->GetGUIDLow());
                if (lootGameObject)
                {
                    const HardcoreLootItem* hardcoreItem = lootGameObject->GetItem(item->GetProto()->ItemId);
                    if (hardcoreItem)
                    {
                        if (!hardcoreItem->m_enchantments.empty() || (hardcoreItem->m_durability > 0))
                        {
                            if (!hardcoreItem->m_enchantments.empty())
                            {
#if EXPANSION == 0
                                item->_LoadIntoDataField(hardcoreItem->m_enchantments.c_str(), ITEM_FIELD_ENCHANTMENT, MAX_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET);
#else
                                item->_LoadIntoDataField(hardcoreItem->m_enchantments.c_str(), ITEM_FIELD_ENCHANTMENT_1_1, MAX_ENCHANTMENT_SLOT * MAX_ENCHANTMENT_OFFSET);
#endif
                            }

                            if (hardcoreItem->m_durability > 0)
                            {
                                item->SetUInt32Value(ITEM_FIELD_DURABILITY, hardcoreItem->m_durability);
                            }
                        }

                        if (lootGameObject->RemoveItem(hardcoreItem->m_id))
                        {
                            if (!lootGameObject->HasItems())
                            {
                                const uint32 lootId = lootGameObject->GetLootId();
                                const uint32 playerId = lootGameObject->GetPlayerId();

                                HardcorePlayerLoot* playerLoot = FindLootByID(playerId, lootId);
                                if (playerLoot)
                                {
                                    if (playerLoot->RemoveGameObject(lootGameObject->GetId()))
                                    {
                                        if (!playerLoot->HasGameObjects())
                                        {
                                            RemoveLoot(playerId, lootId);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    bool AttunementModule::OnPreHandleInitializeTrade(Player* player, Player* trader)
    {
        const AttunementModuleConfig* moduleConfig = GetConfig();
        if (moduleConfig->hardcoreEnabled && moduleConfig->selfFound)
        {
            if (!CanTrade(player, trader, moduleConfig, GetPlayerConfig(player), GetPlayerConfig(trader)))
            {
                std::ostringstream notification;
                notification << "You can't trade with other players that are not doing the same challenges as you";

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);
                return true;
            }
        }

        return false;
    }

    bool AttunementModule::OnGetReactionTo(const Unit* unit, const Unit* target, ReputationRank& outReaction)
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->disablePVP && !m_getReactionToInternal)
        {
            if (unit && target && unit->IsPlayer() && target->IsPlayer())
            {
                const Player* player = static_cast<const Player*>(unit);
                const Player* playerTarget = static_cast<const Player*>(target);

                m_getReactionToInternal = true;
                outReaction = unit->GetReactionTo(target);
                m_getReactionToInternal = false;

                if (outReaction == REP_HOSTILE)
                {
                    const HardcorePlayerConfig* playerConfig = GetPlayerConfig(player);
                    const bool playerDisabledPvp = playerConfig && playerConfig->IsPVPDisabled();

                    const HardcorePlayerConfig* targetConfig = GetPlayerConfig(playerTarget);
                    const bool targetDisabledPvp = targetConfig && targetConfig->IsPVPDisabled();

                    if (playerDisabledPvp || targetDisabledPvp)
                    {
                        outReaction = REP_FRIENDLY;
                    }
                }

                return true;
            }
        }

        return false;
    }

    bool AttunementModule::OnCanCheckMailBox(Player* player, const ObjectGuid& /*mailboxGuid*/, bool& outResult)
    {
        const AttunementModuleConfig* moduleConfig = GetConfig();
        if (moduleConfig->hardcoreEnabled && moduleConfig->selfFound)
        {
            if (!CanUseMailBox(player, moduleConfig, GetPlayerConfig(player)))
            {
                std::ostringstream notification;
                notification << "You can't use the mailbox during the self found challenge";

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);

                outResult = false;
                return true;
            }
        }

        return false;
    }

    void AttunementModule::PreLoadGraves()
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->spawnGrave)
        {
            auto result = WorldDatabase.PQuery("SELECT entry, data10 FROM gameobject_template WHERE type = '%d' AND CustomData1 = '%d'", 2, 3643);
            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    const uint32 gameObjectEntry = fields[0].GetUInt32();
                    const uint32 playerId = fields[1].GetUInt32();

                    if (m_playerGraves.find(playerId) == m_playerGraves.end())
                    {
                        m_playerGraves.insert(std::make_pair(playerId, HardcorePlayerGrave::Load(playerId, gameObjectEntry, GetConfig())));
                    }
                }
                while (result->NextRow());
            }

            GenerateMissingGraves();
        }
    }

    void AttunementModule::LoadGraves()
    {
        if (GetConfig()->hardcoreEnabled && GetConfig()->spawnGrave)
        {
            for (auto& pair : m_playerGraves)
            {
                pair.second.Spawn();
            }
        }
    }

    void AttunementModule::CreateGrave(Player* player, Unit* killer)
    {
        if (player && ShouldSpawnGrave(player, killer, GetConfig(), GetPlayerConfig(player)))
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            auto it = m_playerGraves.find(playerId);
            if (it != m_playerGraves.end())
            {
                it->second.Create();
            }
        }
    }

    void AttunementModule::RemoveAllGraves()
    {
        for (auto& pair : m_playerGraves)
        {
            pair.second.Destroy();
        }

        m_playerGraves.clear();
    }

    void AttunementModule::LevelDown(Player* player, Unit* killer)
    {
        if (player && ShouldLevelDown(player, killer, GetConfig(), GetPlayerConfig(player)))
        {
            const float levelDownRate = GetConfig()->levelDownPct;
            uint32 totalLevelXP = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
            uint32 curXP = player->GetUInt32Value(PLAYER_XP);
            totalLevelXP = totalLevelXP ? totalLevelXP : 1;
            const float levelPct = (float)(curXP) / totalLevelXP;
            const float level = player->GetLevel() + levelPct;
            const float levelDown = level - levelDownRate;

            double newLevel, newXPpct;
            newXPpct = modf(levelDown, &newLevel);
            newLevel = newLevel < 0.0 ? 1.0 : newLevel;

            if (newLevel > 0.0)
            {
                player->GiveLevel((uint32)newLevel);
                player->InitTalentForLevel();
                player->SetUInt32Value(PLAYER_XP, 0);
            }

            if (newXPpct > 0.0)
            {
                curXP = player->GetUInt32Value(PLAYER_XP);
                totalLevelXP = sObjectMgr.GetXPForLevel(newLevel);

                const uint32 levelXP = (uint32)(totalLevelXP * newXPpct);
                player->SetUInt32Value(PLAYER_XP, levelXP);
            }
        }
    }

    Unit* AttunementModule::GetKiller(Player* player) const
    {
        Unit* killer = nullptr;
        if (player)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return nullptr;
#endif

            const uint32 playerGuid = player->GetObjectGuid().GetCounter();
            if (m_lastPlayerDeaths.find(playerGuid) != m_lastPlayerDeaths.end())
            {
                const ObjectGuid& killerGuid = m_lastPlayerDeaths.at(playerGuid);
                return sObjectAccessor.GetUnit(*player, killerGuid);
            }
        }

        return killer;
    }

    void AttunementModule::SetKiller(Player* player, Unit* killer)
    {
        if (player)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return;
#endif

            const uint32 playerGuid = player->GetObjectGuid().GetCounter();
            const ObjectGuid killerGuid = killer ? killer->GetObjectGuid() : ObjectGuid();
            m_lastPlayerDeaths[playerGuid] = killerGuid;
        }
    }

    // ================================================================
    // Chat commands (hardcore subsystem, prefix '.attunement')
    // ================================================================

    std::vector<ModuleChatCommand>* AttunementModule::GetCommandTable()
    {
        static std::vector<ModuleChatCommand> commandTable =
        {
            { "reset",       std::bind(&AttunementModule::HandleResetCommand,            this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "resetgraves", std::bind(&AttunementModule::HandleResetGravesCommand,      this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "resetloot",   std::bind(&AttunementModule::HandleResetLootCommand,        this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "spawnloot",   std::bind(&AttunementModule::HandleSpawnLootCommand,        this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "spawngrave",  std::bind(&AttunementModule::HandleSpawnGraveCommand,       this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "leveldown",   std::bind(&AttunementModule::HandleLevelDownCommand,        this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "revive",      std::bind(&AttunementModule::HandleToggleReviveCommand,     this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "droploot",    std::bind(&AttunementModule::HandleToggleDropLootCommand,   this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "losexp",      std::bind(&AttunementModule::HandleToggleLoseXPCommand,     this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "pvp",         std::bind(&AttunementModule::HandleTogglePVPCommand,        this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "selffound",   std::bind(&AttunementModule::HandleToggleSelfFoundCommand,  this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "deathlog",    std::bind(&AttunementModule::HandleDeathlogCommand,         this, std::placeholders::_1, std::placeholders::_2), SEC_PLAYER }
        };

        return &commandTable;
    }

    bool AttunementModule::HandleResetCommand(WorldSession* /*session*/, const std::string& /*args*/)
    {
        if (GetConfig()->hardcoreEnabled)
        {
            RemoveAllLoot();
            RemoveAllGraves();
            return true;
        }

        return false;
    }

    bool AttunementModule::HandleResetGravesCommand(WorldSession* /*session*/, const std::string& /*args*/)
    {
        if (GetConfig()->hardcoreEnabled)
        {
            RemoveAllGraves();
            return true;
        }

        return false;
    }

    bool AttunementModule::HandleResetLootCommand(WorldSession* /*session*/, const std::string& /*args*/)
    {
        if (GetConfig()->hardcoreEnabled)
        {
            RemoveAllLoot();
            return true;
        }

        return false;
    }

    bool AttunementModule::HandleSpawnLootCommand(WorldSession* session, const std::string& /*args*/)
    {
        if (GetConfig()->hardcoreEnabled)
        {
            if (session && session->GetPlayer())
            {
                CreateLoot(session->GetPlayer(), nullptr);
                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleSpawnGraveCommand(WorldSession* session, const std::string& /*args*/)
    {
        if (GetConfig()->hardcoreEnabled)
        {
            if (session && session->GetPlayer())
            {
                CreateGrave(session->GetPlayer());
                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleLevelDownCommand(WorldSession* session, const std::string& /*args*/)
    {
        if (GetConfig()->hardcoreEnabled)
        {
            if (session && session->GetPlayer())
            {
                LevelDown(session->GetPlayer());
                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleToggleReviveCommand(WorldSession* session, const std::string& args)
    {
        const Player* player = session ? session->GetPlayer() : nullptr;
        if (player && !args.empty())
        {
            const bool enable = args == "1" || args == "true" ? true : false;

            const Player* target = player;
            const ObjectGuid& guid = player->GetSelectionGuid();
            if (guid)
            {
                target = sObjectMgr.GetPlayer(guid);
            }

            if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(target))
            {
                playerConfig->ToggleReviveDisabled(!enable);

                std::ostringstream notification;
                notification << "Revive has been " << (enable ? "enabled" : "disabled") << " for the player " << target->GetName();

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);

                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleToggleDropLootCommand(WorldSession* session, const std::string& args)
    {
        const Player* player = session ? session->GetPlayer() : nullptr;
        if (player && !args.empty())
        {
            const bool enable = args == "1" || args == "true" ? true : false;

            const Player* target = player;
            const ObjectGuid& guid = player->GetSelectionGuid();
            if (guid)
            {
                target = sObjectMgr.GetPlayer(guid);
            }

            if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(target))
            {
                playerConfig->ToggleDropLootOnDeath(enable);

                std::ostringstream notification;
                notification << "Drop loot on death has been " << (enable ? "enabled" : "disabled") << " for the player " << target->GetName();

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);

                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleToggleLoseXPCommand(WorldSession* session, const std::string& args)
    {
        const Player* player = session ? session->GetPlayer() : nullptr;
        if (player && !args.empty())
        {
            const bool enable = args == "1" || args == "true" ? true : false;

            const Player* target = player;
            const ObjectGuid& guid = player->GetSelectionGuid();
            if (guid)
            {
                target = sObjectMgr.GetPlayer(guid);
            }

            if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(target))
            {
                playerConfig->ToggleLoseXPOnDeath(enable);

                std::ostringstream notification;
                notification << "Lose XP on death has been " << (enable ? "enabled" : "disabled") << " for the player " << target->GetName();

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);

                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleTogglePVPCommand(WorldSession* session, const std::string& args)
    {
        const Player* player = session ? session->GetPlayer() : nullptr;
        if (player && !args.empty())
        {
            const bool enable = args == "1" || args == "true" ? true : false;

            const Player* target = player;
            const ObjectGuid& guid = player->GetSelectionGuid();
            if (guid)
            {
                target = sObjectMgr.GetPlayer(guid);
            }

            if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(target))
            {
                playerConfig->TogglePVPDisabled(!enable);

                std::ostringstream notification;
                notification << "PVP has been " << (enable ? "enabled" : "disabled") << " for the player " << target->GetName();

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);

                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleToggleSelfFoundCommand(WorldSession* session, const std::string& args)
    {
        const Player* player = session ? session->GetPlayer() : nullptr;
        if (player && !args.empty())
        {
            const bool enable = args == "1" || args == "true" ? true : false;

            const Player* target = player;
            const ObjectGuid& guid = player->GetSelectionGuid();
            if (guid)
            {
                target = sObjectMgr.GetPlayer(guid);
            }

            if (HardcorePlayerConfig* playerConfig = GetPlayerConfig(target))
            {
                playerConfig->ToggleSelfFound(enable);

                std::ostringstream notification;
                notification << "Self found has been " << (enable ? "enabled" : "disabled") << " for the player " << target->GetName();

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, notification.str().c_str());
                player->SendDirectMessage(data);

                return true;
            }
        }

        return false;
    }

    bool AttunementModule::HandleDeathlogCommand(WorldSession* session, const std::string& args)
    {
        const Player* player = session ? session->GetPlayer() : nullptr;
        if (player && GetConfig()->hardcoreEnabled)
        {
            int amount = 5;
            HardcoreDeathFilter filter = HARDCORE_DEATH_FILTER_WORLD;
            uint32 accountId = session->GetAccountId();
            std::string playerName = "";

            if (!args.empty())
            {
                const auto arguments = helper::SplitString(args, " ");

                for (size_t i = 0; i < arguments.size(); ++i)
                {
                    const auto& argument = arguments[i];
                    if (helper::IsValidNumberString(argument))
                    {
                        amount = std::stoi(argument);
                    }
                    else if (argument == "account")
                    {
                        filter = HARDCORE_DEATH_FILTER_ACCOUNT;
                    }
                    else if (argument == "player" && (i + 1 < arguments.size() - 1))
                    {
                        filter = HARDCORE_DEATH_FILTER_PLAYER;
                        playerName = arguments[i + 1];
                        i++;
                    }
                    else
                    {
                        filter = HARDCORE_DEATH_FILTER_PLAYER;
                        playerName = argument;
                    }
                }
            }

            const auto entries = m_deathLog.GetEntries(filter, amount, accountId, playerName);
            if (entries.empty())
            {
                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, "No entries have been found");
                player->SendDirectMessage(data);
            }
            else
            {
                std::ostringstream message;
                message << "Showing the " << amount << " most recent death entries";

                if (filter == HARDCORE_DEATH_FILTER_WORLD)
                    message << " of the world:";
                else if (filter == HARDCORE_DEATH_FILTER_ACCOUNT)
                    message << " of your account:";
                else if (filter == HARDCORE_DEATH_FILTER_PLAYER)
                    message << " for the player " << playerName << ":";

                WorldPacket data;
                ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, message.str().c_str());
                player->SendDirectMessage(data);

                for (const HardcorePlayerDeathLogEntry* entry : entries)
                {
                    WorldPacket entryData;
                    const std::string entryMessage = entry->GetMessage(player);
                    ChatHandler::BuildChatPacket(entryData, CHAT_MSG_SYSTEM, entryMessage.c_str());
                    player->SendDirectMessage(entryData);
                }
            }

            return true;
        }

        return false;
    }
}
