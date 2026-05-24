# Fork: PlayerBots

This directory is our long-term-maintained fork of `cmangos/playerbots`, living inside the cmangos-classic monorepo (no submodule, no subtree). We pull individual upstream commits in as they land — cherry-picked, not wholesale-merged — and carry our own patches on top.

Previously the source was fetched at CMake configure time via `FetchContent_Declare(GIT_TAG "master")`. That meant every build pulled whatever upstream HEAD happened to be — non-reproducible, no opportunity for local patches, configure-time network dependency. We replaced that with the in-tree copy and now treat the code as ours; see the original `vendor:` and `build:` commits for the import + CMake change.

## Upstream tracking

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/cmangos/playerbots |
| Branch       | master |
| Last upstream commit pulled in | `2165ec47ea240d9863031b9afcd7fb853f7988ee` (fix checkmountstateaction bug for vanilla, #296, 2026-05-22) |
| Initial import baseline | `98487e010c57dd1c6aad611639d8f9daede1ae47` (Remove problematic handlers, #293, 2026-05-19) |

To see what's landed upstream since we last pulled, grep this repo's `git log` for `cherry picked from commit <sha> in cmangos/playerbots` trailers, take the most recent SHA, and `git log <sha>..origin/master` in an upstream checkout.

## Local divergence from upstream

Patches we carry that aren't upstream (yet). When pulling new upstream commits in, check that none of these files are about to be overwritten.

| Commit (in this fork) | File | Description |
|---|---|---|
| `fix(playerbot): guard EnchantItem against bad seed-data slot pairings` | `playerbot/strategy/actions/UpdateGearAction.cpp` | Validates `EquippedItemInventoryTypeMask` before dispatching to `EnchantItemT`, preventing chest-only enchants from being applied to ring slots (the "Ring +2 All Stats" antipattern). |
| `fix(playerbot): make Engine::Reset reentrancy/exception-safe (UAF on TriggerNode)` | `playerbot/strategy/Engine.cpp` | Hardens `Engine::Reset` against reentrant `ChangeStrategy` calls and exception unwinds that would otherwise leave dangling `TriggerNode*` pointers in `triggers`. Discovered on PTR under 500-bot load — deterministic SIGSEGV after ~24 min. |

Consider opening upstream PRs for each so we eventually shed them.

## Pulling new upstream commits

We cherry-pick across repos by hand (no shared git history). Each upstream commit lands as its own commit in this repo, with the original author/date/message preserved and a `(cherry picked from commit <sha> in cmangos/playerbots)` trailer for traceability.

```bash
# 1. Get the upstream tree (anywhere outside this repo)
git clone https://github.com/cmangos/playerbots.git /tmp/playerbots-upstream

# 2. Find the last SHA we pulled in (see "Upstream tracking" above, or grep for trailers)
LAST=2165ec47ea240d9863031b9afcd7fb853f7988ee

# 3. List what's new and decide what's worth taking
git -C /tmp/playerbots-upstream log ${LAST}..origin/master --oneline

# 4. For each commit to pull, generate a patch with format-patch...
git -C /tmp/playerbots-upstream format-patch -1 -o /tmp/pb-patches/ <upstream-sha>

# 5. ...then apply with git apply (NOT git am — git am tries to verify pre-image blobs
#    via the patch's `index <sha>..<sha>` header, which fails across repos)
cd <this repo>
git apply --index --directory=src/modules/PlayerBots/ /tmp/pb-patches/0001-*.patch

# 6. Commit with original author + date + cherry-pick trailer
git -c format.signoff=false commit \
  --author="<Upstream Author Name> <upstream@email>" \
  --date="<original date from patch>" \
  -m "<original subject>

<original body, if any>

(cherry picked from commit <upstream-sha> in cmangos/playerbots)"

# 7. Update the "Last upstream commit pulled in" row in this NOTES.md
```

Notes on the above:
- `--directory=src/modules/PlayerBots/` remaps upstream's `playerbot/...` paths to our nested location.
- `-c format.signoff=false` is only needed if you've set `format.signoff = true` globally (which causes `format-patch` to inject your sign-off into the upstream author's commit — wrong).
- If an upstream commit touches a file listed in "Local divergence" above, resolve the conflict by hand and note in the commit message that the local patch was reapplied/dropped.

## Size note

The import is ~139 MB / 1 022 files. Most of the bulk is `sql/world/{tbc,wotlk}/` seed data (travel nodes, named locations) that classic-only deployments do not use. Kept verbatim to match upstream; if disk pressure ever matters, a future commit can prune `sql/world/tbc/` and `sql/world/wotlk/` without affecting the C++ build or the classic SQL deployment data.

## Removing the module

Drop PlayerBots support by:

1. Removing the `src/modules/PlayerBots/` directory.
2. Building without `-DBUILD_PLAYERBOTS=ON`.
3. The CMake guard in `src/CMakeLists.txt` will refuse to configure with `BUILD_PLAYERBOTS=ON` and the directory missing — clear error message rather than mysterious link failures.

Optional DB cleanup (`ai_playerbot_*` tables across `classicmangos` and `classiccharacters`) is documented in the upstream README — apply the `sql/other/database_drop_classic.sql` script.
