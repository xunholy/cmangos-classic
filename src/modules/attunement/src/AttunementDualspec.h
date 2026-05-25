#ifndef CMANGOS_MODULE_ATTUNEMENT_DUALSPEC_H
#define CMANGOS_MODULE_ATTUNEMENT_DUALSPEC_H

// Data definitions for the dualspec subsystem amalgamated into the
// attunement module on 2026-05-25. See ../NOTES.md for provenance
// (formerly flekz-games/cmangos-dualspec). All hook handlers live on
// AttunementModule itself — this header only declares the data types
// and constants the implementation needs, mirroring the
// AttunementHardcore.h shape.

#include "Common.h"

#include <unordered_map>
#include <string>

namespace cmangos_module
{
    // Talent storage geometry (unchanged from upstream dualspec).
    #define MAX_TALENT_RANK 5
    #define MAX_TALENT_SPECS 2

    // The dualspec ring/crystal item players carry in inventory. Using
    // the item triggers the spec-switch gossip just like talking to the
    // Attuner does. Item entry is unchanged from upstream dualspec so
    // every existing copy in player inventories on live keeps working.
    #define DUALSPEC_ITEM_ENTRY 17731

    // Gossip menu npc_text entries. NPC_TEXT (50700) is reused when the
    // attuner shows the dualspec submenu; ITEM_TEXT (50701) is used when
    // gossip is opened via the inventory item. Both rows are owned by
    // attunement's world.sql now.
    #define DUALSPEC_NPC_TEXT 50700
    #define DUALSPEC_ITEM_TEXT 50701

    // mangos_string entries — kept at upstream IDs (12000-12021) so any
    // localisation work done against the old dualspec module continues
    // to apply without re-translation.
    enum DualSpecMessages
    {
        DUAL_SPEC_DESCRIPTION = 12000,
        DUAL_SPEC_COST_IS,
        DUAL_SPEC_CHANGE_MY_SPEC,
        DUAL_SPEC_NO_GOLD_UNLOCK,
        DUAL_SPEC_ARE_YOU_SURE_BEGIN,
        DUAL_SPEC_ARE_YOU_SURE_END,
        DUAL_SPEC_ALREADY_ON_SPEC,
        DUAL_SPEC_ACTIVATE,
        DUAL_SPEC_RENAME,
        DUAL_SPEC_UNNAMED,
        DUAL_SPEC_ACTIVE,
        DUAL_SPEC_ERR_COMBAT,
        DUAL_SPEC_ERR_INSTANCE,
        DUAL_SPEC_ERR_MOUNT,
        DUAL_SPEC_ERR_DEAD,
        DUAL_SPEC_ERR_UNLOCK,
        DUAL_SPEC_ERR_LEVEL,
        DUAL_SPEC_ACTIVATE_COLOR,
        DUAL_SPEC_RENAME_COLOR,
        DUAL_SPEC_ARE_YOU_SURE_SWITCH,
        DUAL_SPEC_PURCHASE,
        DUAL_SPEC_ERR_ITEM_CREATE,
    };

    // Dialogue-option action IDs for the dualspec submenu when the
    // player opens gossip on the Attuner. Sit above the attunement
    // ACTION_* range and the HARDCORE_DIALOGUE_OPTION_* range so the
    // dispatcher in AttunementModule::OnGossipSelect can route by ID
    // without ambiguity.
    //
    // Spec names are fixed to "Main Spec" / "Secondary Spec" and not
    // user-renameable — DUALSPEC_GOSSIP_RENAME_SPEC_* actions intentionally
    // do not exist. Any pre-existing rows in custom_dualspec_talent_name
    // from before the rename UX was removed are still loaded from the DB
    // and honoured by DualspecGetSpecName, so legacy custom names persist
    // even though the rename UI is gone.
    enum DualSpecGossipAction
    {
        DUALSPEC_GOSSIP_OPEN_MENU = 30000,   // open the dualspec submenu
        DUALSPEC_GOSSIP_PURCHASE,            // pay the unlock cost
        DUALSPEC_GOSSIP_ACTIVATE_SPEC_0,
        DUALSPEC_GOSSIP_ACTIVATE_SPEC_1,
        DUALSPEC_GOSSIP_BACK,                // return to Attuner main menu
    };

    // In-memory talent state for a single (player, spec) pair. `state`
    // mirrors the Player::PlayerSpellState enum (PLAYERSPELL_UNCHANGED,
    // PLAYERSPELL_NEW, PLAYERSPELL_CHANGED, PLAYERSPELL_REMOVED).
    struct DualspecPlayerTalent
    {
        uint8 state;
        uint8 spec;
    };

    // Per-player dualspec status. specCount=1 means dualspec not yet
    // unlocked; specCount=2 means unlocked. activeSpec is 0 or 1.
    struct DualspecPlayerStatus
    {
        uint8 specCount;
        uint8 activeSpec;
    };

    // spellId -> talent
    typedef std::unordered_map<uint32, DualspecPlayerTalent> DualSpecPlayerTalentMap;
}

#endif
