-- Hearthstone (item 6948) cooldown: 1h → 15m.
--
-- Spell 8690 has TWO cooldown fields and the engine uses the MAX of them:
--   RecoveryTime         — per-spell cooldown
--   CategoryRecoveryTime — cooldown on the spell's Category (89 = Hearthstone)
-- Updating only RecoveryTime leaves the Category at 60min so the cap stays.
-- Both must be set to 900000ms (15 * 60s * 1000ms) for the cooldown to
-- actually drop to 15 min.
--
-- Idempotent: re-running this just rewrites the same values.
-- Takes effect after mangosd restart (spell_template loads at startup).

UPDATE `spell_template`
   SET `RecoveryTime`         = 900000,
       `CategoryRecoveryTime` = 900000
 WHERE `Id` = 8690;
