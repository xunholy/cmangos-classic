# cmangos-autoscale

Dynamically scales mob HP **and outgoing damage** in dungeons and raids based on the number of players (real or bot) in the instance. The intent is to keep under-staffed groups viable without making full groups easier than designed.

The module only ever scales **down**. At or above the baseline group size, mobs use their designed HP and damage unmodified.

## What it does

On a configurable interval the module walks each active instance map, counts the players present, recomputes per-creature HP and damage factors from the formula below, and re-scales every creature in that instance from its **template** HP. Re-scaling is idempotent because the factor is always recomputed against the template — never chain-multiplied off the previous run.

```
hpFactor  = clamp( (playerCount / baseline) ^ HpExponent,  MinScale,    MaxScale    )
dmgFactor = clamp( (playerCount / baseline) ^ DmgExponent, MinDmgScale, MaxDmgScale )
newMaxHp  = templateMaxHp * hpFactor
```

Damage scaling hooks `Unit::DealDamage` via the modules framework's `OnPreDealDamage` hook, so it covers melee, spell direct damage, DoT splits, and damage shields uniformly — anything that funnels through `DealDamage`.

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
| `Autoscale.DmgExponent` | `0.85` | Same shape as `HpExponent` but for outgoing creature damage |
| `Autoscale.MinDmgScale` | `0.20` | Damage floor — prevents trivial-damage 1-shots of soloers |
| `Autoscale.MaxDmgScale` | `1.0` | Damage ceiling — leave at `1.0` to never scale up |
| `Autoscale.Baseline.Dungeon` | `5` | Designed party size for non-raid instances |
| `Autoscale.Baseline.RaidDefault` | `40` | Default raid size for maps not in `MapBaselines` |
| `Autoscale.MapBaselines` | `309:20,509:20` | `mapId:size,...` overrides. ZG=309, AQ20=509 |
| `Autoscale.MapBlacklist` | (empty) | `mapId,...` skipped entirely (e.g. scripted boss HP) |

## Scope and non-goals

* Mob HP and outgoing damage are scaled. Armor and threat curves are not — they're often tuned per encounter and touching them would shift fights in surprising ways.
* Damage scaling applies to anything that flows through `Unit::DealDamage` (melee, spell direct, DoT splits, damage shields). Scripted boss mechanics that compute damage outside that path are not covered; if a specific encounter needs its damage left alone, blacklist the map.
* Only instance maps (`IsDungeon() || IsRaid()`) are considered. World mobs are never touched.
* No per-creature opt-out yet. A creature in a blacklisted **map** is skipped, but there is no per-entry exemption. If a specific boss needs static HP/damage inside an otherwise-scaled raid, add the map to `MapBlacklist` and accept that the trash also stops scaling.
* The module recomputes from the creature template each tick — manual `.npc setmaxhp` from a GM gets overwritten on the next rescan.

## In-game / DB surface

* No NPC entries, no DB tables, no chat commands. Pure in-memory HP modifier.
* Logs scale changes at `INFO` when `Autoscale.RescanIntervalSeconds` ticks observe a player-count change for an instance — useful for confirming behaviour without enabling debug logs.

## Provenance

Originated in this fork — see [`NOTES.md`](./NOTES.md).
