-- Emberstone-specific spawn locations for training dummies:
--   190021 Grandmaster's Training Dummy
--   190022 Advanced Training Dummy
--   190023 Beginner Training Dummy
--
-- 6 locations × 3 dummies = 18 spawns. Locations are the major-city /
-- class-trainer areas in Classic. Some coords (10000+, 2258+ on map 1)
-- are Teldrassil / Darnassus rather than Kalimdor mainland — that's
-- intentional, follows upstream.
--
-- The trainingdummies module's OnAddToWorld hook does the rest
-- (REACT_PASSIVE + UNIT_STAT_NO_COMBAT_MOVEMENT). The template values
-- live in src/modules/trainingdummies/sql/install/world/world_classic.sql.
--
-- Idempotent — DELETE-then-INSERT keyed on creature.id.

DELETE FROM `creature` WHERE `id` IN (190021, 190022, 190023);
INSERT INTO `creature`
  (`id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`,
   `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`)
VALUES
  -- Grandmaster (190021)
  (190021, 1, 1, -1416.73,   -76.4812, 158.526,  1.04029, 25, 25, 0, 0),
  (190021, 0, 1,  1768.18,   352.747,  -62.2883, 4.4141,  25, 25, 0, 0),
  (190021, 1, 1,  2151.77, -4632.69,    50.4428, 3.59001, 25, 25, 0, 0),
  (190021, 1, 1, 10000.5,   2258.5,   1329.8,    3.99055, 25, 25, 0, 0),
  (190021, 0, 1, -4910.96, -1151.19,   501.449,  3.09082, 25, 25, 0, 0),
  (190021, 0, 1, -8795.57,   363.195,  101.021,  5.41645, 25, 25, 0, 0),
  -- Advanced (190022)
  (190022, 1, 1, -1422.05,   -72.171,  157.476,  0.693121, 25, 25, 0, 0),
  (190022, 0, 1,  1780.49,   333.857,  -62.2898, 2.37992,  25, 25, 0, 0),
  (190022, 1, 1,  2152.63, -4639.71,    50.3799, 3.22873,  25, 25, 0, 0),
  (190022, 1, 1, 10006.2,   2252.62,  1329.75,   3.99449,  25, 25, 0, 0),
  (190022, 0, 1, -4936.94, -1138.88,   501.46,   6.12402,  25, 25, 0, 0),
  (190022, 0, 1, -8789.85,   367.182,  101.021,  5.33006,  25, 25, 0, 0),
  -- Beginner (190023)
  (190023, 1, 1, -1410.51,   -78.656,  158.935,  1.37014,  25, 25, 0, 0),
  (190023, 0, 1,  1783.91,   341.82,   -62.3358, 2.8865,   25, 25, 0, 0),
  (190023, 1, 1,  2148.06, -4627.23,    50.9305, 3.88454,  25, 25, 0, 0),
  (190023, 1, 1,  9993.03,  2260.47,  1330.81,   4.48136,  25, 25, 0, 0),
  (190023, 0, 1, -4938.02, -1148.3,    501.497,  6.14209,  25, 25, 0, 0),
  (190023, 0, 1, -8800.62,   358.618,  101.021,  5.44787,  25, 25, 0, 0);
