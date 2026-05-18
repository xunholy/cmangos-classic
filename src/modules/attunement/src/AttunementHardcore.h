#ifndef CMANGOS_MODULE_ATTUNEMENT_HARDCORE_H
#define CMANGOS_MODULE_ATTUNEMENT_HARDCORE_H

#include "AttunementModuleConfig.h"
#include "Entities/ObjectGuid.h"

#include <utility>
#include <vector>
#include <string>
#include <map>

// Player + Unit are used by pointer throughout this header (GetPlayer,
// GetZoneName(Player*), OnDeath(Unit*), etc.). Forward-declare to avoid
// pulling the full Player.h / Unit.h into every TU that includes this.
class Player;
class Unit;

namespace cmangos_module
{
    class AttunementModule;

    // Bag, Slot
    typedef std::pair<uint8, uint8> ItemSlot;

    struct HardcoreLootItem
    {
        HardcoreLootItem(uint32 id, uint8 amount);
        HardcoreLootItem(uint32 id, uint8 amount, const std::vector<ItemSlot>& slots);
        HardcoreLootItem(uint32 id, uint8 amount, uint32 randomPropertyId, uint32 durability, const std::string& enchantments);
        HardcoreLootItem(uint32 id, uint8 amount, uint32 randomPropertyId, uint32 durability, const std::string& enchantments, const std::vector<ItemSlot>& slots);

        uint32 m_id;
        bool m_isGear;
        uint32 m_randomPropertyId;
        uint32 m_durability;
        std::string m_enchantments;
        uint8 m_amount;
        std::vector<ItemSlot> m_slots;
    };

    class HardcoreLootGameObject
    {
    private:
        HardcoreLootGameObject(uint32 id, uint32 playerId, uint32 lootId, uint32 lootTableId, uint32 money, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const std::vector<HardcoreLootItem>& items, const AttunementModuleConfig* moduleConfig);

    public:
        static HardcoreLootGameObject Load(uint32 id, uint32 playerId, const AttunementModuleConfig* moduleConfig);
        static HardcoreLootGameObject Create(uint32 playerId, uint32 lootId, uint32 money, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const std::vector<HardcoreLootItem>& items, const AttunementModuleConfig* moduleConfig);

        void Spawn();
        void DeSpawn();
        bool IsSpawned() const;
        void Destroy();

        uint32 GetId() const { return m_id; }
        uint32 GetGUID() const { return m_guid; }
        uint32 GetPlayerId() const { return m_playerId; }
        uint32 GetLootId() const { return m_lootId; }
        uint32 GetMoney() const { return m_money; }
        void SetMoney(uint32 money);
        bool HasItems() const { return !m_items.empty(); }
        const std::vector<HardcoreLootItem>& GetItems() const { return m_items; }
        const HardcoreLootItem* GetItem(uint32 itemId) const;
        bool RemoveItem(uint32 itemId);

    private:
        uint32 m_id;
        uint32 m_playerId;
        uint32 m_guid;
        uint32 m_lootId;
        uint32 m_lootTableId;
        uint32 m_money;
        float m_positionX;
        float m_positionY;
        float m_positionZ;
        float m_orientation;
        uint32 m_mapId;
        uint32 m_phaseMask;
        std::vector<HardcoreLootItem> m_items;
        const AttunementModuleConfig* m_moduleConfig;
    };

    class HardcorePlayerLoot
    {
    public:
        HardcorePlayerLoot(uint32 id, uint32 playerId, AttunementModule* module);
        void LoadGameObject(uint32 gameObjectId);
        HardcoreLootGameObject* FindGameObjectByGUID(const uint32 guid);
        bool RemoveGameObject(uint32 gameObjectId);

        bool HasGameObjects() const { return !m_gameObjects.empty(); }
        uint32 GetPlayerId() const { return m_playerId; }
        uint32 GetId() const { return m_id; }

        void Spawn();
        void DeSpawn();
        bool Create();
        void Destroy();

    private:
        uint32 m_id;
        uint32 m_playerId;
        std::vector<HardcoreLootGameObject> m_gameObjects;
        AttunementModule* m_module;
    };

    class HardcoreGraveGameObject
    {
    private:
        HardcoreGraveGameObject(uint32 id, uint32 gameObjectEntry, uint32 playerId, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const AttunementModuleConfig* moduleConfig);

    public:
        static HardcoreGraveGameObject Load(uint32 id, const AttunementModuleConfig* moduleConfig);
        static HardcoreGraveGameObject Create(uint32 playerId, uint32 gameObjectEntry, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const AttunementModuleConfig* moduleConfig);

        void Spawn();
        void DeSpawn();
        bool IsSpawned() const;
        void Destroy();

    private:
        uint32 m_id;
        uint32 m_gameObjectEntry;
        uint32 m_guid;
        uint32 m_playerId;
        float m_positionX;
        float m_positionY;
        float m_positionZ;
        float m_orientation;
        uint32 m_mapId;
        uint32 m_phaseMask;
        const AttunementModuleConfig* m_moduleConfig;
    };

    class HardcorePlayerGrave
    {
    private:
        HardcorePlayerGrave(uint32 playerId, uint32 gameObjectEntry, const AttunementModuleConfig* moduleConfig);
        HardcorePlayerGrave(uint32 playerId, uint32 gameObjectEntry, const std::vector<HardcoreGraveGameObject>& gameObjects, const AttunementModuleConfig* moduleConfig);

