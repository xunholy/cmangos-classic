-- Stock CMaNGOS ships the read-only .lookup subcommands (item, creature,
-- quest, spell, object, itemset, faction, skill, taxinode) at security
-- level 3 (Administrator). That blocks GMs (level 2) from basic lookups
-- like ".lookup item Hearthstone" — needed to ID NPCs/items/spells when
-- answering player tickets. The commands are read-only (return IDs, no
-- world mutation), so granting them to GMs is safe.
--
-- `.reload command` in the mangosd console picks the change up without
-- a worldserver restart, when applied to an already-running mangosd.
UPDATE `command`
   SET `security` = 2
 WHERE `name` IN (
   'lookup creature',
   'lookup faction',
   'lookup item',
   'lookup itemset',
   'lookup object',
   'lookup quest',
   'lookup skill',
   'lookup spell',
   'lookup taxinode'
 );
