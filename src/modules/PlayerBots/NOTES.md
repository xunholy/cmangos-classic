# Vendored module: PlayerBots

This directory is a vendored copy of the upstream `cmangos/playerbots` repo. The `.git` directory was stripped on import. Treat the code as part of this fork — modify freely.

Previously the source was fetched at CMake configure time via `FetchContent_Declare(GIT_TAG "master")`. That meant every build pulled whatever upstream HEAD happened to be — non-reproducible, no opportunity for local patches, configure-time network dependency. Switched to a vendored copy as part of this commit series; see the surrounding `vendor:` and `build:` commits for the import + CMake change, and `fix(playerbot):` for the first local patch.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/cmangos/playerbots |
| Branch       | master |
| Commit SHA   | 98487e010c57dd1c6aad611639d8f9daede1ae47 |
| Commit date  | 2026-05-19 |
| Commit title | Remove problematic handlers (#293) |
| Imported on  | 2026-05-21 |

## Local patches we carry on top

| Commit (in this fork) | File | Description |
|---|---|---|
| `fix(playerbot): guard EnchantItem against bad seed-data slot pairings` | `playerbot/strategy/actions/UpdateGearAction.cpp` | Validates `EquippedItemInventoryTypeMask` before dispatching to `EnchantItemT`, preventing chest-only enchants from being applied to ring slots (the "Ring +2 All Stats" antipattern). |

When importing new upstream changes, re-apply each of these patches on top, or open upstream PRs to land them at the source.

## Bumping the vendored snapshot

```bash
# 1. Pull the new upstream tree (no .git history needed in the import)
git clone --depth=1 --branch=master https://github.com/cmangos/playerbots.git /tmp/playerbots-new

# 2. Stage the diff against the current vendored copy
cd <fork checkout>
rsync -a --delete --exclude='.git' --exclude='NOTES.md' \
    /tmp/playerbots-new/ src/modules/PlayerBots/

# 3. Re-apply any local patches that didn't land upstream (check the table above)

# 4. Bump the SHA + Commit date + Imported on rows in this NOTES.md

# 5. Commit:
git add src/modules/PlayerBots/
git commit -m "vendor: bump cmangos/playerbots to <new-sha>"
```

Always check upstream's commit log since the previous import for behavior changes that affect what you want to keep:

```bash
git -C /tmp/playerbots-new log 98487e010c57dd1c6aad611639d8f9daede1ae47..HEAD --oneline
```

## Size note

The import is ~139 MB / 1 022 files. Most of the bulk is `sql/world/{tbc,wotlk}/` seed data (travel nodes, named locations) that classic-only deployments do not use. Kept verbatim to match upstream exactly; if disk pressure ever matters, a future commit can prune `sql/world/tbc/` and `sql/world/wotlk/` without affecting the C++ build or the classic SQL deployment data.

## Removing the module

Drop PlayerBots support by:

1. Removing the `src/modules/PlayerBots/` directory.
2. Building without `-DBUILD_PLAYERBOTS=ON`.
3. The CMake guard in `src/CMakeLists.txt` will refuse to configure with `BUILD_PLAYERBOTS=ON` and the directory missing — clear error message rather than mysterious link failures.

Optional DB cleanup (`ai_playerbot_*` tables across `classicmangos` and `classiccharacters`) is documented in the upstream README — apply the `sql/other/database_drop_classic.sql` script.
