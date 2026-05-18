#include "AttunementHardcore.h"
#include "AttunementModule.h"

#include "Database/DatabaseEnv.h"
#include "Entities/Player.h"
#include "Globals/ObjectMgr.h"
#include "Globals/ObjectAccessor.h"
#include "Guilds/Guild.h"
#include "Guilds/GuildMgr.h"
#include "Maps/Map.h"
#include "Maps/MapManager.h"
#include "Server/DBCStores.h"
#include "Spells/SpellMgr.h"
#include "SystemConfig.h"
#include "World/World.h"

#ifdef ENABLE_PLAYERBOTS
#include "playerbot/PlayerbotAI.h"
#endif

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>

namespace cmangos_module
{
    static time_t DateTimeToTime(const std::string& datetime)
    {
        time_t time = 0;
        std::tm tm = {};
        std::istringstream ss(datetime);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (!ss.fail())
        {
            time = std::mktime(&tm);
        }

        return time;
    }

    static bool IsInRaid(const Player* player, const Unit* killer = nullptr)
    {
        if (player && player->IsInWorld())
        {
            if (!player->IsBeingTeleported())
            {
                if (const Map* map = player->GetMap())
                {
                    return map->IsRaid();
                }
            }
            else if (killer)
            {
                if (const Map* map = killer->GetMap())
                {
                    return map->IsRaid();
                }
            }
        }

        return false;
    }

    static bool IsInDungeon(const Player* player, const Unit* killer = nullptr)
    {
        if (player && player->IsInWorld())
        {
            if (!player->IsBeingTeleported())
            {
                if (const Map* map = player->GetMap())
                {
                    return map->IsDungeon();
                }
            }
            else if (killer)
            {
                if (const Map* map = killer->GetMap())
                {
                    return map->IsDungeon();
                }
            }
        }

        return false;
    }

    static bool IsFairKill(const Player* player, const Unit* killer)
    {
        if (player && killer && killer->IsPlayer())
        {
            const uint32 killerLevel = killer->GetLevel();
            const uint32 playerLevel = player->GetLevel();
            return killerLevel <= playerLevel + 3;
        }

        return true;
    }

    uint32 GetMaxPlayerLoot(const AttunementModuleConfig* moduleConfig)
    {
        const uint32 maxPlayerLoot = moduleConfig ? moduleConfig->maxDroppedLoot : 0;
        return maxPlayerLoot > 0 ? maxPlayerLoot : 1;
    }

    float GetDropMoneyRate(const Player* player, const AttunementModuleConfig* moduleConfig)
    {
        if (moduleConfig)
        {
#ifdef ENABLE_PLAYERBOTS
            const bool isBot = player ? !player->isRealPlayer() : false;
            return isBot ? moduleConfig->botDropMoneyPct : moduleConfig->dropMoneyPct;
#else
            return moduleConfig->dropMoneyPct;
#endif
        }

        return 0;
    }

    float GetDropItemsRate(const Player* player, const AttunementModuleConfig* moduleConfig)
    {
        if (moduleConfig)
        {
#ifdef ENABLE_PLAYERBOTS
            const bool isBot = player ? !player->isRealPlayer() : false;
            return isBot ? moduleConfig->botDropItemsPct : moduleConfig->dropItemsPct;
#else
            return moduleConfig->dropItemsPct;
#endif
        }

        return 0;
    }

    float GetDropGearRate(const Player* player, const AttunementModuleConfig* moduleConfig)
    {
        if (moduleConfig)
        {
#ifdef ENABLE_PLAYERBOTS
            const bool isBot = player ? !player->isRealPlayer() : false;
            return isBot ? moduleConfig->botDropGearPct : moduleConfig->dropGearPct;
#else
            return moduleConfig->dropGearPct;
#endif
        }

        return 0;
    }

    bool ShouldLevelDown(const Player* player, const Unit* killer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig && moduleConfig->hardcoreEnabled)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return false;
#endif

            if (moduleConfig->levelDownPct > 0.0f)
            {
                const uint32 playerLevel = player->GetLevel();
                if (playerLevel < moduleConfig->levelDownMinLevel || playerLevel >= moduleConfig->levelDownMaxLevel)
                {
                    return false;
                }

                if (!moduleConfig->levelDownOnDungeons && IsInDungeon(player, killer))
                {
                    return false;
                }

                if (!moduleConfig->levelDownOnRaids && IsInRaid(player, killer))
                {
                    return false;
                }

                return !helper::InPvpMap(player) && IsFairKill(player, killer) && (!playerConfig || playerConfig->ShouldLoseXPOnDeath());
            }
        }

        return false;
    }

    bool ShouldDropMoney(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig && moduleConfig->hardcoreEnabled)
        {
            if (!helper::InPvpMap(player) && (!playerConfig || playerConfig->ShouldDropLootOnDeath()))
            {
                return GetDropMoneyRate(player, moduleConfig) > 0;
            }
        }

        return false;
    }

    bool ShouldDropItems(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig && moduleConfig->hardcoreEnabled)
        {
            if (!helper::InPvpMap(player) && (!playerConfig || playerConfig->ShouldDropLootOnDeath()))
            {
                return GetDropItemsRate(player, moduleConfig) > 0;
            }
        }

        return false;
    }

    bool ShouldDropGear(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig && moduleConfig->hardcoreEnabled)
        {
            if (!helper::InPvpMap(player) && (!playerConfig || playerConfig->ShouldDropLootOnDeath()))
            {
                return GetDropGearRate(player, moduleConfig) > 0;
            }
        }

        return false;
    }

    bool ShouldDropLoot(const Player* player, const Unit* killer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig && moduleConfig->hardcoreEnabled)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
            {
                if (!killer || (killer->IsCreature() || (killer->IsPlayer() && !((Player*)killer)->isRealPlayer())))
                {
                    return false;
                }
            }
#endif

            const uint32 playerLevel = player->GetLevel();
            if (playerLevel < moduleConfig->dropMinLevel || playerLevel >= moduleConfig->dropMaxLevel)
            {
                return false;
            }

            if (!moduleConfig->dropOnDungeons && IsInDungeon(player, killer))
            {
                return false;
            }

            if (!moduleConfig->dropOnRaids && IsInRaid(player, killer))
            {
                return false;
            }

            return IsFairKill(player, killer) && (ShouldDropGear(player, moduleConfig, playerConfig) || ShouldDropItems(player, moduleConfig, playerConfig) || ShouldDropMoney(player, moduleConfig, playerConfig));
        }

        return false;
    }

    bool ShouldSpawnGrave(const Player* player, const Unit* killer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig && moduleConfig->hardcoreEnabled && moduleConfig->spawnGrave)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return false;
#endif

            if (IsInDungeon(player, killer))
            {
                return false;
            }

            if (IsInRaid(player, killer))
            {
                return false;
            }

            return !helper::InPvpMap(player) && (!playerConfig || playerConfig->IsReviveDisabled());
        }

        return false;
    }

    bool IsReviveDisabled(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return false;
#endif

            return moduleConfig->reviveDisabled && (!playerConfig || playerConfig->IsReviveDisabled());
        }

        return false;
    }

    bool ShouldReviveOnGraveyard(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (player && moduleConfig && moduleConfig->hardcoreEnabled && moduleConfig->reviveOnGraveyard)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return false;
#endif

            return !helper::InPvpMap(player) && !IsInDungeon(player) && !IsInRaid(player) && !IsReviveDisabled(player, moduleConfig, playerConfig);
        }

        return false;
    }

    bool CanInviteToGroup(const Player* player, const Player* otherPlayer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig, const HardcorePlayerConfig* otherPlayerConfig)
    {
        if (moduleConfig && moduleConfig->hardcoreEnabled && moduleConfig->selfFound && player && otherPlayer)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer() && !otherPlayer->isRealPlayer())
                return true;
