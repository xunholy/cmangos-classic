-- Emberstone-specific spawn locations for the Dual Specialization Crystal
-- (entry 190024) — kept at the upstream-suggested coordinates for now.
--
-- These positions originally came from the upstream dualspec install SQL;
-- they're slightly off the beaten path (Stormwind Mage Quarter fountain
-- level z=29.6; Orgrimmar elevated z=58.99). Move when ready by editing
-- this file's values via a follow-up migration (NNNN+1).
--
-- Idempotent — DELETE-then-INSERT keyed on creature.id.

SET @Entry := 190024;

DELETE FROM `creature` WHERE `id` = @Entry;
INSERT INTO `creature`
  (`id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`,
   `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`)
VALUES
  (@Entry, 0, 1, -8988.56,  849.754, 29.621,  2.27687, 25, 25, 0, 0),  -- Stormwind Mage Quarter fountain area
  (@Entry, 1, 1,  1471.63, -4216.46, 58.9942, 4.35778, 25, 25, 0, 0);  -- Orgrimmar central elevated
