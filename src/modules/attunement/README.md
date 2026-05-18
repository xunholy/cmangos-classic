# cmangos-attunement

Unified XP-rate and hardcore-challenge module for [CMaNGOS Classic](https://github.com/cmangos/mangos-classic).

Players visit the **Attuner of Paths** NPC at any starting zone or capital city and use a single gossip menu to:

- adjust their personal XP multiplier (presets 1×/2×/5×/10× or a custom value),
- boost to level 60 (one-shot per character) with class-appropriate gear,
- opt into hardcore challenges (single life, drop loot on death, lose XP on death, self-found, PvP toggle).

Hardcore was previously a separate module (`src/modules/hardcore`). It has been amalgamated here so the Attuner is the single source of truth for both subsystems and there is one NPC entry (190014) to spawn, configure, and reason about.

## Build

```sh
cmake -DBUILD_MODULE_ATTUNEMENT=ON ..
```

CMake flag exposed: `BUILD_MODULE_ATTUNEMENT`. The module's `CMakeLists.txt` defines `-DENABLE_ATTUNEMENT`. No separate `BUILD_MODULE_HARDCORE` flag exists anymore — hardcore is gated at runtime by `Hardcore.Enable` in the config.

## Configure

Copy `attunement.conf.dist` to `attunement.conf` next to your `mangosd.conf`. Key sections:

```ini
# Attunement (XP rate)
Attunement.Enable = 1
Attunement.DefaultRate = 1.0
Attunement.MinRate = 0.1
Attunement.MaxRate = 0           # 0 = uncapped

# Optional visible-aura tiers (operator-chosen spell IDs)
Attunement.Aura.Tier1.SpellId = 0   # 1.0×
Attunement.Aura.Tier2.SpellId = 0   # >1×–2×
Attunement.Aura.Tier3.SpellId = 0   # >2×–5×
Attunement.Aura.Tier4.SpellId = 0   # >5×–25×
Attunement.Aura.Tier5.SpellId = 0   # >25×

# Hardcore (death penalties, challenges)
Hardcore.Enable = 1
Hardcore.PlayerConfig = 1          # gossip-opt-in per player
Hardcore.SpawnGrave = 1
Hardcore.GraveGameObjectID = 61
Hardcore.GraveMessage = "Here lies <PlayerName>"
Hardcore.ReviveDisabled = 1        # one-life mode (opt-in)
Hardcore.DropGear = 0.5            # fraction of gear dropped on death
Hardcore.DropItems = 0.5
Hardcore.DropMoney = 0.5
Hardcore.DisablePVP = 1            # allow players to disable PvP
Hardcore.SelfFound = 1             # allow opt-in to self-found
# ...see attunement.conf.dist for the full list
```

`Hardcore.CustomXPRates` no longer exists — Attunement owns XP rates exclusively.

## Database

Apply the install scripts:

```sh
mariadb -u mangos -pmangos classiccharacters < sql/install/characters/characters.sql
mariadb -u mangos -pmangos classicmangos    < sql/install/world/world.sql
```

The world script creates NPC entry `190014` ("Attuner of Paths") and spawns it at **14 locations** — the 6 Classic starting zones plus the 8 main-city / capital spawns inherited from the former hardcore NPC ("Masochist Pete", 190011, which is removed):

| Type           | Zone(s)                                                                                  |
|----------------|------------------------------------------------------------------------------------------|
| Starting zones | Northshire, Coldridge Valley, Deathknell, Shadowglen, Valley of Trials, Camp Narache      |
| Capitals       | Stormwind (×2), Ironforge, Undercity, Orgrimmar, Darnassus, Razor Hill, Thunder Bluff     |

To remove the module entirely:

```sh
mariadb -u mangos -pmangos classiccharacters < sql/uninstall/characters/characters.sql
mariadb -u mangos -pmangos classicmangos    < sql/uninstall/world/world.sql
```

## Schema

```sql
-- Attunement per-player options (sparse — default rate omitted)
CREATE TABLE custom_attunement_player_config (
  guid       INT UNSIGNED NOT NULL,
  option_key VARCHAR(64)  NOT NULL,   -- 'xp_rate', 'boosted'
  value      FLOAT        NOT NULL DEFAULT 0,
  updated_at DATETIME     DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (guid, option_key)
);

-- Hardcore tables (loot piles, graves, per-player challenge state, death log)
custom_hardcore_loot_gameobjects
custom_hardcore_loot_tables
custom_hardcore_grave_gameobjects
custom_hardcore_player_config       -- no xp_rate column; Attunement owns it
custom_hardcore_player_deathlog
```

## GM commands

All under the unified `.attunement <subcommand>` prefix (previously `.hardcore <subcommand>`):

| Command                                  | Description                                       |
|------------------------------------------|---------------------------------------------------|
| `.attunement reset`                      | Wipe all loot piles + graves                      |
| `.attunement resetloot`                  | Wipe all loot piles                               |
| `.attunement resetgraves`                | Wipe all graves                                   |
| `.attunement spawnloot`                  | Spawn loot at your position (debug)               |
| `.attunement spawngrave`                 | Spawn grave at your position (debug)              |
| `.attunement leveldown`                  | Apply LevelDown to self                           |
| `.attunement revive true\|false`         | Toggle revive ability for selected player         |
| `.attunement droploot true\|false`       | Toggle drop-loot on death for selected player     |
| `.attunement losexp true\|false`         | Toggle lose-xp on death for selected player       |
| `.attunement pvp true\|false`            | Toggle PvP for selected player                    |
| `.attunement selffound true\|false`      | Toggle self-found for selected player             |
| `.attunement deathlog [filter] [N]`      | Show recent hardcore deaths (filter: account/player) |

## How it works

- **Attunement**: `OnPreGiveXP` multiplies all XP gain by the player's saved rate (default `1.0` if no row exists). `OnRegenerate` whispers a readout when a player inspects another with a non-default rate.
- **Hardcore**: `OnDeath` triggers loot drops + grave creation (gated by player-level config). `OnPreResurrect` blocks resurrection in single-life mode. `OnGetReactionTo` flips player-vs-player reactions to friendly when PvP is disabled. `OnPreInviteMember`, `OnPreHandleInitializeTrade`, and `OnCanCheckMailBox` enforce self-found restrictions.
- **Single NPC**: `OnPreGossipHello` for NPC 190014 builds one combined menu — XP-rate options first, then any hardcore challenge options that are enabled in `attunement.conf` and applicable to the player. The `Hardcore.CustomXPRates` gossip path from the legacy module is dropped: attunement handles XP rates exclusively.

## Migration from the standalone hardcore module

If you previously ran the standalone hardcore module:

1. Remove `BUILD_MODULE_HARDCORE` from your CMake invocation (the option no longer exists).
2. Delete `hardcore.conf` from your mangosd directory and merge its values into the `Hardcore.*` section of `attunement.conf`.
3. Apply this module's `sql/install/world/world.sql` — it cleans up the old NPC entry (190011) and creates 190014.
4. The `custom_hardcore_player_config.xp_rate` column is no longer used; the new schema drops it. If you wish to preserve existing XP-rate preferences, migrate them into `custom_attunement_player_config` keyed by `option_key='xp_rate'` (note: scale is `value` is the multiplier directly, not multiplied by 100).
5. GM commands moved from `.hardcore <subcmd>` to `.attunement <subcmd>`. The `.hardcore xp` command is gone — use the gossip menu or set values directly in `custom_attunement_player_config`.

## Scope

Intentionally narrow: this module owns **XP rate** (per-player) plus the **hardcore challenge system** (death penalties, loot drops, self-found). Other rates (reputation, money drop, loot drop, skill-up) remain global server settings.
