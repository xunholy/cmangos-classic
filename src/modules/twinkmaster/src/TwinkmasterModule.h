#ifndef CMANGOS_MODULE_TWINKMASTER_H
#define CMANGOS_MODULE_TWINKMASTER_H

#include "Module.h"
#include "TwinkmasterModuleConfig.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cmangos_module
{
    class TwinkmasterModule : public Module
    {
    public:
        TwinkmasterModule();
        const TwinkmasterModuleConfig* GetConfig() const override;

        // Module hooks
        void OnInitialize() override;

        // Player hooks
        void OnLoadFromDB(Player* player) override;
        void OnLogOut(Player* player) override;
        bool OnPreGiveXP(Player* player, uint32& xp, Creature* victim) override;
        bool OnPreGossipHello(Player* player, Creature* creature) override;
        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId) override;
        bool OnUseItem(Player* player, Item* item) override;

        // Public helpers
        bool IsEnabled() const;
        uint32 GetTargetLevel() const;
        bool IsXpLocked(uint32 guid) const;

        void SetLevelAndLock(Player* player);
        void LockXP(Player* player);
        void UnlockXP(Player* player);

    private:
        // Auto-learn helpers
        void LearnProfessions(Player* player);
        void LearnWeaponSkills(Player* player);
        void LearnClassSpells(Player* player);

        // Buff helper
        void ApplyBuffPackage(Player* player, Creature* creature);

        // Enchant helpers
        void ApplyEnchant(Player* player, Item* item, uint32 enchantId);
        void ShowEnchantSlotMenu(Player* player, Creature* creature);
        void ShowEnchantOptions(Player* player, Creature* creature, uint8 equipSlot);

        // Vendor category helpers
        void LoadVendorCategories();
        void SendFilteredVendorInventory(Player* player, Creature* creature, uint8 category);
        void ShowBrowseMenu(Player* player, Creature* creature);

        std::unordered_set<uint32> m_xpLockedPlayers;
        std::unordered_map<uint8, std::vector<uint32>> m_categoryItems;
        std::unordered_map<uint32, uint8> m_enchantSlotSelection;
    };
}

#endif
