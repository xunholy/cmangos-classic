-- Hearthstone (item 6948) cooldown: 1h → 15m.
--
-- The cooldown lives on the spell (8690), not the item — cmangos reads
-- spell data from the SQL spell_template table at world load, so an
-- UPDATE here is the canonical override (no DBC file edit needed).
--
-- RecoveryTime is in milliseconds. 900000ms = 15 * 60s * 1000ms.
-- Idempotent: re-running this just rewrites the same value.

UPDATE `spell_template`
   SET `RecoveryTime` = 900000
 WHERE `id` = 8690;