    public:
        static HardcorePlayerGrave Load(uint32 playerId, uint32 gameObjectEntry, const AttunementModuleConfig* moduleConfig);
        static HardcorePlayerGrave Generate(uint32 playerId, const std::string& playerName, const AttunementModuleConfig* moduleConfig);

        void Spawn();
        void DeSpawn();
        void Create();
        void Destroy();

    private:
        static std::string GenerateGraveMessage(const std::string& playerName, const AttunementModuleConfig* moduleConfig);

    private:
        uint32 m_playerId;
        uint32 m_gameObjectEntry;
        std::vector<HardcoreGraveGameObject> m_gameObjects;
        const AttunementModuleConfig* m_moduleConfig;
    };

    class HardcorePlayerConfig
    {
    private:
        HardcorePlayerConfig(uint32 playerId);

    public:
        static HardcorePlayerConfig Load(uint32 playerId);
        void Destroy();

        bool IsReviveDisabled() const { return m_reviveDisabled; }
        bool ShouldDropLootOnDeath() const { return m_dropLootOnDeath; }
        bool ShouldLoseXPOnDeath() const { return m_loseXPOnDeath; }
        bool IsPVPDisabled() const { return m_pvpDisabled; }
        bool IsSelfFound() const { return m_selfFound; }

        void ToggleReviveDisabled(bool enable);
        void ToggleDropLootOnDeath(bool enable);
        void ToggleLoseXPOnDeath(bool enable);
        void TogglePVPDisabled(bool enable);
        void ToggleSelfFound(bool enable);

        static bool HasSameChallenges(const HardcorePlayerConfig* playerConfig, const HardcorePlayerConfig* otherPlayerConfig);

        Player* GetPlayer() const;
        const Player* GetPlayerConst() const;

    private:
        void ToggleAura(bool enable, uint32 spellId);

    private:
        uint32 m_playerId;

        bool m_reviveDisabled;
        bool m_dropLootOnDeath;
        bool m_loseXPOnDeath;
        bool m_pvpDisabled;
        bool m_selfFound;
    };

    class HardcorePlayerDeathLogEntry
    {
    public:
        HardcorePlayerDeathLogEntry(uint32 playerId, uint32 accountId, const std::string& playerName, uint32 level, uint32 zoneId, uint32 areaId, uint32 mapId, uint32 killerId, const std::string& killerName, HardcoreDeathReason reason, time_t date);

        std::string GetDateTime() const;
        uint32 GetAccountId() const { return m_accountId; }
        const std::string& GetPlayerName() const { return m_playerName; }
        uint32 GetLevel() const { return m_level; }
        std::string GetZoneName(const Player* player) const;
        std::string GetAreaName(const Player* player) const;
        std::string GetMapName(const Player* player) const;
        uint32 GetKillerId() const { return m_killerId; }
        const std::string& GetKillerName() const { return m_killerName; }
        std::string GetNPCKillerName(const Player* player) const;
        HardcoreDeathReason GetReason() const { return m_reason; }
        time_t GetDate() const { return m_date; }

        std::string GetMessage(const Player* player) const;

    private:
        uint32 m_playerId;
        uint32 m_accountId;
        std::string m_playerName;
        uint32 m_level;
        uint32 m_zoneId;
        uint32 m_areaId;
        uint32 m_mapId;
        uint32 m_killerId;
        std::string m_killerName;
        HardcoreDeathReason m_reason;
        time_t m_date;
    };

    class HardcorePlayerDeathLog
    {
    public:
        HardcorePlayerDeathLog() {}

        void Load();

        void OnDeath(Player* player, const AttunementModuleConfig* moduleConfig, const Unit* killer = nullptr, int8 environmentDamageType = -1);

        std::vector<const HardcorePlayerDeathLogEntry*> GetEntries(HardcoreDeathFilter filter, uint8 amount, uint32 accountId = 0, std::string playerName = "") const;

    private:
        void Add(uint32 playerId, uint32 accountId, const std::string& playerName, uint32 level, uint32 zoneId, uint32 areaId, uint32 mapId, uint32 killerId, const std::string& killerName, HardcoreDeathReason reason, time_t date);

    private:
        std::vector<HardcorePlayerDeathLogEntry> entries;
    };

    // Pure helpers — predicates and rate accessors used by both
    // AttunementModule's hardcore hooks and HardcorePlayerLoot::Create.
    bool ShouldDropLoot(const Player* player, const Unit* killer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool ShouldDropGear(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool ShouldDropItems(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool ShouldDropMoney(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool ShouldSpawnGrave(const Player* player, const Unit* killer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool ShouldLevelDown(const Player* player, const Unit* killer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool ShouldReviveOnGraveyard(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool IsReviveDisabled(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool CanInviteToGroup(const Player* player, const Player* otherPlayer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig, const HardcorePlayerConfig* otherPlayerConfig);
    bool CanUseAuctionHouse(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool CanUseMailBox(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig);
    bool CanTrade(const Player* player, const Player* otherPlayer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig, const HardcorePlayerConfig* otherPlayerConfig);
    float GetDropMoneyRate(const Player* player, const AttunementModuleConfig* moduleConfig);
    float GetDropItemsRate(const Player* player, const AttunementModuleConfig* moduleConfig);
    float GetDropGearRate(const Player* player, const AttunementModuleConfig* moduleConfig);
    uint32 GetMaxPlayerLoot(const AttunementModuleConfig* moduleConfig);
}

#endif
