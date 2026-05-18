SET @Entry := 190010;

DELETE FROM `creature_template` WHERE `entry` = @Entry;
INSERT INTO `creature_template` (`entry`, `DisplayId1`, `DisplayIdProbability1`, `name`, `subname`, `GossipMenuId`, `minlevel`, `maxlevel`, `faction`, `NpcFlags`, `scale`, `rank`, `DamageSchool`, `MeleeBaseAttackTime`, `RangedBaseAttackTime`, `unitClass`, `unitFlags`, `CreatureType`, `CreatureTypeFlags`, `lootid`, `PickpocketLootId`, `SkinningLootId`, `AIName`, `MovementType`, `RacialLeader`, `RegenerateStats`, `MechanicImmuneMask`, `ExtraFlags`) VALUES
(@Entry, 2240, 100, "Magister Stellaria", "Transmogrifier", 0, 60, 60, 35, 1, 1, 0, 0, 2000, 0, 1, 0, 7, 138936390, 0, 0, 0, '', 0, 0, 1, 0, 0);

DELETE FROM `locales_creature` WHERE `entry` = @Entry;
INSERT INTO `locales_creature` (`entry`, `name_loc6`, `subname_loc6`) VALUES (@Entry, 'Magister Cielo Estrellado', 'Transmogrificadora');

DELETE FROM `creature` WHERE `id` = @Entry;
-- Stormwind: Trade District tailor area near Lillian Cooper.
-- Moved from the upstream spawn (-8999, 851) which overlapped attunement's
-- Attuner of Paths (entry 190014) — same x/y/z/orientation caused
-- click-collision and overlapping nameplates.
INSERT INTO `creature` (`id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`) VALUES (@Entry, 0, 1, -8930.96, 770.06, 100.21, 4.70, 25, 25, 0, 0);
-- Orgrimmar: Drag's tailor district. Moved from upstream (1467, -4226) for
-- consistency with the Stormwind relocation.
INSERT INTO `creature` (`id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`) VALUES (@Entry, 1, 1, 1623.00, -4419.00, 22.00, 4.00, 25, 25, 0, 0);
