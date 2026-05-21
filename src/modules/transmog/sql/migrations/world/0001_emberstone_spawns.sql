-- Emberstone-specific spawn locations for Magister Stellaria (entry 190010).
--
-- Stormwind: Trade District tailor area, chosen to avoid overlap with
-- attunement's Attuner of Paths (190014).
-- Orgrimmar: The Drag, tailor district.
--
-- Idempotent — DELETE-then-INSERT keyed on creature.id.

SET @Entry := 190010;

DELETE FROM `creature` WHERE `id` = @Entry;
INSERT INTO `creature`
  (`id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`,
   `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`)
VALUES
  (@Entry, 0, 1, -8930.96,   770.06, 100.21, 4.70, 25, 25, 0, 0),  -- Stormwind Trade District tailor area
  (@Entry, 1, 1,  1623.00, -4419.00,  22.00, 4.00, 25, 25, 0, 0);  -- Orgrimmar Drag's tailor district
