#include "VipModule.h"

#include "Chat/Chat.h"
#include "Entities/Player.h"
#include "Globals/ObjectAccessor.h"
#include "Server/WorldSession.h"

namespace cmangos_module
{
    VipModule::VipModule()
    : Module("VIP", new VipModuleConfig())
    {
    }

    const VipModuleConfig* VipModule::GetConfig() const
    {
        return (const VipModuleConfig*)Module::GetConfig();
    }

    bool VipModule::IsEnabled() const
    {
        return GetConfig()->enabled;
    }

    std::vector<ModuleChatCommand>* VipModule::GetCommandTable()
    {
        static std::vector<ModuleChatCommand> commandTable =
        {
            { "grant",  std::bind(&VipModule::HandleGrantCommand,  this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
            { "revoke", std::bind(&VipModule::HandleRevokeCommand, this, std::placeholders::_1, std::placeholders::_2), SEC_GAMEMASTER },
        };
        return &commandTable;
    }

    // Online-only target lookup. The target player must be logged in for the
    // command to succeed — offline granting would require a SQL detour into
    // character_spell, which is overkill for v1 (the GM can just ask the
    // VIP to log in for 5 seconds).
    bool VipModule::HandleGrantCommand(WorldSession* session, const std::string& args)
    {
        if (!IsEnabled() || !session)
            return false;

        if (args.empty())
        {
            ChatHandler(session).PSendSysMessage("Usage: .vip grant <character_name>");
            return false;
        }

        Player* target = sObjectAccessor.FindPlayerByName(args.c_str());
        if (!target)
        {
            ChatHandler(session).PSendSysMessage("|cffff4444[VIP]|r '%s' not found or offline.", args.c_str());
            return false;
        }

        uint32_t taught = 0;
        for (uint32_t spellId : GetConfig()->grantedSpellIds)
        {
            if (!target->HasSpell(spellId))
            {
                target->learnSpell(spellId, false);
                ++taught;
            }
        }

        ChatHandler(session).PSendSysMessage(
            "|cff1eff00[VIP]|r %s granted the VIP boon (%u spells taught, %zu already known).",
            target->GetName(), taught, GetConfig()->grantedSpellIds.size() - taught);

        target->GetSession()->SendNotification(
            "You have been granted the VIP boon. Cast the new spells from your spellbook.");
        return true;
    }

    bool VipModule::HandleRevokeCommand(WorldSession* session, const std::string& args)
    {
        if (!IsEnabled() || !session)
            return false;

        if (args.empty())
        {
            ChatHandler(session).PSendSysMessage("Usage: .vip revoke <character_name>");
            return false;
        }

        Player* target = sObjectAccessor.FindPlayerByName(args.c_str());
        if (!target)
        {
            ChatHandler(session).PSendSysMessage("|cffff4444[VIP]|r '%s' not found or offline.", args.c_str());
            return false;
        }

        uint32_t removed = 0;
        for (uint32_t spellId : GetConfig()->grantedSpellIds)
        {
            if (target->HasSpell(spellId))
            {
                target->removeSpell(spellId, false, false);
                ++removed;
            }
        }

        ChatHandler(session).PSendSysMessage(
            "|cff1eff00[VIP]|r %s's VIP boon revoked (%u spells removed).",
            target->GetName(), removed);

        target->GetSession()->SendNotification("Your VIP boon has been revoked.");
        return true;
    }
}
