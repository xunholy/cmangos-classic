#ifndef CMANGOS_MODULE_ATTUNEMENT_H
#define CMANGOS_MODULE_ATTUNEMENT_H

#include "Module.h"
#include "AttunementModuleConfig.h"
#include "AttunementHardcore.h"
#include "AttunementDualspec.h"

#include "Entities/ObjectGuid.h"

#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace cmangos_module
{
    class AttunementModule : public Module
    {
        friend HardcorePlayerLoot;

    public:
        AttunementModule();
        const AttunementModuleConfig* GetConfig() const override;

        // Module hooks
        void OnInitialize() override;
        void OnWorldPreInitialized() override;

        // Player hooks (attunement)
        void OnLoadFromDB(Player* player) override;
        void OnLogOut(Player* player) override;
        bool OnPreGiveXP(Player* player, uint32& xp, Creature* victim) override;
        void OnRegenerate(Player* player, uint8 power, uint32 diff, float& addedValue) override;

        // Player hooks (hardcore)
        void OnCharacterCreated(Player* player) override;
        void OnDeleteFromDB(uint32 playerId) override;
        bool OnPreResurrect(Player* player) override;
        void OnResurrect(Player* player) override;
        void OnDeath(Player* player, Unit* killer) override;
        void OnDeath(Player* player, uint8 environmentalDamageType) override;
        void OnReleaseSpirit(Player* player, const WorldSafeLocsEntry* closestGrave) override;
        void OnStoreItem(Player* player, Loot* loot, Item* item) override;
        bool OnPreHandleInitializeTrade(Player* player, Player* trader) override;

        // Player hooks (dualspec — formerly the dualspec module)
        bool OnUseItem(Player* player, Item* item) override;
        void OnLearnTalent(Player* player, uint32 spellId) override;
        void OnResetTalents(Player* player, uint32 cost) override;
        void OnPreLoadFromDB(uint32 playerId) override;
        void OnSaveToDB(Player* player) override;
        bool OnLoadActionButtons(Player* player, ActionButtonList& actionButtons) override;
        bool OnSaveActionButtons(Player* player, ActionButtonList& actionButtons) override;

        // Unit / mailbox hooks (hardcore)
        bool OnGetReactionTo(const Unit* unit, const Unit* target, ReputationRank& outReaction) override;
        bool OnCanCheckMailBox(Player* player, const ObjectGuid& mailboxGuid, bool& outResult) override;

        // Loot hooks (hardcore)
        bool OnFillLoot(Loot* loot, Player* owner) override;
        bool OnGenerateMoneyLoot(Loot* loot, uint32& outMoney) override;
        void OnAddItem(Loot* loot, LootItem* lootItem) override;
        void OnSendGold(Loot* loot, Player* player, uint32 gold, uint8 lootMethod) override;

        // Periodic Attuner-of-Paths in-game announcement
        void OnAddToWorld(Creature* creature) override;
        void OnUpdate(uint32 elapsed) override;

        // Group hooks (hardcore)
        bool OnPreInviteMember(Group* group, Player* player, Player* recipient) override;

        // Gossip hooks (combined)
        bool OnPreGossipHello(Player* player, Creature* creature) override;
        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId) override;
        bool OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId) override;

        // Chat commands (hardcore subsystem, now under the 'attunement' prefix)
        std::vector<ModuleChatCommand>* GetCommandTable() override;
        const char* GetChatCommandPrefix() const override { return "attunement"; }
        bool HandleResetCommand(WorldSession* session, const std::string& args);
        bool HandleResetGravesCommand(WorldSession* session, const std::string& args);
        bool HandleResetLootCommand(WorldSession* session, const std::string& args);
        bool HandleSpawnLootCommand(WorldSession* session, const std::string& args);
        bool HandleSpawnGraveCommand(WorldSession* session, const std::string& args);
        bool HandleLevelDownCommand(WorldSession* session, const std::string& args);
        bool HandleToggleReviveCommand(WorldSession* session, const std::string& args);
        bool HandleToggleDropLootCommand(WorldSession* session, const std::string& args);
        bool HandleToggleLoseXPCommand(WorldSession* session, const std::string& args);
        bool HandleTogglePVPCommand(WorldSession* session, const std::string& args);
        bool HandleToggleSelfFoundCommand(WorldSession* session, const std::string& args);
        bool HandleDeathlogCommand(WorldSession* session, const std::string& args);

        // Public helpers (attunement)
        bool IsEnabled() const;
        float GetXpRate(uint32 guid) const;

        // Public helpers (hardcore)
        HardcorePlayerConfig* GetPlayerConfig(uint32 playerId);
        HardcorePlayerConfig* GetPlayerConfig(const Player* player);
        Unit* GetKiller(Player* player) const;
        void SetKiller(Player* player, Unit* killer);

    private:
        // Attunement helpers
        void SetXpRate(Player* player, float rate);
        void RefreshAura(Player* player, float rate);
        uint32 PickAuraSpell(float rate) const;
        float ClampRate(float rate) const;

        // Boost helpers
        void LearnClassSpells(Player* player, uint32 targetLevel);
        void LearnWeaponSkills(Player* player, uint32 targetLevel);
        bool HasAccountBoosted(uint32 accountId) const;
        void RecordAccountBoost(uint32 accountId, Player* player) const;

        // Hardcore submenu helpers
        bool HasAnyHardcoreOption(Player* player, const HardcorePlayerConfig* playerConfig) const;
        void ShowHardcoreMenu(Player* player, Creature* creature);

        // Hardcore helpers
        HardcoreLootGameObject* FindLootGOByGUID(const uint32 guid);
        HardcorePlayerLoot* FindLootByID(const uint32 playerId, const uint32 lootId);

        void PreLoadLoot();
        void LoadLoot();
        void RemoveAllLoot();
        void CreateLoot(Player* player, Unit* killer);
        bool RemoveLoot(uint32 playerId, uint32 lootId);

        void RemoveAllGraves();
        void CreateGrave(Player* player, Unit* killer = nullptr);
        void GenerateMissingGraves();
        void PreLoadGraves();
        void LoadGraves();

        void LevelDown(Player* player, Unit* killer = nullptr);

        // Dualspec helpers (port of upstream dualspec module methods)
        void DualspecLoadPlayerSpec(uint32 playerId);
        uint8 DualspecGetActiveSpec(uint32 playerId) const;
        void DualspecSetActiveSpec(Player* player, uint8 spec);
        uint8 DualspecGetSpecCount(uint32 playerId) const;
        void DualspecSetSpecCount(Player* player, uint8 count);
        void DualspecSavePlayerSpec(uint32 playerId);

        void DualspecLoadSpecNames(Player* player);
        // Returns the player's stored spec name, falling back to the
        // fixed defaults ("Main Spec" / "Secondary Spec") when the DB
        // row is absent or empty. Spec names are no longer player-
        // renameable; this exists only so legacy custom names already
        // in the DB continue to display for existing characters.
        const std::string& DualspecGetSpecName(Player* player, uint8 spec) const;
        static const std::string& DualspecDefaultSpecName(uint8 spec);
        void DualspecSaveSpecNames(Player* player);

        void DualspecLoadTalents(Player* player);
        bool DualspecHasTalent(Player* player, uint32 spellId, uint8 spec);
        DualSpecPlayerTalentMap* DualspecGetTalents(uint32 playerId, int8 spec = -1, bool assert = true);
        void DualspecAddTalent(uint32 playerId, uint32 spellId, uint8 spec, bool learned);
        void DualspecSaveTalents(uint32 playerId);

        void DualspecSendActionButtons(const Player* player, bool clear) const;
        void DualspecActivateSpec(Player* player, uint8 spec);
        void DualspecGiveItem(Player* player);

        // Gossip-on-Attuner: build the dualspec submenu for a player.
        // Called from OnGossipSelect when DUALSPEC_GOSSIP_OPEN_MENU
        // fires. Pulled out so the menu shape stays separate from the
        // dispatcher switch.
        void DualspecSendNpcMenu(Player* player, Creature* attuner);

        // Attunement state
        // guid -> rate (only present when non-default; default is implicit)
        std::unordered_map<uint32, float> m_playerRates;
        // inspector guid -> last seen target. Used by OnRegenerate to whisper
        // an attunement readout when the inspector targets a non-default player.
        std::unordered_map<uint32, ObjectGuid> m_lastSelection;

        // Hardcore state
        std::map<uint32, ObjectGuid> m_lastPlayerDeaths;
        std::unordered_map<uint32, HardcorePlayerGrave> m_playerGraves;
        std::unordered_map<uint32, std::map<uint32, HardcorePlayerLoot>> m_playersLoot;
        std::unordered_map<uint32, HardcorePlayerConfig> m_playerManagers;
        HardcorePlayerDeathLog m_deathLog;
        bool m_getReactionToInternal;

        // Dualspec state (formerly the dualspec module's in-memory maps)
        std::map<uint32, DualSpecPlayerTalentMap[MAX_TALENT_SPECS]> m_dualspecTalents;
        std::map<uint32, std::string[MAX_TALENT_SPECS]> m_dualspecSpecNames;
        std::map<uint32, DualspecPlayerStatus> m_dualspecStatus;

        // Periodic Attuner announcement: spawned Attuner NPCs (tracked by guid
        // so they are resolved fresh each tick - there is no remove-from-world
        // hook, and storing Creature* would dangle on despawn) + the timer.
        struct AttunerSpawn { uint32 mapId; uint32 instanceId; ObjectGuid guid; };
        std::vector<AttunerSpawn> m_attuners;
        // OnAddToWorld runs on map-update worker threads (MapUpdate.Threads>1),
        // so every access to m_attuners must be serialized against concurrent
        // adds and the OnUpdate reader.
        std::mutex m_attunersMutex;
        uint32 m_announceTimerMs = 0;
    };
}

#endif
