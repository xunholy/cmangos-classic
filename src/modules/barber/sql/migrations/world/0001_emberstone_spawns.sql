-- Emberstone-specific spawn locations for Shav Cutiss (entry 190020).
--
-- Stormwind: outside Old Town, where players naturally pass on the way
-- to / from the Stockades and Trade District.
-- Orgrimmar: Valley of Honor area, near the gladiator pits.
--
-- Idempotent — DELETE-then-INSERT keyed on creature.id.

SET @Entry := 190020;

DELETE FROM `creature` WHERE `id` = @Entry;
INSERT INTO `creature`
  (`id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`,
   `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`)
VALUES
  (@Entry, 0, 1, -8748.08,   651.95, 105.43, 1.82255, 25, 25, 0, 0),  -- Stormwind
  (@Entry, 1, 1,  1713.69, -4207.01,  51.65, 3.97408, 25, 25, 0, 0);  -- Orgrimmar
