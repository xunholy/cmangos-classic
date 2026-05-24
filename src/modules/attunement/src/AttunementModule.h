#ifndef CMANGOS_MODULE_ATTUNEMENT_H
#define CMANGOS_MODULE_ATTUNEMENT_H

#include "Module.h"
#include "AttunementModuleConfig.h"
#include "AttunementHardcore.h"

#include "Entities/ObjectGuid.h"

#include <map>
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

        // Unit / mailbox hooks (hardcore)
        bool OnGetReactionTo(const Unit* unit, const Unit* target, ReputationRank& outReaction) override;
        bool OnCanCheckMailBox(Player* player, const ObjectGuid& mailboxGuid, bool& outResult) override;

        // Loot hooks (hardcore)
        bool OnFillLoot(Loot* loot, Player* owner) override;
        bool OnGenerateMoneyLoot(Loot* loot, uint32& outMoney) override;
        void OnAddItem(Loot* loot, LootItem* lootItem) override;
        void OnSendGold(Loot* loot, Player* player, uint32 gold, uint8 lootMethod) override;

        // Group hooks (hardcore)
        bool OnPreInviteMember(Group* group, Player* player, Player* recipient) override;

        // Gossip hooks (combined)
        bool OnPreGossipHello(Player* player, Creature* creature) override;
        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId) override;

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
    };
}

#endif
