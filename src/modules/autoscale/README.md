# cmangos-autoscale

Dynamically scales mob HP in dungeons and raids based on the number of players (real or bot) in the instance. The intent is to keep under-staffed groups viable without making full groups easier than designed.

The module only ever scales **down**. At or above the baseline group size, mobs use their designed HP unmodified.

## What it does

On a configurable interval the module walks each active instance map, counts the players present, recomputes a per-creature HP factor from the formula below, and re-scales every creature in that instance from its **template** HP. Re-scaling is idempotent because the factor is always recomputed against the template — never chain-multiplied off the previous run.

```
factor   = clamp( (playerCount / baseline) ^ HpExponent, MinScale, MaxScale )
newMaxHp = templateMaxHp * factor
```

Worked examples (defaults: `HpExponent=0.85`, `MinScale=0.20`, `MaxScale=1.0`):

| Map | Baseline | Players | Factor | Effective HP |
|---|---|---|---|---|
| UBRS (5-man dungeon) | 5 | 5 | 1.00 | designed |
| UBRS solo | 5 | 1 | 0.30 | 30 % designed |
| MC (40-man raid) | 40 | 25 | 0.66 | 66 % designed |
| MC (40-man raid) | 40 | 40 | 1.00 | designed |
| MC (40-man raid) | 40 | 50 | 1.00 | designed (clamped by `MaxScale=1.0`) |
| ZG (20-man raid via `MapBaselines`) | 20 | 10 | 0.56 | 56 % designed |

## Build

```sh
cmake -DBUILD_MODULE_AUTOSCALE=ON ..
```

## Configure

Copy `autoscale.conf.dist` to `autoscale.conf` next to your `mangosd.conf`.

| Key | Default | Notes |
|---|---|---|
| `Autoscale.Enable` | `0` | Master switch — opt in |
| `Autoscale.RescanIntervalSeconds` | `10` | Lower = more responsive, higher = less CPU |
| `Autoscale.HpExponent` | `0.85` | `1.0` = linear, lower = harder on understaffed |
| `Autoscale.MinScale` | `0.20` | Floor — prevents solo runs from trivialising the world |
| `Autoscale.MaxScale` | `1.0` | Ceiling — leave at `1.0` to never scale up |
| `Autoscale.Baseline.Dungeon` | `5` | Designed party size for non-raid instances |
| `Autoscale.Baseline.RaidDefault` | `40` | Default raid size for maps not in `MapBaselines` |
| `Autoscale.MapBaselines` | `309:20,509:20` | `mapId:size,...` overrides. ZG=309, AQ20=509 |
| `Autoscale.MapBlacklist` | (empty) | `mapId,...` skipped entirely (e.g. scripted boss HP) |

## Scope and non-goals

* Only mob HP is scaled. Damage, armor, and threat curves are unchanged — they're often tuned per encounter and scaling them would change boss fights in surprising ways.
* Only instance maps (`IsDungeon() || IsRaid()`) are considered. World mobs are never touched.
* No per-creature opt-out yet. A creature in a blacklisted **map** is skipped, but there is no per-entry exemption. If a specific boss needs static HP inside an otherwise-scaled raid, add the map to `MapBlacklist` and accept that the trash also stops scaling.
* The module recomputes from the creature template each tick — manual `.npc setmaxhp` from a GM gets overwritten on the next rescan.

## In-game / DB surface

* No NPC entries, no DB tables, no chat commands. Pure in-memory HP modifier.
* Logs scale changes at `INFO` when `Autoscale.RescanIntervalSeconds` ticks observe a player-count change for an instance — useful for confirming behaviour without enabling debug logs.

## Provenance

Originated in this fork — see [`NOTES.md`](./NOTES.md).
