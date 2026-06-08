# Vendored module: paladinpower

This directory is a vendored copy of an upstream module. It is no longer
linked to the upstream repo via git (the `.git` directory was stripped on
import). Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/Redbu11dev/cmangos-paladinpower |
| Branch       | main |
| Commit SHA   | 40cb9a4649f41eab871c3c3b067eed39780fe293 |
| Commit date  | 2024-08-11 |
| Imported on  | 2026-06-08 |

## What it does

Gives Classic paladins their cut melee abilities back, learned automatically
by level (and removed cleanly when disabled). Classic-only.

- Crusader Strike (ranks 2537/8823/8824/10336/10337 at levels 10/22/34/46/58).
  Present in this fork's world DB via `z2691_01_mangos_spell_template.sql` —
  melee Physical, instant, **no cooldown (GCD only)**, flat school damage
  (Effect1 SCHOOL_DAMAGE, not weapon-scaling). If a 4-6s CD / weapon scaling
  is wanted, that is a separate `spell_template` override on these IDs.
- Old Holy Strike (the `zzOLDHoly Strike` cut ranks) — left **disabled**.

## Known upstream quirks (left verbatim)

- `PaladinpowerModuleConfig.cpp` uses a malformed `#ifndef EXPANSION=0`
  preprocessor guard. GCC/Clang parse it as `#ifndef EXPANSION` (extra-tokens
  warning only; no `-Werror` in this fork). `EXPANSION` is always defined
  (0/1/2 keyed on CMAKE_PROJECT_NAME), so the force-disable branch is dead.
  Not fixed, to keep cherry-picks clean.
- README step 3 references `src/modules/transmog/src/paladinpower.conf.dist.in`
  — an upstream copy-paste typo; the real path is this module's own `src/`.
  Irrelevant here: the runtime conf is delivered by the k8s ConfigMap.

## Playerbot note

The `#ifdef ENABLE_PLAYERBOTS` block in `PaladinpowerModule.cpp` is empty, so
the module's level-up grant does not survive `PlayerbotFactory::ClearSpells()`
on random-bot re-randomization. For random bots to learn CS, add the rank IDs
to `AiPlayerbot.RandomBotSpellIds` (done on the PTR realm). Human/GM paladins
get CS via the normal `OnGiveLevel`/`OnLoadFromDB` hooks regardless.

## Cherry-picking upstream fixes

```bash
git clone https://github.com/Redbu11dev/cmangos-paladinpower.git /tmp/upstream-paladinpower
cd /tmp/upstream-paladinpower
git log 40cb9a4649f41eab871c3c3b067eed39780fe293..HEAD
# Cherry-pick or copy individual changes into src/modules/paladinpower/
```
