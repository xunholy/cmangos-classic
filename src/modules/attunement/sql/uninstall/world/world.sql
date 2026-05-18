-- Reverses the install/world/world.sql for the merged Attunement module.

-- Hardcore: graves + NPC + text + strings + spells.
DELETE FROM `gameobject_template` WHERE `type`=2 AND `CustomData1`=3643;

SET @AttunementEntry := 190014;
DELETE FROM `creature_template` WHERE `entry` = @AttunementEntry;
DELETE FROM `locales_creature` WHERE `entry` = @AttunementEntry;
DELETE FROM `creature` WHERE `id` = @AttunementEntry;

-- Drop the legacy Hardcore NPC entry too in case it was reintroduced.
DELETE FROM `creature_template` WHERE `entry` = 190011;
DELETE FROM `locales_creature` WHERE `entry` = 190011;
DELETE FROM `creature` WHERE `id` = 190011;

SET @ATTUNE_TEXT_ID := 50930;
DELETE FROM `npc_text` WHERE `ID` BETWEEN @ATTUNE_TEXT_ID AND @ATTUNE_TEXT_ID+1;

SET @TEXT_ID := 50900;
DELETE FROM `npc_text` WHERE `ID` BETWEEN @TEXT_ID AND @TEXT_ID+16;
DELETE FROM `locales_npc_text` WHERE `entry` BETWEEN @TEXT_ID AND @TEXT_ID+16;

SET @STRING_ENTRY := 12200;
DELETE FROM `mangos_string` WHERE `entry` BETWEEN @STRING_ENTRY AND @STRING_ENTRY+14;

SET @START_SPELL_ID := 33500;
SET @END_SPELL_ID := @START_SPELL_ID+1;
DELETE FROM `spell_template` WHERE `Id` BETWEEN @START_SPELL_ID AND @END_SPELL_ID;
