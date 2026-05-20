-- The AiPlayerbot module's seed data for ai_playerbot_enchants ships 54
-- rows (27 class/spec combos × 2 ring slots) that label spell 13700
-- ("Enchant Chest - Lesser Stats", +2 all stats, enchant_id 866) as
-- "Ring +2 All Stats" and apply it to slots 10 and 11. The module's
-- EnchantItem code path bypasses the spell's normal item-class restriction
-- (spell 13700 is chest-only) and writes the chest enchant onto rings
-- in memory — so every bot ends up with +4 all stats from rings that no
-- real player can reproduce.
--
-- Our cmangos data set has no vanilla ring-enchant spells available
-- (22536 missing; 22538 is "Nef Trans" in spell_template, not Enchant
-- Ring - Stats), so no player can enchant a ring on this realm. Bots
-- shouldn't get a backdoor advantage that real mechanics can't deliver.
--
-- Take effect: bot ring enchants drop off as the random-bot manager
-- re-randomizes each bot's gear on the next RandomGearUpgrade tick.
-- Worldserver restart is not required.
--
-- If vanilla ring enchant spells are later added to spell_template
-- (enchant IDs 2929 Minor Stats / 2931 Stats), substitute the legit
-- IDs back in via a follow-up migration.
DELETE FROM `ai_playerbot_enchants`
 WHERE `spellid` = 13700
   AND `slotid` IN (10, 11);
