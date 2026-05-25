-- ============================================================
-- Attunement (+ amalgamated Hardcore) world data
--
-- Single NPC: 190014 "Attuner of Paths" — handles both
--   1. Per-player XP-rate adjustment (Attunement.*)
--   2. Hardcore-challenge gossip flow (Hardcore.*)
--
-- Replaces the formerly-separate "Masochist Pete" (190011) NPC; its
-- spawn locations are merged into the Attuner's spawn list so the NPC
-- is reachable from every Classic starting zone AND every major
-- capital city.
-- ============================================================

-- Clean up any leftover hardcore-grave templates from a prior install.
DELETE FROM `gameobject_template` WHERE `type`=2 AND `CustomData1`=3643;

-- Drop the legacy hardcore NPC (190011) so it doesn't double-spawn.
DELETE FROM `creature` WHERE `id` = 190011;
DELETE FROM `creature_template` WHERE `entry` = 190011;
DELETE FROM `locales_creature` WHERE `entry` = 190011;

-- ============================================================
-- NPC Template
-- faction 35 = Friendly to all (works for both factions)
-- DisplayId 7949 = Spirit Healer-style ethereal model
-- ScriptName 'npc_attunement' is consumed by the C++ module
-- ============================================================

SET @AttunementEntry := 190014;

DELETE FROM `creature_template` WHERE `entry` = @AttunementEntry;
INSERT INTO `creature_template`
  (`entry`, `DisplayId1`, `DisplayIdProbability1`, `name`, `subname`, `GossipMenuId`,
   `minlevel`, `maxlevel`, `faction`, `NpcFlags`, `scale`, `rank`,
   `DamageSchool`, `MeleeBaseAttackTime`, `RangedBaseAttackTime`, `unitClass`, `unitFlags`,
   `CreatureType`, `CreatureTypeFlags`, `ScriptName`, `lootid`, `PickpocketLootId`, `SkinningLootId`,
   `AIName`, `MovementType`, `RacialLeader`, `RegenerateStats`, `MechanicImmuneMask`, `ExtraFlags`)
VALUES
  (@AttunementEntry, 7949, 100, 'Attuner of Paths', 'Adjuster of Fate', 0,
   60, 60, 35, 1, 1.1, 0,
   0, 2000, 0, 1, 0,
   7, 138936390, 'npc_attunement', 0, 0, 0,
   '', 0, 0, 1, 0, 0);

-- ============================================================
-- Spawns
-- map 0 = Eastern Kingdoms, map 1 = Kalimdor
-- Union of attunement's starting-zone spawns AND the (former)
-- hardcore module's capital-city spawns. 14 spawns total.
-- ============================================================

DELETE FROM `creature` WHERE `id` = @AttunementEntry;
INSERT INTO `creature`
  (`id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`,
   `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`)
VALUES
  -- ----- Starting zones (attunement origin) -----
  -- Northshire Abbey (near Marshal McBride)         map 0
  (@AttunementEntry, 0, 1, -8902.6,  -158.6,    82.0,  3.14, 300, 300, 0, 0),
  -- Coldridge Valley (near Sten Stoutarm)            map 0
  (@AttunementEntry, 0, 1, -6214.9,   332.2,   383.7,  3.14, 300, 300, 0, 0),
  -- Deathknell (near Shadow Priest Sarvis)           map 0
  (@AttunementEntry, 0, 1,  1843.3,  1643.9,    97.8,  3.14, 300, 300, 0, 0),
  -- Shadowglen (near Conservator Ilthalaine)         map 1
  (@AttunementEntry, 1, 1, 10328.9,   830.1,  1326.5,  3.14, 300, 300, 0, 0),
  -- Valley of Trials (near Kaltunk)                  map 1
  (@AttunementEntry, 1, 1,  -607.4, -4247.3,    39.0,  3.14, 300, 300, 0, 0),
  -- Camp Narache (near Grull Hawkwind)               map 1
  (@AttunementEntry, 1, 1, -2912.7,  -253.5,    53.0,  3.14, 300, 300, 0, 0),

  -- ----- Capital cities (hardcore origin) -----
  -- Stormwind                                         map 0
  (@AttunementEntry, 0, 1, -8999.00,   851.191,   29.621, 3.88538, 300, 300, 0, 0),
  -- Stormwind (auxiliary spawn)                       map 0
  (@AttunementEntry, 0, 1, -8903.58,  -108.401,   81.849, 4.08677, 300, 300, 0, 0),
  -- Ironforge gate                                    map 0
  (@AttunementEntry, 0, 1, -6213.26,   330.664,  383.719, 2.89842, 300, 300, 0, 0),
  -- Undercity                                         map 0
  (@AttunementEntry, 0, 1,  1859.94,  1560.67,    99.0791, 1.57723, 300, 300, 0, 0),
  -- Orgrimmar (main gate)                             map 1
  (@AttunementEntry, 1, 1,  1467.40, -4226.33,    58.9939, 1.19063, 300, 300, 0, 0),
  -- Darnassus                                         map 1
  (@AttunementEntry, 1, 1, 10327.10,   822.521,  1326.43,  2.53681, 300, 300, 0, 0),
  -- Durotar / Razor Hill                              map 1
  (@AttunementEntry, 1, 1,  -638.981, -4227.08,   38.1342, 5.47014, 300, 300, 0, 0),
  -- Thunder Bluff                                     map 1
  (@AttunementEntry, 1, 1, -2882.11,  -277.045,   53.9154, 2.37644, 300, 300, 0, 0);

