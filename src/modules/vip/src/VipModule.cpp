#include "VipModule.h"

#include "Chat/Chat.h"
#include "Entities/Player.h"
#include "Globals/ObjectAccessor.h"
#include "Server/DBCStores.h"
#include "Server/WorldSession.h"
#include "Spells/Spell.h"
#include "Spells/SpellMgr.h"

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
        // masterSpellId=0 is a misconfiguration — treat as disabled so OnCast
        // doesn't compare every spell against 0 on the hot path, and the
        // grant/revoke commands refuse to operate on a non-existent spell.
        return GetConfig()->enabled && GetConfig()->masterSpellId != 0;
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

    // Intercept the master-spell cast and cascade into the bundle. Each
    // bundled spell is cast on the caster as a triggered cast so it bypasses
    // global cooldown / mana cost and applies the buff (with its full
    // original duration) instantly.
    void VipModule::OnCast(Spell* spell, Unit* caster, Unit* /*victim*/)
    {
        if (!IsEnabled() || !spell || !caster)
            return;

        const SpellEntry* info = spell->m_spellInfo;
        if (!info || info->Id != GetConfig()->masterSpellId)
            return;

        for (uint32_t bundledId : GetConfig()->bundledSpellIds)
        {
            const SpellEntry* sub = sSpellTemplate.LookupEntry<SpellEntry>(bundledId);
            if (!sub)
                continue;
            caster->CastSpell(caster, sub, TRIGGERED_OLD_TRIGGERED);
        }
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

        uint32_t masterId = GetConfig()->masterSpellId;
        if (target->HasSpell(masterId))
        {
            ChatHandler(session).PSendSysMessage(
                "|cff1eff00[VIP]|r %s already has the VIP boon (spell %u).",
                target->GetName(), masterId);
            return true;
        }

        target->learnSpell(masterId, false);
        ChatHandler(session).PSendSysMessage(
            "|cff1eff00[VIP]|r %s granted the VIP boon (taught spell %u, cascades into %zu buffs).",
            target->GetName(), masterId, GetConfig()->bundledSpellIds.size());

        target->GetSession()->SendNotification(
            "You have been granted the VIP boon. Cast 'Wayfarer's Boon' from your spellbook.");
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

        uint32_t masterId = GetConfig()->masterSpellId;
        if (!target->HasSpell(masterId))
        {
            ChatHandler(session).PSendSysMessage(
                "|cff1eff00[VIP]|r %s did not have the VIP boon to revoke.",
                target->GetName());
            return true;
        }

        target->removeSpell(masterId, false, false);

        // Strip any active auras the boon cast on the target. Without this the
        // bundled buffs persist until they naturally expire (some world buffs
        // last 2h+), so revoke would only block re-casting, not actually take
        // anything away. Match the cascade set we apply in OnCast.
        target->RemoveAurasDueToSpell(masterId);
        for (uint32_t bundledId : GetConfig()->bundledSpellIds)
        {
            target->RemoveAurasDueToSpell(bundledId);
        }

        ChatHandler(session).PSendSysMessage(
            "|cff1eff00[VIP]|r %s's VIP boon revoked.", target->GetName());

        target->GetSession()->SendNotification("Your VIP boon has been revoked.");
        return true;
    }
}
