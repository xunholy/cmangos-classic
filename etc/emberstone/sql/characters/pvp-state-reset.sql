-- Clean stale per-character PvP / BG state at the realm-type flip.
--
-- character_battleground_data tracks a character's active BG slot and
-- queue state. After a realm-type change, lingering rows can reference
-- BGs that no longer exist (or were never properly torn down). This is
-- the same crash class our 0002-WorldportAck patch defends against; a
-- one-shot wipe at the flip-over removes the trigger entirely.
--
-- Honor totals (characters.totalHonorPoints / totalKills) and PvP rank
-- (characters.honor_rank_points) are NOT touched — those are durable
-- progression stats that should survive the realm change.
--
-- playerFlags (PLAYER_FLAGS_PVP_DESIRED etc.) are derived per-tick by
-- mangosd from GameType + zone + faction on each player update, so no
-- bit-clearing is needed there.

TRUNCATE TABLE `character_battleground_data`;
