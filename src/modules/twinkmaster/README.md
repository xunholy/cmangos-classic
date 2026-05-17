# CMaNGOS Twinkmaster Module

A CMaNGOS module that adds a **Twink Master** NPC for level 19 WSG twink characters. The NPC provides one-stop setup: level locking, gear, enchants, buffs, talent resets, and consumables — all for free.

## Features

### NPC Gossip Menu
- **Set level and lock XP** — sets player to target level (default 19), locks XP, and grants:
  - Professions (First Aid, Fishing, Cooking, Engineering) at Journeyman
  - All class weapon skills maxed for level
  - All class spells available at target level (validated by SpellFamilyName)
  - Riding Turtle mount (no level requirement)
  - PvP rank 14 (Grand Marshal / High Warlord) for honor gear eligibility
  - Timbermaw Hold Friendly reputation (Furbolg Medicine Pouch)
  - WSG faction Revered reputation (Silverwing Sentinels / Warsong Outriders)
- **Lock / Unlock XP** — toggle XP gain independently
- **Browse your wares** — category vendor with class/race filtering:
  - BiS Gear (weapons, armor, rings, trinkets, shields, off-hands)
  - Honor Gear (WSG reputation items, PvP insignias, battle standards, tabards)
  - Insane (raid/endgame items with no level requirement)
  - Consumables (potions, elixirs, arcanums, engineering gadgets, food, ammo)
- **Reset my talents** — free talent reset
- **Buff me up!** — applies world buffs + class buffs and repairs gear:
  - Rallying Cry, Spirit of Zandalar, Songflower, DM Tribute (all players)
  - Warchief's Blessing (Horde only)
  - Mark of the Wild, Fortitude, Shadow Protection, Thorns, BoK, BoM (all classes)
  - Arcane Intellect, Blessing of Wisdom (mana classes only — skipped for Warriors/Rogues)
- **Enchant my gear** — per-slot enchant menu:
  - Chest, Cloak, Bracers, Gloves, Boots, Main Hand, Off Hand
  - Off Hand supports both weapon enchants (for dual wielders) and shield enchants
  - 2H-only enchants validated against equipped weapon type

### NPCs
| Entry | Faction | Spawn Location |
|-------|---------|----------------|
| 190012 | Alliance (Stormwind) | Stormwind Trade District |
| 190013 | Horde (Orgrimmar) | Orgrimmar near WSG entrance |

## Installation

### Prerequisites
- [CMaNGOS Classic](https://github.com/cmangos/mangos-classic) with module support
- [cmangos-modules](https://github.com/davidonete/cmangos-modules) framework

### Build
Clone into your CMaNGOS modules directory:
```bash
git clone https://github.com/xunholy/cmangos-twinkmaster.git src/modules/twinkmaster
```

Enable in CMake:
```bash
cmake .. -DBUILD_MODULES=ON -DBUILD_MODULE_TWINKMASTER=ON
```

### Database
Apply the SQL files to your databases:
```bash
mysql -u mangos -p classiccharacters < sql/install/characters/characters.sql
mysql -u mangos -p classicmangos < sql/install/world/world.sql
```

### Configuration
Copy `twinkmaster.conf.dist` to your server's config directory and enable:
```ini
Twinkmaster.Enable = 1
Twinkmaster.TargetLevel = 19
```

## Configuration Options

| Setting | Default | Description |
|---------|---------|-------------|
| `Twinkmaster.Enable` | `0` | Enable/disable the module |
| `Twinkmaster.TargetLevel` | `19` | Target level for the "Set level and lock" option |

## Database Tables

### `custom_twinkmaster_player_config` (characters DB)
Tracks per-player XP lock state.

| Column | Type | Description |
|--------|------|-------------|
| `guid` | int unsigned | Character GUID (PK) |
| `xp_locked` | boolean | Whether XP gain is blocked |
| `level_set` | boolean | Whether level was set by Twink Master |

### `custom_twinkmaster_vendor_categories` (world DB)
Maps items to vendor categories, read by the C++ module at startup.

| Column | Type | Description |
|--------|------|-------------|
| `item` | int unsigned | Item entry ID (PK) |
| `categories` | tinyint | 1=BiS, 2=Consumables, 3=Honor, 4=Insane |

## Vendor System

The module uses custom `SMSG_LIST_INVENTORY` packets to display filtered vendor lists per category. Items are filtered by the player's class and race at runtime. Prices are set to zero in the custom packet — `item_template` prices are **not** modified, so items retain their normal price at other vendors on the server.

## License

GPL-2.0 — same as CMaNGOS.