-- ============================================================
-- NPC text — attunement greetings (50930, 50931, 50932)
-- ============================================================

SET @ATTUNE_TEXT_ID := 50930;
DELETE FROM `npc_text` WHERE `ID` BETWEEN @ATTUNE_TEXT_ID AND @ATTUNE_TEXT_ID+2;
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES
  (@ATTUNE_TEXT_ID,
   'Some walk this world swiftly, others savor every step. I can attune the pace at which experience flows to you, $N. Pick a path - or whisper your own.'),
  (@ATTUNE_TEXT_ID+1,
   'Whisper the rate you desire, $N. A number such as 1.5 or 7.'),
  (@ATTUNE_TEXT_ID+2,
   'This is a one-time gift, $N — once any character on your account uses it, no other character on the same account may use it again.\n\nIf you accept, you will receive:\n  - Level 60\n  - 500 gold\n  - Full Tier 0 dungeon set\n  - Class-appropriate accessories and weapons\n  - All flight paths unlocked\n  - Full world map revealed\n  - Every class spell learned (no trainer needed)\n\nDo you accept this passage?');

-- ============================================================
-- NPC text — hardcore challenge dialogs (50900-50916)
-- ============================================================

SET @TEXT_ID := 50900;
DELETE FROM `npc_text` WHERE `ID` BETWEEN @TEXT_ID AND @TEXT_ID+16;
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES
(@TEXT_ID,    "Ahoy, $N. If you are looking for a challenge you have come to the right place. Tell me, what challenge are you interested in?"),
(@TEXT_ID+1,  "Appologies $N. I can't provide you with any challenges at the moment."),
(@TEXT_ID+2,  "Oh, the hardcore challenge you say? I thought you'd never ask... If you accept this challenge, it means you will only have one life and if you die, that's it, no more retries. Are you up for it?"),
(@TEXT_ID+3,  "Hmm, the drop loot challenge? This challenge will make you drop some of your belongings every time you die, including gear and gold. What do you say?"),
(@TEXT_ID+4,  "Ah, the classic lose experience challenge. This challenge will make you lose some experience every time you die, and if you die enough you can even lose levels. Interested?"),
(@TEXT_ID+5,  "The self found challenge... I see you are a lone wolf like me. Just to clarify, this means you won't be able to use any help from any other adventurer, unless they are in the same challenge as you. Ready for it?"),
(@TEXT_ID+6,  "Hold your horses cowboy! It seems like your journey has already taken its course. To start this challenge you must speak with me with a fresh start, if you know what I mean..."),
(@TEXT_ID+7,  "I know you are excited about this challenge, but you seem like you already accepted it. What do you want, to double accept it?"),
(@TEXT_ID+8,  "Hahaha! That's what I'm talking about! Good luck! You will need it..."),
(@TEXT_ID+9,  "Oh, so you don’t want people attacking you during the challenge? That’s cute... But hey, who am I to judge..."),
(@TEXT_ID+10, "All right then... Consider it done. Careful not to break a nail out there."),
(@TEXT_ID+11, "Are you sure? You won't be able to retake the challenge if you drop it now."),
(@TEXT_ID+12, "Oh! That is such a relieve! I was wondering how long were you going to keep joking around..."),
(@TEXT_ID+13, "All done! Enemies will be able to attack you now."),
(@TEXT_ID+14, "All done! You are no longer doing the challenge."),
(@TEXT_ID+15, "Which XP rate would you like to have?"),
(@TEXT_ID+16, "All done! You xp rate has been changed.");

