# Module: autoscale

Originated in this fork. There is no separate upstream repository to track — changes land directly on `main` here.

## Why it lives in-tree

Autoscale is tightly coupled to cmangos's instance map lifecycle (`Map::Update`, `Map::AddToMap`, `IsDungeon()/IsRaid()`) and to creature template lookups. Splitting it into a separate repo would force every change to land in two places, with no compensating benefit since it isn't useful outside cmangos.

## If you fork

Drop the module by:

1. Removing the `src/modules/autoscale/` directory.
2. Removing the `AUTOSCALE=in-tree` line from `src/modules/modules/modules.conf`.
3. Building without `-DBUILD_MODULE_AUTOSCALE=ON` (or that flag will silently no-op because the directory is gone).

No DB cleanup needed — the module has no tables.
