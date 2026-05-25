# Vendored module: attunement

This directory is a vendored copy of the upstream `xunholy/cmangos-attunement` repo. The `.git` directory was stripped on import. Treat the code as part of this fork — modify freely.

The module is the result of amalgamating three formerly-separate modules:

* `xunholy/cmangos-attunement` — per-player XP rate via the Attuner of Paths NPC.
* `flekz-games/cmangos-hardcore` — death penalties, loot drops, self-found, PvP toggle. Folded in so the Attuner NPC is the single source of truth and there's one entry (190014) to spawn / configure / reason about.
* `flekz-games/cmangos-dualspec` — dual talent specialization, talent + action-bar persistence per spec. Folded in 2026-05-25; the standalone Dual Specialization Crystal NPC (entry 190024) is despawned by this module's `sql/install/world/world.sql`, and the Attuner now exposes the spec-switch gossip. Inventory item 17731 keeps its script binding so existing copies on live continue to work. The four `custom_dualspec_*` characters-DB tables are kept under their original names (no migration of player state) — they hold ~18k characters' worth of spec + action-bar data on live that can't be reconstructed from the standard `character_*` tables.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/xunholy/cmangos-attunement |
| Branch       | main |
| Commit SHA   | b47d7aa1ab3143f61f31fce4aa66bb81a34f1e1e |
| Commit date  | 2026-05-17 |
| Imported on  | 2026-05-21 |
| Notes        | Hardcore (flekz-games/cmangos-hardcore) and Dualspec (flekz-games/cmangos-dualspec) are both amalgamated in — there are no separate hardcore / dualspec modules |

## Cherry-picking upstream fixes

```bash
# Attunement (current home for both XP rate and hardcore)
git clone https://github.com/xunholy/cmangos-attunement.git /tmp/upstream-attunement
cd /tmp/upstream-attunement
git log b47d7aa1ab3143f61f31fce4aa66bb81a34f1e1e..HEAD

# Pre-amalgamation hardcore (only relevant if you're back-porting a fix
# that hasn't been carried across)
git clone https://github.com/flekz-games/cmangos-hardcore.git /tmp/upstream-hardcore

# Pre-amalgamation dualspec (same caveat — only relevant for back-porting
# upstream fixes; the in-tree code is the canonical version now)
git clone https://github.com/flekz-games/cmangos-dualspec.git /tmp/upstream-dualspec
```

After importing changes, bump the SHA + date in the table above so the next contributor sees the new baseline.

## Removing the module

Drop the module by:

1. Removing the `src/modules/attunement/` directory.
2. Removing the `ATTUNEMENT=...` line from `src/modules/modules/modules.conf`.
3. Building without `-DBUILD_MODULE_ATTUNEMENT=ON`.
4. Optional DB cleanup (see `sql/uninstall/` inside the module dir for the canonical uninstall scripts, OR run by hand):
   * `DROP TABLE custom_attunement_player_config;` (characters DB)
   * `DROP TABLE custom_hardcore_loot_gameobjects, custom_hardcore_loot_tables, custom_hardcore_grave_gameobjects, custom_hardcore_player_config, custom_hardcore_player_deathlog;` (characters DB)
   * `DELETE FROM creature_template WHERE entry = 190014;` (world DB)