DELETE FROM `locales_npc_text` WHERE `entry` BETWEEN @TEXT_ID AND @TEXT_ID+14;
INSERT INTO `locales_npc_text` (`entry`, `text0_0_loc6`) VALUES
(@TEXT_ID,    "¡Hola, $N! Si estas buscando un reto, estás en el lugar indicado. Dime, ¿qué reto te interesa?"),
(@TEXT_ID+1,  "Mis disculpas $N. No tengo ningun desafío disponible en estos momentos."),
(@TEXT_ID+2,  "¿El desafío hardcore dices? Creí que nunca me lo preguntarías... Si aceptas este desafío, significa que solo tendrás una vida y, si mueres, se acabó, no habrá más reintentos. ¿Te animas?"),
(@TEXT_ID+3,  "Mmm, ¿el desafío de perder posesiones? Este desafío te hará soltar algunas de sus pertenencias cada vez que mueras, incluyendo equipo y oro. ¿Qué me dices?"),
(@TEXT_ID+4,  "Ah, el clásico desafío de perder experiencia. Este desafío te hará perder experiencia cada vez que mueras, y si mueres lo suficiente, incluso puedes perder niveles. ¿Te interesa?"),
(@TEXT_ID+5,  "El desafío del lobo solitario... Supongo que eres de los que prefiere ir solo que mal acompañado, eh?. Solo para aclarar, esto significa que no podrás usar la ayuda de ningún otro aventurero, a menos que esté en el mismo desafío que tú. ¿Estas listo?"),
(@TEXT_ID+6,  "¡Calma, vaquero! Parece que tu viaje ya ha tomado su curso. Para empezar este reto, debes hablar conmigo desde el principio, si sabes a que me refiero..."),
(@TEXT_ID+7,  "Sé que estás entusiasmado con este reto, pero parece que ya lo has aceptado. ¿Qué quieres, aceptarlo dos veces?"),
(@TEXT_ID+8,  "¡Jajaja! ¡Eso es lo que queria escuchar! ¡Mucha suerte! La necesitarás..."),
(@TEXT_ID+9,  "¿Ah, entonces no quieres que otras personas te ataquen durante el desafío? Qué monada... Pero bueno, ¿quién soy yo para juzgar?"),
(@TEXT_ID+10, "De acuerdo, ya estas listo. Ve con cuidado por ahi, no vaya a ser que te rompas una uña."),
(@TEXT_ID+11, "¿Estas seguro? No podras retomar el desafío si lo terminas ahora."),
(@TEXT_ID+12, "Ah! Que alivio! Ya estaba pensando que de verdad ibas en serio..."),
(@TEXT_ID+13, "¡Listo! Los enemigos podrán combatir contigo ahora."),
(@TEXT_ID+14, "¡Listo! Ya no estas realizando el desafío.");

-- ============================================================
-- mangos_string — gossip menu labels (12200-12214)
-- Note: entry 12208 ("change xp rate") is retained for binary compatibility
-- but is no longer surfaced in the menu — Attunement handles XP rates.
-- ============================================================

SET @STRING_ENTRY := 12200;
DELETE FROM `mangos_string` WHERE `entry` BETWEEN @STRING_ENTRY AND @STRING_ENTRY+14;
INSERT INTO `mangos_string` (`entry`, `content_default`, `content_loc6`) VALUES
(@STRING_ENTRY,    "I'm interested in the hardcore challenge", "Estoy interesado en el desafío hardcore"),
(@STRING_ENTRY+1,  "I would like to stop doing the hardcore challenge", "Me gustaria terminar el desafío hardcore"),
(@STRING_ENTRY+2,  "I'm interested in the drop loot challenge", 'Estoy interesado en el desafío de perder posesiones'),
(@STRING_ENTRY+3,  "I would like to stop doing the drop loot challenge", 'Me gustaria terminar el desafío de perder posesiones'),
(@STRING_ENTRY+4,  "I'm interested in the lose experience challenge", 'Estoy interesado en el desafío de perder experiencia'),
(@STRING_ENTRY+5,  "I would like to stop doing the lose experience challenge", 'Me gustaria terminar el desafío de perder experiencia'),
(@STRING_ENTRY+6,  "I'm interested in the self found challenge", 'Estoy interesado en el desafío del lobo solitario'),
(@STRING_ENTRY+7,  "I would like to stop doing the self found challenge", 'Me gustaria terminar el desafío del lobo solitario'),
(@STRING_ENTRY+8,  "I would like to change my xp rate", 'Me gustaria cambiar el ratio de XP'),
(@STRING_ENTRY+9,  "I accept the challenge!", '¡Acepto el desafío!'),
(@STRING_ENTRY+10, "Maybe later...", 'Quizas mas tarde...'),
(@STRING_ENTRY+11, "I would like to disable pvp fights", 'Me gustaria desactivar el combate pvp'),
(@STRING_ENTRY+12, "I would like to enable pvp fights", 'Me gustaria activat el combate pvp'),
(@STRING_ENTRY+13, "Yes, please!", '¡Si, por favor!'),
(@STRING_ENTRY+14, "Maybe not...", 'Quizas no...');