#endif

            const uint32 playerLevel = player->GetLevel();
            const uint32 otherPlayerLevel = otherPlayer->GetLevel();

            if (moduleConfig->playerConfig)
            {
                if (playerConfig && otherPlayerConfig)
                {
                    if (playerConfig->IsSelfFound() && (playerConfig->IsSelfFound() == otherPlayerConfig->IsSelfFound()))
                    {
                        return playerLevel - 1 <= otherPlayerLevel && playerLevel + 1 >= otherPlayerLevel;
                    }
                }
                else if ((playerConfig && !otherPlayerConfig) || (!playerConfig && otherPlayerConfig))
                {
                    const HardcorePlayerConfig* config = playerConfig ? playerConfig : otherPlayerConfig;
                    return !config->IsSelfFound();
                }
            }
            else
            {
                return playerLevel - 1 <= otherPlayerLevel &&
                       playerLevel + 1 >= otherPlayerLevel;
            }
        }

        return true;
    }

    bool CanUseAuctionHouse(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (moduleConfig && moduleConfig->hardcoreEnabled && moduleConfig->selfFound && player)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return true;
#endif

            if (moduleConfig->playerConfig && playerConfig)
            {
                return !playerConfig->IsSelfFound();
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    bool CanUseMailBox(const Player* player, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig)
    {
        if (!player)
            return true;

        if (moduleConfig && moduleConfig->hardcoreEnabled && moduleConfig->selfFound)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer())
                return true;
#endif

            if (moduleConfig->playerConfig && playerConfig)
            {
                return !playerConfig->IsSelfFound();
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    bool CanTrade(const Player* player, const Player* otherPlayer, const AttunementModuleConfig* moduleConfig, const HardcorePlayerConfig* playerConfig, const HardcorePlayerConfig* otherPlayerConfig)
    {
        if (moduleConfig && moduleConfig->hardcoreEnabled && moduleConfig->selfFound)
        {
#ifdef ENABLE_PLAYERBOTS
            if (!player->isRealPlayer() && !otherPlayer->isRealPlayer())
                return true;
#endif

            const uint32 playerLevel = player->GetLevel();
            const uint32 otherPlayerLevel = otherPlayer->GetLevel();

            if (moduleConfig->playerConfig)
            {
                if (playerConfig && otherPlayerConfig)
                {
                    if (playerConfig->IsSelfFound() && (playerConfig->IsSelfFound() == otherPlayerConfig->IsSelfFound()))
                    {
                        return playerLevel - 1 <= otherPlayerLevel && playerLevel + 1 >= otherPlayerLevel;
                    }
                }
                else if ((playerConfig && !otherPlayerConfig) || (!playerConfig && otherPlayerConfig))
                {
                    const HardcorePlayerConfig* config = playerConfig ? playerConfig : otherPlayerConfig;
                    return !config->IsSelfFound();
                }
            }
            else
            {
                return playerLevel - 1 <= otherPlayerLevel &&
                       playerLevel + 1 >= otherPlayerLevel;
            }
        }

        return true;
    }

    HardcoreLootItem::HardcoreLootItem(uint32 id, uint8 amount)
    : m_id(id)
    , m_isGear(false)
    , m_randomPropertyId(0)
    , m_durability(0)
    , m_enchantments(0)
    , m_amount(amount)
    {
    }

    HardcoreLootItem::HardcoreLootItem(uint32 id, uint8 amount, const std::vector<ItemSlot>& slots)
    : m_id(id)
    , m_isGear(false)
    , m_randomPropertyId(0)
    , m_durability(0)
    , m_enchantments(0)
    , m_amount(amount)
    , m_slots(slots)
    {
    }

    HardcoreLootItem::HardcoreLootItem(uint32 id, uint8 amount, uint32 randomPropertyId, uint32 durability, const std::string& enchantments, const std::vector<ItemSlot>& slots)
    : m_id(id)
    , m_isGear(true)
    , m_randomPropertyId(randomPropertyId)
    , m_durability(durability)
    , m_enchantments(enchantments)
    , m_amount(amount)
    , m_slots(slots)
    {
    }

    HardcoreLootItem::HardcoreLootItem(uint32 id, uint8 amount, uint32 randomPropertyId, uint32 durability, const std::string& enchantments)
    : m_id(id)
    , m_isGear(true)
    , m_randomPropertyId(randomPropertyId)
    , m_durability(durability)
    , m_enchantments(enchantments)
    , m_amount(amount)
    {
    }

    HardcoreLootGameObject::HardcoreLootGameObject(uint32 id, uint32 playerId, uint32 lootId, uint32 lootTableId, uint32 money, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const std::vector<HardcoreLootItem>& items, const AttunementModuleConfig* moduleConfig)
    : m_id(id)
    , m_playerId(playerId)
    , m_guid(0)
    , m_lootId(lootId)
    , m_lootTableId(lootTableId)
    , m_money(money)
    , m_positionX(positionX)
    , m_positionY(positionY)
    , m_positionZ(positionZ)
    , m_orientation(orientation)
    , m_mapId(mapId)
    , m_phaseMask(phaseMask)
    , m_items(items)
    , m_moduleConfig(moduleConfig)
    {
    }

    HardcoreLootGameObject HardcoreLootGameObject::Load(uint32 id, uint32 playerId, const AttunementModuleConfig* moduleConfig)
    {
        std::vector<HardcoreLootItem> items;
        uint32 lootId = 0, lootTableId = 0, money = 0, mapId = 0, phaseMask = 0;
        float positionX = 0, positionY = 0, positionZ = 0, orientation = 0;

        auto result = CharacterDatabase.PQuery("SELECT loot_id, loot_table, money, position_x, position_y, position_z, orientation, map, phase_mask FROM custom_hardcore_loot_gameobjects WHERE id = '%d'", id);
        if (result)
        {
            Field* fields = result->Fetch();
            lootId = fields[0].GetUInt32();
            lootTableId = fields[1].GetUInt32();
            money = fields[2].GetUInt32();
            positionX = fields[3].GetFloat();
            positionY = fields[4].GetFloat();
            positionZ = fields[5].GetFloat();
            orientation = fields[6].GetFloat();
            mapId = fields[7].GetUInt32();
            phaseMask = fields[8].GetUInt32();

            auto result2 = CharacterDatabase.PQuery("SELECT item, amount, random_property_id, durability, enchantments FROM custom_hardcore_loot_tables WHERE id = '%d'", lootTableId);
            if (result2)
            {
                do
                {
                    Field* fields2 = result2->Fetch();
                    const uint32 itemId = fields2[0].GetUInt32();
                    const uint8 amount = fields2[1].GetUInt8();
                    const uint32 randomPropertyId = fields2[2].GetUInt32();
                    const uint32 durability = fields2[3].GetUInt32();
                    const std::string enchantments = fields2[4].GetString();
                    items.emplace_back(itemId, amount, randomPropertyId, durability, enchantments);
                }
                while (result2->NextRow());
            }
        }

        return HardcoreLootGameObject(id, playerId, lootId, lootTableId, money, positionX, positionY, positionZ, orientation, mapId, phaseMask, items, moduleConfig);
    }

    HardcoreLootGameObject HardcoreLootGameObject::Create(uint32 playerId, uint32 lootId, uint32 money, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const std::vector<HardcoreLootItem>& items, const AttunementModuleConfig* moduleConfig)
    {
        uint32 newLootTableId = 1;
        auto result = CharacterDatabase.PQuery("SELECT id FROM custom_hardcore_loot_tables ORDER BY id DESC LIMIT 1");
        if (result)
        {
            Field* fields = result->Fetch();
            newLootTableId = fields[0].GetUInt32() + 1;
        }

        uint32 newGOId = 1;
        auto result3 = CharacterDatabase.PQuery("SELECT id FROM custom_hardcore_loot_gameobjects ORDER BY id DESC LIMIT 1");
        if (result3)
        {
            Field* fields = result3->Fetch();
            newGOId = fields[0].GetUInt32() + 1;
        }

        for (const HardcoreLootItem& item : items)
        {
            CharacterDatabase.PExecute("INSERT INTO custom_hardcore_loot_tables (id, item, amount, random_property_id, durability, enchantments) VALUES ('%d', '%d', '%d', '%d', '%d', '%s')",
                newLootTableId,
                item.m_id,
                item.m_amount,
                item.m_randomPropertyId,
                item.m_durability,
                item.m_enchantments.c_str());
        }

        CharacterDatabase.DirectPExecute("INSERT INTO custom_hardcore_loot_gameobjects (id, player, loot_id, loot_table, money, position_x, position_y, position_z, orientation, map, phase_mask) VALUES ('%d', '%d', '%d', '%d', '%d', '%f', '%f', '%f', '%f', '%d', '%d')",
            newGOId,
            playerId,
            lootId,
            newLootTableId,
            money,
            positionX,
            positionY,
            positionZ,
            orientation,
            mapId,
            phaseMask);

        return HardcoreLootGameObject(newGOId, playerId, lootId, newLootTableId, money, positionX, positionY, positionZ, orientation, mapId, phaseMask, items, moduleConfig);
    }

    void HardcoreLootGameObject::Spawn()
    {
        if (!IsSpawned())
        {
            const static uint32 lootGOEntry = m_moduleConfig->lootGameObjectId;
            const uint32 goLowGUID = sObjectMgr.GenerateStaticGameObjectLowGuid();
            if (goLowGUID)
            {
                Map* map = sMapMgr.FindMap(m_mapId);
                if (!map)
                {
                    const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
                    if (Player* player = sObjectMgr.GetPlayer(playerGUID))
                    {
                        map = player->GetMap();
                    }
                }

                if (map)
                {
                    GameObject* pGameObject = GameObject::CreateGameObject(lootGOEntry);
#if EXPANSION == 2
                    if (pGameObject->Create(0, goLowGUID, lootGOEntry, map, m_phaseMask, m_positionX, m_positionY, m_positionZ, m_orientation))
#else
                    if (pGameObject->Create(0, goLowGUID, lootGOEntry, map, m_positionX, m_positionY, m_positionZ, m_orientation))
#endif
                    {
#if EXPANSION == 0
                        pGameObject->SaveToDB(map->GetId());
#elif EXPANSION == 1
                        pGameObject->SaveToDB(map->GetId(), pGameObject->GetPhaseMask());
#elif EXPANSION == 2
                        GameObjectData const* data = sObjectMgr.GetGOData(pGameObject->GetDbGuid());
                        if (data)
                        {
                            pGameObject->SaveToDB(map->GetId(), data->spawnMask, pGameObject->GetPhaseMask());
                        }
#endif
                        if (pGameObject->LoadFromDB(goLowGUID, map, goLowGUID, 0))
                        {
                            pGameObject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                            pGameObject->SetGoState(GO_STATE_READY);
                            pGameObject->SetLootState(GO_READY);
                            pGameObject->SetCooldown(0);

                            sObjectMgr.AddGameobjectToGrid(goLowGUID, sObjectMgr.GetGOData(goLowGUID));

                            WorldDatabase.PExecute("DELETE FROM gameobject WHERE guid = '%d'", goLowGUID);

                            m_guid = goLowGUID;
                        }
                        else
                        {
                            delete pGameObject;
                        }
                    }
                    else
                    {
                        delete pGameObject;
                    }
                }
            }
        }
    }

    void HardcoreLootGameObject::DeSpawn()
    {
        if (IsSpawned())
        {
            if (const GameObjectData* goData = sObjectMgr.GetGOData(m_guid))
            {
                Map* map = sMapMgr.FindMap(m_mapId);
                if (!map)
                {
                    const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
                    if (Player* player = sObjectMgr.GetPlayer(playerGUID))
                    {
                        map = player->GetMap();
                    }
                }

                if (map)
                {
                    GameObject* obj = map->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, goData->id, m_guid));
                    if (obj)
                    {
                        if (const ObjectGuid& ownerGuid = obj->GetOwnerGuid())
                        {
                            Unit* owner = ownerGuid.IsPlayer() ? ObjectAccessor::FindPlayer(ownerGuid) : nullptr;
                            if (owner)
                            {
                                owner->RemoveGameObject(obj, false);
                            }
                        }

                        obj->SetRespawnTime(0);
                        obj->Delete();
                        obj->DeleteFromDB();

                        m_guid = 0;
                    }
                }
            }
        }
    }

    bool HardcoreLootGameObject::IsSpawned() const
    {
        return m_guid;
    }

    void HardcoreLootGameObject::Destroy()
    {
        DeSpawn();

        CharacterDatabase.PExecute("DELETE FROM custom_hardcore_loot_gameobjects WHERE id = '%d'", m_id);
        CharacterDatabase.PExecute("DELETE FROM custom_hardcore_loot_tables WHERE id = '%d'", m_lootTableId);
    }

    void HardcoreLootGameObject::SetMoney(uint32 money)
    {
        m_money = money;
        CharacterDatabase.PExecute("UPDATE custom_hardcore_loot_gameobjects SET money = '%d' WHERE id = '%d'", m_money, m_id);
    }

    const HardcoreLootItem* HardcoreLootGameObject::GetItem(uint32 itemId) const
    {
        for (const HardcoreLootItem& item : m_items)
        {
            if (item.m_id == itemId)
            {
                return &item;
            }
        }

        return nullptr;
    }

    bool HardcoreLootGameObject::RemoveItem(uint32 itemId)
    {
        const HardcoreLootItem* item = GetItem(itemId);
        if (item)
        {
            if (CharacterDatabase.DirectPExecute("DELETE FROM custom_hardcore_loot_tables WHERE id = '%d' AND item = '%d'", m_lootTableId, itemId))
            {
                m_items.erase(std::remove_if(m_items.begin(), m_items.end(), [&item](const HardcoreLootItem& itemInList)
                {
                    return (item->m_id == itemInList.m_id);
                }), m_items.end());

                return true;
            }
        }

        return false;
    }

    HardcorePlayerLoot::HardcorePlayerLoot(uint32 lootId, uint32 playerId, AttunementModule* module)
    : m_id(lootId)
    , m_playerId(playerId)
    , m_module(module)
    {
    }

    void HardcorePlayerLoot::LoadGameObject(uint32 gameObjectId)
    {
        m_gameObjects.push_back(std::move(HardcoreLootGameObject::Load(gameObjectId, m_playerId, m_module->GetConfig())));
    }

    HardcoreLootGameObject* HardcorePlayerLoot::FindGameObjectByGUID(const uint32 guid)
    {
        for (HardcoreLootGameObject& gameObject : m_gameObjects)
        {
            if (gameObject.GetGUID() == guid)
            {
                return &gameObject;
            }
        }

        return nullptr;
    }

    bool HardcorePlayerLoot::RemoveGameObject(uint32 gameObjectId)
    {
        for (uint32 i = 0; i < m_gameObjects.size(); i++)
        {
            if (m_gameObjects[i].GetId() == gameObjectId)
            {
                m_gameObjects[i].Destroy();
                m_gameObjects.erase(m_gameObjects.begin() + i);
                return true;
            }
        }

        return false;
    }

    void HardcorePlayerLoot::Spawn()
    {
        DeSpawn();

        for (HardcoreLootGameObject& gameObject : m_gameObjects)
        {
            gameObject.Spawn();
        }
    }

    void HardcorePlayerLoot::DeSpawn()
    {
        for (HardcoreLootGameObject& gameObject : m_gameObjects)
        {
            gameObject.DeSpawn();
        }
    }

    bool HardcorePlayerLoot::Create()
    {
        const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
        if (Player* player = sObjectMgr.GetPlayer(playerGUID))
        {
            std::vector<HardcoreLootItem> playerLoot;

            // Hearthstone, Earth Totem, Fire Totem, Water Totem, Air Totem, Ankh
            std::set<uint32> ignoreItems = { 6948, 5175, 5176, 5177, 5178, 17030 };

            auto AddItem = [&player, &ignoreItems](uint8 bag, uint8 slot, std::vector<HardcoreLootItem>& outItems)
            {
                if (Item* pItem = player->GetItemByPos(bag, slot))
                {
                    const ItemPrototype* itemData = pItem->GetProto();
                    if ((itemData->Class != ITEM_CLASS_PROJECTILE) && (itemData->Class != ITEM_CLASS_QUEST))
                    {
                        const uint32 itemId = itemData->ItemId;

                        if (ignoreItems.find(itemId) == ignoreItems.end())
                        {
                            auto it = std::find_if(outItems.begin(), outItems.end(), [&itemId](const HardcoreLootItem& item)
                            {
                                return item.m_id == itemId;
                            });

                            if (it != outItems.end())
                            {
                                static const uint8 maxAmount = std::numeric_limits<uint8>::max();
                                const uint32 newAmount = (*it).m_amount + pItem->GetCount();
                                (*it).m_amount = (newAmount < maxAmount) ? newAmount : maxAmount;
                                (*it).m_slots.emplace_back(bag, slot);
                            }
                            else
                            {
                                uint32 durability = 0;
                                uint32 randomPropertyId = 0;
                                std::ostringstream enchantments;
                                if (itemData->Class == ITEM_CLASS_WEAPON || itemData->Class == ITEM_CLASS_ARMOR)
                                {
                                    randomPropertyId = pItem->GetItemRandomPropertyId();
                                    durability = pItem->GetUInt32Value(ITEM_FIELD_DURABILITY);

                                    for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
                                    {
                                        enchantments << pItem->GetEnchantmentId(EnchantmentSlot(i)) << ' ';
                                        enchantments << pItem->GetEnchantmentDuration(EnchantmentSlot(i)) << ' ';
                                        enchantments << pItem->GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
                                    }
                                }

                                std::vector<ItemSlot> slots = { ItemSlot(bag, slot) };
                                outItems.emplace_back(itemId, pItem->GetCount(), randomPropertyId, durability, enchantments.str(), slots);
                            }
                        }
                    }
                }
            };

            auto SelectItemsToDrop = [&player](float dropRate, std::vector<HardcoreLootItem>& items, std::vector<HardcoreLootItem>& outItems)
            {
                if (!items.empty())
                {
                    dropRate = std::min(dropRate, 1.0f);
                    const uint32 dropAmount = items.size() * dropRate;
                    for (uint32 i = 0; i < dropAmount; i++)
                    {
                        const uint32 randIdx = urand(0, items.size() - 1);
                        const HardcoreLootItem& item = outItems.emplace_back(items[randIdx]);

                        items.erase(items.begin() + randIdx);

#ifdef ENABLE_PLAYERBOTS
                        if (player->isRealPlayer())
#endif
                        {
                            for (const ItemSlot& slot : item.m_slots)
                            {
                                player->DestroyItem(slot.first, slot.second, true);
                            }
                        }
                    }
                }
            };

            const AttunementModuleConfig* moduleConfig = m_module->GetConfig();
            const HardcorePlayerConfig* playerConfig = m_module->GetPlayerConfig(player);
            if (ShouldDropGear(player, moduleConfig, playerConfig))
            {
                std::vector<HardcoreLootItem> playerGear;

                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    AddItem(INVENTORY_SLOT_BAG_0, slot, playerGear);
                }

                SelectItemsToDrop(GetDropGearRate(player, moduleConfig), playerGear, playerLoot);
            }

            if (ShouldDropItems(player, moduleConfig, playerConfig))
            {
                std::vector<HardcoreLootItem> playerItems;

                for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
                {
                    AddItem(INVENTORY_SLOT_BAG_0, slot, playerItems);
                }

                for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
                {
                    for (uint8 slot = 0; slot < MAX_BAG_SIZE; ++slot)
                    {
                        AddItem(bag, slot, playerItems);
                    }
                }

                SelectItemsToDrop(GetDropItemsRate(player, moduleConfig), playerItems, playerLoot);
            }

            if (!playerLoot.empty())
            {
                std::vector<std::vector<HardcoreLootItem>> gameObjectsLoot;
                gameObjectsLoot.emplace_back();

                for (const HardcoreLootItem& item : playerLoot)
                {
                    if (gameObjectsLoot.back().size() < MAX_NR_LOOT_ITEMS)
                    {
                        gameObjectsLoot.back().emplace_back(item);
                    }
                    else
                    {
                        gameObjectsLoot.emplace_back();
                        gameObjectsLoot.back().emplace_back(item);
                    }
                }

                uint32 dropMoney = 0;
                if (ShouldDropMoney(player, moduleConfig, playerConfig))
                {
                    const float moneyDropRate = std::min(GetDropMoneyRate(player, moduleConfig), 1.0f);
                    const uint32 playerMoney = player->GetMoney();
                    dropMoney = playerMoney * moneyDropRate;

#ifdef ENABLE_PLAYERBOTS
                    if (player->isRealPlayer())
#endif
                    {
                        player->SetMoney(playerMoney - dropMoney);
                    }
                }

                const float playerX = player->GetPositionX();
                const float playerY = player->GetPositionY();
                const float playerZ = player->GetPositionZ();
                const uint32 mapId = player->GetMapId();
#if EXPANSION == 2
                const uint32 phaseMask = player->GetPhaseMask();
#else
                const uint32 phaseMask = 0;
#endif

                const float angleIncrement = (2 * M_PI) / gameObjectsLoot.size();
                static const float radius = 3.0f;
                float angle = 0;

                for (const std::vector<HardcoreLootItem>& items : gameObjectsLoot)
                {
                    float x = playerX + (radius * cos(angle));
                    float y = playerY + (radius * sin(angle));
                    float z = playerZ;
                    float o = atan2(y - playerY, x - playerX);

                    player->UpdateAllowedPositionZ(x, y, z);

                    angle += angleIncrement;

                    m_gameObjects.emplace_back(std::move(HardcoreLootGameObject::Create(m_playerId, m_id, dropMoney, x, y, z, o, mapId, phaseMask, items, moduleConfig)));

                    if (dropMoney)
                    {
                        dropMoney = 0;
                    }
                }

                Spawn();

                return true;
            }
        }

        return false;
    }

    void HardcorePlayerLoot::Destroy()
    {
        for (HardcoreLootGameObject& gameObject : m_gameObjects)
        {
            gameObject.Destroy();
        }

        m_gameObjects.clear();
    }

    HardcoreGraveGameObject::HardcoreGraveGameObject(uint32 id, uint32 gameObjectEntry, uint32 playerId, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const AttunementModuleConfig* moduleConfig)
    : m_id(id)
    , m_gameObjectEntry(gameObjectEntry)
    , m_guid(0)
    , m_playerId(playerId)
    , m_positionX(positionX)
    , m_positionY(positionY)
    , m_positionZ(positionZ)
    , m_orientation(orientation)
    , m_mapId(mapId)
    , m_phaseMask(phaseMask)
    , m_moduleConfig(moduleConfig)
    {
    }

    HardcoreGraveGameObject HardcoreGraveGameObject::Load(uint32 id, const AttunementModuleConfig* moduleConfig)
    {
        uint32 gameObjectEntry = 0, playerId = 0, mapId = 0, phaseMask = 0;
        float positionX = 0, positionY = 0, positionZ = 0, orientation = 0;

        auto result = CharacterDatabase.PQuery("SELECT player, gameobject_template, position_x, position_y, position_z, orientation, map, phase_mask FROM custom_hardcore_grave_gameobjects WHERE id = '%d'", id);
        if (result)
        {
            Field* fields = result->Fetch();
            playerId = fields[0].GetUInt32();
            gameObjectEntry = fields[1].GetUInt32();
            positionX = fields[2].GetFloat();
            positionY = fields[3].GetFloat();
            positionZ = fields[4].GetFloat();
            orientation = fields[5].GetFloat();
            mapId = fields[6].GetUInt32();
            phaseMask = fields[7].GetUInt32();
        }

        return HardcoreGraveGameObject(id, gameObjectEntry, playerId, positionX, positionY, positionZ, orientation, mapId, phaseMask, moduleConfig);
    }

    HardcoreGraveGameObject HardcoreGraveGameObject::Create(uint32 playerId, uint32 gameObjectEntry, float positionX, float positionY, float positionZ, float orientation, uint32 mapId, uint32 phaseMask, const AttunementModuleConfig* moduleConfig)
    {
        uint32 newGameObjectId = 1;
        auto result = CharacterDatabase.PQuery("SELECT id FROM custom_hardcore_grave_gameobjects ORDER BY id DESC LIMIT 1");
        if (result)
        {
            Field* fields = result->Fetch();
            newGameObjectId = fields[0].GetUInt32() + 1;
        }

        CharacterDatabase.PExecute("INSERT INTO custom_hardcore_grave_gameobjects (id, player, gameobject_template, position_x, position_y, position_z, orientation, map, phase_mask) VALUES ('%d', '%d', '%d', '%f', '%f', '%f', '%f', '%d', '%d')",
            newGameObjectId,
            playerId,
            gameObjectEntry,
            positionX,
            positionY,
            positionZ,
            orientation,
            mapId,
            phaseMask);

        return HardcoreGraveGameObject(newGameObjectId, gameObjectEntry, playerId, positionX, positionY, positionZ, orientation, mapId, phaseMask, moduleConfig);
    }

    void HardcoreGraveGameObject::Spawn()
    {
        if (!IsSpawned())
        {
            uint32 gameObjectEntry = m_gameObjectEntry;
            const GameObjectInfo* goInfo = sObjectMgr.GetGameObjectInfo(m_gameObjectEntry);
            if (!goInfo)
            {
                gameObjectEntry = m_moduleConfig->graveGameObjectId;
            }

            const uint32 goLowGUID = sObjectMgr.GenerateStaticGameObjectLowGuid();
            if (goLowGUID)
            {
                Map* map = sMapMgr.FindMap(m_mapId);
                if (!map)
                {
                    const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
                    if (Player* player = sObjectMgr.GetPlayer(playerGUID))
                    {
                        map = player->GetMap();
                    }
                }

                if (map)
                {
                    GameObject* pGameObject = GameObject::CreateGameObject(gameObjectEntry);
#if EXPANSION == 2
                    if (pGameObject->Create(0, goLowGUID, gameObjectEntry, map, m_phaseMask, m_positionX, m_positionY, m_positionZ, m_orientation))
#else
                    if (pGameObject->Create(0, goLowGUID, gameObjectEntry, map, m_positionX, m_positionY, m_positionZ, m_orientation))
#endif
                    {
#if EXPANSION == 0
                        pGameObject->SaveToDB(map->GetId());
#elif EXPANSION == 1
                        pGameObject->SaveToDB(map->GetId(), pGameObject->GetPhaseMask());
#elif EXPANSION == 2
                        GameObjectData const* data = sObjectMgr.GetGOData(pGameObject->GetDbGuid());
                        if (data)
                        {
                            pGameObject->SaveToDB(map->GetId(), data->spawnMask, pGameObject->GetPhaseMask());
                        }
#endif
                        if (pGameObject->LoadFromDB(goLowGUID, map, goLowGUID, 0))
                        {
                            sObjectMgr.AddGameobjectToGrid(goLowGUID, sObjectMgr.GetGOData(goLowGUID));

                            WorldDatabase.PExecute("DELETE FROM gameobject WHERE guid = '%d'", goLowGUID);

                            m_guid = goLowGUID;
                        }
                        else
                        {
                            delete pGameObject;
                        }
                    }
                    else
                    {
                        delete pGameObject;
                    }
                }
            }
        }
    }

    void HardcoreGraveGameObject::DeSpawn()
    {
        if (IsSpawned())
        {
            if (const GameObjectData* goData = sObjectMgr.GetGOData(m_guid))
            {
                Map* map = sMapMgr.FindMap(m_mapId);
                if (!map)
                {
                    const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
                    if (Player* player = sObjectMgr.GetPlayer(playerGUID))
                    {
                        map = player->GetMap();
                    }
                }

                if (map)
                {
                    GameObject* obj = map->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, goData->id, m_guid));
                    if (obj)
                    {
                        if (const ObjectGuid& ownerGuid = obj->GetOwnerGuid())
                        {
                            Unit* owner = ownerGuid.IsPlayer() ? ObjectAccessor::FindPlayer(ownerGuid) : nullptr;
                            if (owner)
                            {
                                owner->RemoveGameObject(obj, false);
                            }
                        }

                        obj->SetRespawnTime(0);
                        obj->Delete();
                        obj->DeleteFromDB();

                        m_guid = 0;
                    }
                }
            }
        }
    }

    bool HardcoreGraveGameObject::IsSpawned() const
    {
        return m_guid;
    }

    void HardcoreGraveGameObject::Destroy()
    {
        DeSpawn();

        CharacterDatabase.PExecute("DELETE FROM custom_hardcore_grave_gameobjects WHERE id = '%d'", m_id);
    }

    HardcorePlayerGrave::HardcorePlayerGrave(uint32 playerId, uint32 gameObjectEntry, const std::vector<HardcoreGraveGameObject>& gameObjects, const AttunementModuleConfig* moduleConfig)
    : m_playerId(playerId)
    , m_gameObjectEntry(gameObjectEntry)
    , m_gameObjects(gameObjects)
    , m_moduleConfig(moduleConfig)
    {
    }

    HardcorePlayerGrave::HardcorePlayerGrave(uint32 playerId, uint32 gameObjectEntry, const AttunementModuleConfig* moduleConfig)
    : m_playerId(playerId)
    , m_gameObjectEntry(gameObjectEntry)
    , m_moduleConfig(moduleConfig)
    {
    }

    HardcorePlayerGrave HardcorePlayerGrave::Load(uint32 playerId, uint32 gameObjectEntry, const AttunementModuleConfig* moduleConfig)
    {
        std::vector<HardcoreGraveGameObject> gameObjects;
        auto result = CharacterDatabase.PQuery("SELECT id FROM custom_hardcore_grave_gameobjects WHERE player = '%d'", playerId);
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                const uint32 gameObjectId = fields[0].GetUInt32();
                gameObjects.push_back(std::move(HardcoreGraveGameObject::Load(gameObjectId, moduleConfig)));
            }
            while (result->NextRow());
        }

        return HardcorePlayerGrave(playerId, gameObjectEntry, gameObjects, moduleConfig);
    }

    HardcorePlayerGrave HardcorePlayerGrave::Generate(uint32 playerId, const std::string& playerName, const AttunementModuleConfig* moduleConfig)
    {
        uint32 newGameObjectEntry = 0;
        auto result = WorldDatabase.PQuery("SELECT entry FROM gameobject_template ORDER BY entry DESC LIMIT 1");
        if (result)
        {
            Field* fields = result->Fetch();
            newGameObjectEntry = fields[0].GetUInt32() + 1;
        }

        if (newGameObjectEntry)
        {
            float size = 1.29f;
            uint32 displayId = 12;
            const GameObjectInfo* goInfo = ObjectMgr::GetGameObjectInfo(moduleConfig->graveGameObjectId);
            if (goInfo)
            {
                displayId = goInfo->displayId;
                size = goInfo->size;
            }

            // Player names with apostrophes (e.g. "O'Brien") will break the
            // INSERT unless escaped. PExecute does %d/%f arg substitution but
            // not %s escaping.
            std::string graveMessage = GenerateGraveMessage(playerName, moduleConfig);
            WorldDatabase.escape_string(graveMessage);
            WorldDatabase.PExecute("INSERT INTO gameobject_template (entry, type, displayId, name, size, data10, CustomData1) VALUES ('%d', '%d', '%d', '%s', '%f', '%d', '%d')",
                newGameObjectEntry,
                2,
                displayId,
                graveMessage.c_str(),
                size,
                playerId,
                3643);
        }

        return HardcorePlayerGrave(playerId, newGameObjectEntry, moduleConfig);
    }

    void HardcorePlayerGrave::Spawn()
    {
        DeSpawn();

        for (HardcoreGraveGameObject& gameObject : m_gameObjects)
        {
            gameObject.Spawn();
        }
    }

    void HardcorePlayerGrave::DeSpawn()
    {
        for (HardcoreGraveGameObject& gameObject : m_gameObjects)
        {
            gameObject.DeSpawn();
        }
    }

    void HardcorePlayerGrave::Create()
    {
        const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
        if (Player* player = sObjectMgr.GetPlayer(playerGUID))
        {
            const float x = player->GetPositionX();
            const float y = player->GetPositionY();
            float z = player->GetPositionZ();
            const float o = player->GetOrientation();
            const uint32 mapId = player->GetMapId();
#if EXPANSION == 2
            const uint32 phaseMask = player->GetPhaseMask();
#else
            const uint32 phaseMask = 0;
#endif

            player->UpdateAllowedPositionZ(x, y, z);

            HardcoreGraveGameObject& gameObject = m_gameObjects.emplace_back(std::move(HardcoreGraveGameObject::Create(m_playerId, m_gameObjectEntry, x, y, z, o, mapId, phaseMask, m_moduleConfig)));
            gameObject.Spawn();
        }
    }

    void HardcorePlayerGrave::Destroy()
    {
        for (HardcoreGraveGameObject& gameObject : m_gameObjects)
        {
            gameObject.Destroy();
        }

        m_gameObjects.clear();

        WorldDatabase.PExecute("DELETE FROM gameobject_template WHERE entry = '%d'", m_gameObjectEntry);
    }

    std::string HardcorePlayerGrave::GenerateGraveMessage(const std::string& playerName, const AttunementModuleConfig* moduleConfig)
    {
        std::string gravestoneMessage;
        std::string gravestoneMessages = moduleConfig->graveMessage;

        char separator = '|';
        if (gravestoneMessages.find(separator) != std::string::npos)
        {
            std::string segment;
            std::stringstream messages(gravestoneMessages);
            std::vector<std::string> gravestoneMessageList;
            while (std::getline(messages, segment, separator))
            {
                gravestoneMessageList.push_back(segment);
            }

            if (!gravestoneMessageList.empty())
            {
                const size_t messageIndex = urand(0, gravestoneMessageList.size() - 1);
                gravestoneMessage = gravestoneMessageList[messageIndex];
            }
            else
            {
                gravestoneMessage = "Here lies <PlayerName>";
            }
        }
        else
        {
            gravestoneMessage = gravestoneMessages;
        }

        static const std::string playerNameVar = "<PlayerName>";
        const size_t startPos = gravestoneMessage.find(playerNameVar);
        if (startPos != std::string::npos)
        {
            gravestoneMessage.replace(startPos, playerNameVar.length(), playerName);
        }

        return gravestoneMessage;
    }

    HardcorePlayerConfig::HardcorePlayerConfig(uint32 playerId)
    : m_playerId(playerId)
    , m_reviveDisabled(false)
    , m_dropLootOnDeath(false)
    , m_loseXPOnDeath(false)
    , m_pvpDisabled(false)
    , m_selfFound(false)
    {
    }

    HardcorePlayerConfig HardcorePlayerConfig::Load(uint32 playerId)
    {
        HardcorePlayerConfig playerConfig(playerId);
        if (playerId > 0)
        {
            auto result = CharacterDatabase.PQuery("SELECT revive_disabled, drop_loot_on_death, lose_xp_on_death, pvp_disabled, self_found FROM custom_hardcore_player_config WHERE id = '%d'", playerId);
            if (result)
            {
                Field* fields = result->Fetch();
                playerConfig.m_reviveDisabled = fields[0].GetBool();
                playerConfig.m_dropLootOnDeath = fields[1].GetBool();
                playerConfig.m_loseXPOnDeath = fields[2].GetBool();
                playerConfig.m_pvpDisabled = fields[3].GetBool();
                playerConfig.m_selfFound = fields[4].GetBool();
            }
            else
            {
                CharacterDatabase.PExecute("INSERT INTO custom_hardcore_player_config (id, revive_disabled, drop_loot_on_death, lose_xp_on_death, pvp_disabled, self_found) VALUES ('%d', '%d', '%d', '%d', '%d', '%d')",
                    playerId,
                    playerConfig.m_reviveDisabled ? 1 : 0,
                    playerConfig.m_dropLootOnDeath ? 1 : 0,
                    playerConfig.m_loseXPOnDeath ? 1 : 0,
                    playerConfig.m_pvpDisabled ? 1 : 0,
                    playerConfig.m_selfFound ? 1 : 0);
            }
        }

        return playerConfig;
    }

    void HardcorePlayerConfig::Destroy()
    {
        CharacterDatabase.PExecute("DELETE FROM custom_hardcore_player_config WHERE id = '%d'", m_playerId);
    }

    void HardcorePlayerConfig::ToggleReviveDisabled(bool enable)
    {
        m_reviveDisabled = enable;
        CharacterDatabase.PExecute("UPDATE custom_hardcore_player_config SET revive_disabled = '%d' WHERE id = '%d'", m_reviveDisabled ? 1 : 0, m_playerId);

        ToggleAura(enable, HARDCORE_SPELL_HARDCORE_CHALLENGE);
    }

    void HardcorePlayerConfig::ToggleDropLootOnDeath(bool enable)
    {
        m_dropLootOnDeath = enable;
        CharacterDatabase.PExecute("UPDATE custom_hardcore_player_config SET drop_loot_on_death = '%d' WHERE id = '%d'", m_dropLootOnDeath ? 1 : 0, m_playerId);
    }

    void HardcorePlayerConfig::ToggleLoseXPOnDeath(bool enable)
    {
        m_loseXPOnDeath = enable;
        CharacterDatabase.PExecute("UPDATE custom_hardcore_player_config SET lose_xp_on_death = '%d' WHERE id = '%d'", m_loseXPOnDeath ? 1 : 0, m_playerId);
    }

    void HardcorePlayerConfig::TogglePVPDisabled(bool enable)
    {
        m_pvpDisabled = enable;
        CharacterDatabase.PExecute("UPDATE custom_hardcore_player_config SET pvp_disabled = '%d' WHERE id = '%d'", m_pvpDisabled ? 1 : 0, m_playerId);
    }

    void HardcorePlayerConfig::ToggleSelfFound(bool enable)
    {
        m_selfFound = enable;
        CharacterDatabase.PExecute("UPDATE custom_hardcore_player_config SET self_found = '%d' WHERE id = '%d'", m_selfFound ? 1 : 0, m_playerId);

        ToggleAura(enable, HARDCORE_SPELL_SELF_FOUND_CHALLENGE);
    }

    Player* HardcorePlayerConfig::GetPlayer() const
    {
        const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
        return sObjectMgr.GetPlayer(playerGUID);
    }

    const Player* HardcorePlayerConfig::GetPlayerConst() const
    {
        const ObjectGuid playerGUID = ObjectGuid(HIGHGUID_PLAYER, m_playerId);
        return sObjectMgr.GetPlayer(playerGUID);
    }

    bool HardcorePlayerConfig::HasSameChallenges(const HardcorePlayerConfig* playerConfig, const HardcorePlayerConfig* otherPlayerConfig)
    {
        return playerConfig &&
               otherPlayerConfig &&
               playerConfig->IsReviveDisabled() == otherPlayerConfig->IsReviveDisabled() &&
               playerConfig->IsSelfFound() == otherPlayerConfig->IsSelfFound();
    }

    void HardcorePlayerConfig::ToggleAura(bool enable, uint32 spellId)
    {
        if (Player* player = GetPlayer())
        {
            const bool hasAura = player->HasAura(spellId);
            if (enable)
            {
                if (!hasAura)
                {
                    if (const SpellEntry* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(spellId))
                    {
                        SpellAuraHolder* holder = CreateSpellAuraHolder(spellInfo, player, player);
                        Aura* aur = CreateAura(spellInfo, SpellEffectIndex(0), 0, 0, holder, player);
                        holder->AddAura(aur, SpellEffectIndex(0));

                        if (player->AddSpellAuraHolder(holder))
                        {
                            holder->SetState(SPELLAURAHOLDER_STATE_READY);
                        }
                        else
                        {
                            delete holder;
                        }
                    }
                }
            }
            else if (hasAura)
            {
                player->RemoveAurasDueToSpell(spellId);
            }
        }
    }

    HardcorePlayerDeathLogEntry::HardcorePlayerDeathLogEntry(uint32 playerId, uint32 accountId, const std::string& playerName, uint32 level, uint32 zoneId, uint32 areaId, uint32 mapId, uint32 killerId, const std::string& killerName, HardcoreDeathReason reason, time_t date)
    : m_playerId(playerId)
    , m_accountId(accountId)
    , m_playerName(playerName)
    , m_level(level)
    , m_zoneId(zoneId)
    , m_areaId(areaId)
    , m_mapId(mapId)
    , m_killerId(killerId)
    , m_killerName(killerName)
    , m_reason(reason)
    , m_date(date)
    {
    }

    std::string HardcorePlayerDeathLogEntry::GetDateTime() const
    {
        return TimeToTimestampStr(m_date);
    }

    std::string HardcorePlayerDeathLogEntry::GetZoneName(const Player* player) const
    {
        std::string zoneName = "";

        const int localeIdx = player ? player->GetSession()->GetSessionDbcLocale() : sWorld.GetDefaultDbcLocale();
        if (const AreaTableEntry* zoneEntry = sAreaStore.LookupEntry(m_zoneId))
        {
            zoneName = zoneEntry->area_name[localeIdx];
        }

        return zoneName;
    }

    std::string HardcorePlayerDeathLogEntry::GetAreaName(const Player* player) const
    {
        std::string areaName = "";

        const int localeIdx = player ? player->GetSession()->GetSessionDbcLocale() : sWorld.GetDefaultDbcLocale();
        if (const AreaTableEntry* areaEntry = sAreaStore.LookupEntry(m_areaId))
        {
            areaName = areaEntry->area_name[localeIdx];
        }

        return areaName;
    }

    std::string HardcorePlayerDeathLogEntry::GetMapName(const Player* player) const
    {
        std::string mapName = "";

        const int localeIdx = player ? player->GetSession()->GetSessionDbcLocale() : sWorld.GetDefaultDbcLocale();
        if (const MapEntry* mapEntry = sMapStore.LookupEntry(m_mapId))
        {
            mapName = mapEntry->name[localeIdx];
        }

        return mapName;
    }

    std::string HardcorePlayerDeathLogEntry::GetNPCKillerName(const Player* player) const
    {
        std::string killerName = m_killerName;
        if (player)
        {
            char const* name = "";
            const int localeIdx = player->GetSession()->GetSessionDbLocaleIndex();
            sObjectMgr.GetCreatureLocaleStrings(m_killerId, localeIdx, &name);

            if (*name)
            {
                killerName = name;
            }
        }

        return killerName;
    }

    std::string HardcorePlayerDeathLogEntry::GetMessage(const Player* player) const
    {
        std::string message;
        if (player)
        {
            const std::string& playerName = GetPlayerName();
            const uint32 level = GetLevel();
            const HardcoreDeathReason reason = GetReason();
            const std::string zoneName = GetZoneName(player);
            const std::string areaName = GetAreaName(player);
            const std::string mapName = GetMapName(player);
            const std::string dateStr = secsToTimeString(time(nullptr) - GetDate());

            std::ostringstream reasonStr;
            switch (reason)
            {
                case HARDCORE_DEATH_REASON_NPC_KILL:
                    reasonStr << "was killed by a " << GetNPCKillerName(player);
                    break;
                case HARDCORE_DEATH_REASON_PLAYER_KILL:
                    reasonStr << "was killed by the player " << GetKillerName();
                    break;
                case HARDCORE_DEATH_REASON_EXHAUSTED:
                    reasonStr << "died from exhaustion";
                    break;
                case HARDCORE_DEATH_REASON_DROWNING:
                    reasonStr << "drowned";
                    break;
                case HARDCORE_DEATH_REASON_FALL:
                case HARDCORE_DEATH_REASON_FALL_TO_VOID:
                    reasonStr << "fell into the abyss";
                    break;
                case HARDCORE_DEATH_REASON_LAVA:
                    reasonStr << "tried to swim in lava";
                    break;
                case HARDCORE_DEATH_REASON_SLIME:
                    reasonStr << "tried eat a slime";
                    break;
                case HARDCORE_DEATH_REASON_FIRE:
                    reasonStr << "was burned into a crisp";
                    break;
                default:
                    reasonStr << "died";
                    break;
            }

            std::ostringstream placeStr;
            if (!areaName.empty())
            {
                placeStr << " in " << areaName;
            }

            if (!zoneName.empty())
            {
                if (placeStr.str().empty())
                {
                    placeStr << " in " << zoneName;
                }
                else
                {
                    placeStr << ", " << zoneName;
                }
            }

            if (!mapName.empty())
            {
                if (placeStr.str().empty())
                {
                    placeStr << " in " << mapName;
                }
                else
                {
                    placeStr << ", " << mapName;
                }
            }

            std::ostringstream entryStr;
            entryStr
            << playerName
            << " " << reasonStr.str()
            << " at level " << level
            << placeStr.str()
            << ", " << dateStr << " ago";
            message = entryStr.str();
        }

        return message;
    }

    void HardcorePlayerDeathLog::Load()
    {
        auto result = CharacterDatabase.PQuery("SELECT player, account, name, level, zone, area, map, killer, killer_name, reason, date FROM custom_hardcore_player_deathlog");
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                const uint32 playerId = fields[0].GetUInt32();
                const uint32 accountId = fields[1].GetUInt32();
                const std::string playerName = fields[2].GetCppString();
                const uint32 level = fields[3].GetUInt32();
                const uint32 zoneId = fields[4].GetUInt32();
                const uint32 areaId = fields[5].GetUInt32();
                const uint32 mapId = fields[6].GetUInt32();
                const uint32 killerId = fields[7].GetUInt32();
                const std::string killerName = fields[8].GetCppString();
                const HardcoreDeathReason reason = static_cast<HardcoreDeathReason>(fields[9].GetUInt32());
                const time_t date = DateTimeToTime(fields[10].GetCppString());

                entries.push_back(HardcorePlayerDeathLogEntry(playerId, accountId, playerName, level, zoneId, areaId, mapId, killerId, killerName, reason, date));
            }
            while (result->NextRow());
        }
    }

    void HardcorePlayerDeathLog::OnDeath(Player* player, const AttunementModuleConfig* moduleConfig, const Unit* killer, int8 environmentDamageType)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            const uint32 accountId = player->GetSession()->GetAccountId();
            const std::string playerName = player->GetName();
            const uint32 level = player->GetLevel();
            const uint32 zoneId = player->GetZoneId();
            const uint32 areaId = player->GetAreaId();
            const uint32 mapId = player->GetMapId();
            const uint32 killerId = killer ? (killer->IsPlayer() ? killer->GetObjectGuid().GetCounter() : killer->GetEntry()) : 0;
            const std::string killerName = killer ? killer->GetName() : "";

            HardcoreDeathReason reason = HARDCORE_DEATH_REASON_UNKNOWN;
            if (killer)
            {
                reason = killer->IsPlayer() ? HARDCORE_DEATH_REASON_PLAYER_KILL : HARDCORE_DEATH_REASON_NPC_KILL;
            }
            else if (environmentDamageType >= 0)
            {
                reason = static_cast<HardcoreDeathReason>(environmentDamageType + HARDCORE_DEATH_REASON_PLAYER_KILL + 1);
            }

            const time_t date = time(nullptr);

            Add(playerId, accountId, playerName, level, zoneId, areaId, mapId, killerId, killerName, reason, date);

            if (moduleConfig && (moduleConfig->broadcastDeathGuild || moduleConfig->broadcastDeathWorld))
            {
                const std::string message = entries.back().GetMessage(player);
                if (moduleConfig->broadcastDeathGuild)
                {
                    if (Guild* guild = sGuildMgr.GetGuildById(player->GetGuildId()))
                    {
                        guild->BroadcastToGuild(player->GetSession(), message, LANG_UNIVERSAL);
                    }
                }

                if (moduleConfig->broadcastDeathWorld)
                {
                    WorldPacket data;
                    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, message.c_str());
                    sWorld.SendGlobalMessage(data);
                }
            }
        }
    }

    std::vector<const HardcorePlayerDeathLogEntry*> HardcorePlayerDeathLog::GetEntries(HardcoreDeathFilter filter, uint8 amount, uint32 accountId, std::string playerName) const
    {
        std::string playerNameLowercase = playerName;
        std::transform(playerNameLowercase.begin(), playerNameLowercase.end(), playerNameLowercase.begin(), [](unsigned char c) { return std::tolower(c); });

        std::vector<const HardcorePlayerDeathLogEntry*> filteredEntries;
        for (int i = entries.size() - 1; i >= 0; --i)
        {
            const HardcorePlayerDeathLogEntry* entry = &entries[i];
            std::string entryPlayerNameLowercase = entry->GetPlayerName();
            std::transform(entryPlayerNameLowercase.begin(), entryPlayerNameLowercase.end(), entryPlayerNameLowercase.begin(), [](unsigned char c) { return std::tolower(c); });

            if ((filter == HARDCORE_DEATH_FILTER_PLAYER && playerNameLowercase == entryPlayerNameLowercase) ||
                (filter == HARDCORE_DEATH_FILTER_ACCOUNT && accountId == entry->GetAccountId()) ||
                (filter == HARDCORE_DEATH_FILTER_WORLD))
            {
                filteredEntries.push_back(entry);
                if (filteredEntries.size() >= amount)
                {
                    break;
                }
            }
        }

        return filteredEntries;
    }

    void HardcorePlayerDeathLog::Add(uint32 playerId, uint32 accountId, const std::string& playerName, uint32 level, uint32 zoneId, uint32 areaId, uint32 mapId, uint32 killerId, const std::string& killerName, HardcoreDeathReason reason, time_t date)
    {
        entries.push_back(HardcorePlayerDeathLogEntry(playerId, accountId, playerName, level, zoneId, areaId, mapId, killerId, killerName, reason, date));

        const HardcorePlayerDeathLogEntry& entry = entries.back();
        const std::string dateTime = entry.GetDateTime();

        CharacterDatabase.PExecute("INSERT INTO custom_hardcore_player_deathlog (player, account, name, level, zone, area, map, killer, killer_name, reason, date) VALUES ('%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%s', '%d', '%s')",
            playerId,
            accountId,
            playerName.c_str(),
            level,
            zoneId,
            areaId,
            mapId,
            killerId,
            killerName.c_str(),
            static_cast<uint32>(reason),
            dateTime.c_str());
    }
}
