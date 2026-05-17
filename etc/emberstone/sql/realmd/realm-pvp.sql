-- Flip the realm to PvP type.
--
-- The realmlist.icon field controls the badge shown next to the realm name
-- on the character-select screen. Values: 0=Normal, 1=PvP, 4=Normal,
-- 6=RP, 8=RPPvP. Must match mangosd.conf GameType for the in-game
-- enforcement and the lobby badge to agree.
--
-- Single-realm setup — no WHERE filter; all rows get the flip.
-- Idempotent.

UPDATE `realmlist`
   SET `icon` = 1;