-- ============================================================
-- Custom spells — visible auras applied while a challenge is taken
-- (33500 Hardcore, 33501 Self Found)
-- ============================================================

SET @START_SPELL_ID := 33500;
SET @END_SPELL_ID := @START_SPELL_ID+1;
DELETE FROM `spell_template` WHERE `Id` BETWEEN  @START_SPELL_ID AND @END_SPELL_ID;
INSERT INTO `spell_template`              (`Id`,              `School`, `Category`, `CastUI`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `Stances`,   `StancesNot`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `CasterAuraState`, `TargetAuraState`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaxLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerlevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectBaseDice1`, `EffectBaseDice2`, `EffectBaseDice3`, `EffectDicePerLevel1`, `EffectDicePerLevel2`, `EffectDicePerLevel3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `SpellVisual`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName`,             `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `Rank1`,  `Rank2`, `Rank3`, `Rank4`, `Rank5`, `Rank6`, `Rank7`, `Rank8`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaxTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `MaxAffectedTargets`, `DmgClass`, `PreventionType`, `StanceBarOrder`, `DmgMultiplier1`, `DmgMultiplier2`, `DmgMultiplier3`, `MinFactionId`, `MinReputation`, `RequiredAuraVision`, `EffectBonusCoefficient1`, `EffectBonusCoefficient2`, `EffectBonusCoefficient3`, `EffectBonusCoefficientFromAP1`, `EffectBonusCoefficientFromAP2`, `EffectBonusCoefficientFromAP3`, `IsServerSide`, `AttributesServerside`) VALUES
/* Hardcore Challenge          (33500) */ (@START_SPELL_ID,    0,        0,          0,        0,        0,          2147483648,   0,              0,               1048576,         0,               0,           0,            0,         0,                    0,                    0,                 0,                 0,                  0,              0,                      0,                0,                    0,                       0,           101,          0,             0,          1,           1,            21,              0,           0,          0,                  0,               0,                       13,           0,       0,                0,             0,        0,        0,          0,          0,          0,          0,          0,          0,          0,          0,               0,               0,               0,               0,               0,               0,               0,              -1,                   -1,                         0,                               6,         0,         0,         1,                 0,                 0,                 1,                 0,                 0,                 0,                     0,                     0,                     0,                           0,                           0,                           0,                   0,                   0,                   0,                 0,                 0,                 1,                        0,                        0,                        0,                        0,                        0,                        0,                    0,                    0,                    4,                      0,                      0,                      0,                  0,                  0,                  0,                      0,                      0,                      0,                    0,                    0,                    0,                 0,                 0,                 0,                  0,                  0,                  0,                     0,                     0,                     0,                            0,                            0,                            222,           61,            0,              0,              'Hardcore Challenge',     '',           '',           '',            '',          '',           '',           '',           '',       '',      '',      '',      '',      '',      '',      '',      0,                    0,                       0,                   0,                0,                 0,                  0,                    0,          0,                -1,               1,                1,                1,                0,              0,               0,                    1,                         0.2,                       0,                         0,                               0,                               0,                               0,              0),
/* Self Found Challenge        (33501) */ (@START_SPELL_ID+1,  0,        0,          0,        0,        0,          2147483648,   0,              0,               1048576,         0,               0,           0,            0,         0,                    0,                    0,                 0,                 0,                  0,              0,                      0,                0,                    0,                       0,           101,          0,             0,          1,           1,            21,              0,           0,          0,                  0,               0,                       13,           0,       0,                0,             0,        0,        0,          0,          0,          0,          0,          0,          0,          0,          0,               0,               0,               0,               0,               0,               0,               0,              -1,                   -1,                         0,                               6,         0,         0,         1,                 0,                 0,                 1,                 0,                 0,                 0,                     0,                     0,                     0,                           0,                           0,                           0,                   0,                   0,                   0,                 0,                 0,                 1,                        0,                        0,                        0,                        0,                        0,                        0,                    0,                    0,                    4,                      0,                      0,                      0,                  0,                  0,                  0,                      0,                      0,                      0,                    0,                    0,                    0,                 0,                 0,                 0,                  0,                  0,                  0,                     0,                     0,                     0,                            0,                            0,                            222,           1573,          0,              0,              'Self Found Challenge',   '',           '',           '',            '',          '',           '',           '',           '',       '',      '',      '',      '',      '',      '',      '',      0,                    0,                       0,                   0,                0,                 0,                  0,                    0,          0,                -1,               1,                1,                1,                0,              0,               0,                    1,                         0.2,                       0,                         0,                               0,                               0,                               0,              0);
