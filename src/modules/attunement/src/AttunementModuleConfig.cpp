#include "AttunementModuleConfig.h"
#include "Server/DBCEnums.h"

namespace cmangos_module
{
    AttunementModuleConfig::AttunementModuleConfig()
    : ModuleConfig("attunement.conf")
    , enabled(false)
    , defaultRate(1.0f)
    , minRate(0.1f)
    , maxRate(0.0f) // 0 = uncapped
    , auraTier1SpellId(0)
    , auraTier2SpellId(0)
    , auraTier3SpellId(0)
    , auraTier4SpellId(0)
    , auraTier5SpellId(0)
    , hardcoreEnabled(false)
    , playerConfig(false)
    , broadcastDeathGuild(false)
    , broadcastDeathWorld(false)
    , spawnGrave(false)
    , graveGameObjectId(0U)
    , graveMessage("")
    , removeGraveOnCharacterDeleted(true)
    , dropGearPct(0.0f)
    , dropItemsPct(0.0f)
    , dropMoneyPct(0.0f)
#ifdef ENABLE_PLAYERBOTS
    , botDropGearPct(0.0f)
    , botDropItemsPct(0.0f)
    , botDropMoneyPct(0.0f)
#endif
    , removeLootOnCharacterDeleted(true)
    , lootGameObjectId(0U)
    , reviveDisabled(false)
    , reviveOnGraveyard(false)
    , levelDownPct(0.0f)
    , maxDroppedLoot(0U)
    , dropOnDungeons(false)
    , dropOnRaids(false)
    , levelDownOnDungeons(false)
    , levelDownOnRaids(false)
    , dropMinLevel(0)
    , dropMaxLevel(0)
    , levelDownMinLevel(0)
    , levelDownMaxLevel(0)
    , disablePVP(false)
    , selfFound(false)
    {
    }

    bool AttunementModuleConfig::OnLoad()
    {
        enabled          = config.GetBoolDefault("Attunement.Enable", false);
        defaultRate      = config.GetFloatDefault("Attunement.DefaultRate", 1.0f);
        minRate          = config.GetFloatDefault("Attunement.MinRate", 0.1f);
        maxRate          = config.GetFloatDefault("Attunement.MaxRate", 0.0f);
        auraTier1SpellId = config.GetIntDefault("Attunement.Aura.Tier1.SpellId", 0);
        auraTier2SpellId = config.GetIntDefault("Attunement.Aura.Tier2.SpellId", 0);
        auraTier3SpellId = config.GetIntDefault("Attunement.Aura.Tier3.SpellId", 0);
        auraTier4SpellId = config.GetIntDefault("Attunement.Aura.Tier4.SpellId", 0);
        auraTier5SpellId = config.GetIntDefault("Attunement.Aura.Tier5.SpellId", 0);

        hardcoreEnabled = config.GetBoolDefault("Hardcore.Enable", false);
        playerConfig = config.GetBoolDefault("Hardcore.PlayerConfig", false);
        broadcastDeathGuild = config.GetBoolDefault("Hardcore.BroadcastDeathGuild", false);
        broadcastDeathWorld = config.GetBoolDefault("Hardcore.BroadcastDeathWorld", false);
        spawnGrave = config.GetBoolDefault("Hardcore.SpawnGrave", false);
        graveGameObjectId = config.GetIntDefault("Hardcore.GraveGameObjectID", 0U);
        graveMessage = config.GetStringDefault("Hardcore.GraveMessage", "Here lies <PlayerName>");
        removeGraveOnCharacterDeleted = config.GetBoolDefault("Hardcore.RemoveGravesOnCharacterDeleted", true);
        dropGearPct = config.GetFloatDefault("Hardcore.DropGear", 0.0f);
        dropItemsPct = config.GetFloatDefault("Hardcore.DropItems", 0.0f);
        dropMoneyPct = config.GetFloatDefault("Hardcore.DropMoney", 0.0f);
#ifdef ENABLE_PLAYERBOTS
        botDropGearPct = config.GetFloatDefault("Hardcore.BotDropGear", 0.0f);
        botDropItemsPct = config.GetFloatDefault("Hardcore.BotDropItems", 0.0f);
        botDropMoneyPct = config.GetFloatDefault("Hardcore.BotDropMoney", 0.0f);
#endif
        removeLootOnCharacterDeleted = config.GetBoolDefault("Hardcore.RemoveLootOnCharacterDeleted", true);
        lootGameObjectId = config.GetIntDefault("Hardcore.LootGameObjectID", 0U);
        reviveDisabled = config.GetBoolDefault("Hardcore.ReviveDisabled", false);
        reviveOnGraveyard = config.GetBoolDefault("Hardcore.ReviveOnGraveyard", false);
        levelDownPct = config.GetFloatDefault("Hardcore.LevelDown", 0.0f);
        maxDroppedLoot = config.GetIntDefault("Hardcore.MaxPlayerLoot", 0U);
        dropOnDungeons = config.GetBoolDefault("Hardcore.DropOnDungeons", false);
        dropOnRaids = config.GetBoolDefault("Hardcore.DropOnRaids", false);
        levelDownOnDungeons = config.GetBoolDefault("Hardcore.LevelDownOnDungeons", false);
        levelDownOnRaids = config.GetBoolDefault("Hardcore.LevelDownOnRaids", false);
        dropMinLevel = config.GetIntDefault("Hardcore.DropMinLevel", 1);
        dropMaxLevel = config.GetIntDefault("Hardcore.DropMaxLevel", DEFAULT_MAX_LEVEL);
        levelDownMinLevel = config.GetIntDefault("Hardcore.LevelDownMinLevel", 1);
        levelDownMaxLevel = config.GetIntDefault("Hardcore.LevelDownMaxLevel", DEFAULT_MAX_LEVEL);
        disablePVP = config.GetBoolDefault("Hardcore.DisablePVP", false);
        selfFound = config.GetBoolDefault("Hardcore.SelfFound", false);
        return true;
    }

    bool AttunementModuleConfig::IsDropLootEnabled() const
    {
        return dropGearPct > 0.0f ||
#ifdef ENABLE_PLAYERBOTS
               botDropGearPct > 0.0f ||
               botDropItemsPct > 0.0f ||
               botDropMoneyPct > 0.0f ||
#endif
               dropItemsPct > 0.0f ||
               dropMoneyPct > 0.0f;
    }
}
