# cmangos-vip

VIP system for cmangos: an opt-in GM-grantable status that gives players a single "master spell" which, when cast, fires a cascade of bundled raid-buffs as triggered spells (no GCD, no mana, no cast time, original durations retained).

## What it does

* Adds two chat commands gated behind `SEC_GAMEMASTER`:
  * `.vip grant <player>` — flag a player as VIP and teach them the master spell.
  * `.vip revoke <player>` — remove the flag and unlearn the master spell.
* Hooks `OnCast` for the master spell. When a VIP casts it, the module iterates `Vip.BundledSpellIds` and triggers each one on the caster.
* Persists the VIP flag on the player so the master spell survives logout/login.

The intent is **convenience, not power-creep** — every spell in the default bundle is a buff the player could get from another player or world buff. VIPs just don't have to chase down a dozen casters.

## Build

```sh
cmake -DBUILD_MODULE_VIP=ON ..
```

## Configure

Copy `vip.conf.dist` to `vip.conf` next to your `mangosd.conf`.

| Key | Default | Notes |
|---|---|---|
| `Vip.Enable` | `0` | Master switch |
| `Vip.MasterSpellId` | `18282` | Vanilla "Dummy Spell" — already in `spell_template`, in the 1.12 client's `Spell.dbc`, and unreferenced by item/trainer/script tables. Safe to override via `OnCast` |
| `Vip.BundledSpellIds` | `""` (use built-in defaults) | Comma-separated list. Empty/unset uses the defaults below |

### Default bundle

When `Vip.BundledSpellIds` is empty, the module uses these spell IDs:

| ID | Spell | Source in vanilla |
|---|---|---|
| 22888 | Rallying Cry of the Dragonslayer | Onyxia / Nefarian head turn-in |
| 24425 | Spirit of Zandalar | ZG faction turn-in |
| 23735 | Mol'dar's Moxie | Dire Maul tribute |
| 23736 | Fengus' Ferocity | Dire Maul tribute |
| 23737 | Slip'kik's Savvy | Dire Maul tribute |
| 15366 | Songflower Serenade | Felwood songflower pickup |
| 16609 | Warchief's Blessing | Thrall channel |
| 17626 | Greater Arcane Elixir | crafted |
| 17627 | Distilled Wisdom (flask) | crafted |
| 17628 | Supreme Power (flask) | crafted |

Override `Vip.BundledSpellIds` to trim or extend this list. Whitespace is ignored, e.g. `Vip.BundledSpellIds = "22888, 24425, 16609"`.

## Why this design

* **Triggered, not learned-and-cast**: bundled spells are cast via `TRIGGERED_FULL_MASK` so they bypass the GCD, mana cost, and cast time. The player feels one cast; ten auras land at once. The buffs retain their normal **durations**, so it's not a permanent loot pile — VIPs still re-cast every ~2 hours.
* **Master spell choice (18282 vs custom)**: vanilla 1.12 clients can only cast spells whose ID exists in their `Spell.dbc`. A custom row in `spell_template` would work server-side but the client would silently refuse to send the cast. 18282 "Dummy Spell" is already in the client DBC, already a `SPELL_EFFECT_DUMMY` server-side, and has no item/trainer/script references — overriding it via `OnCast` is safe.
* **GM-grant, not purchase**: keeps the surface small and avoids designing an in-world currency. If you want to gate VIP behind activity or donations, wire that into your community ops and use the same `.vip grant` command.

## Scope and non-goals

* No tier system — VIP is binary. If you want gold-vs-silver-vs-platinum tiers, fork the master-spell logic and dispatch on a per-player tier field.
* No combat power-ups in the default bundle — the buffs are world-buff equivalents available to non-VIPs via gameplay. Adding e.g. a +50 % damage spell here would turn the module into a pay-to-win lever.
* No auto-revoke on player misbehaviour. The `Vip.Enable = 0` master switch turns the feature off; per-player revocation is `.vip revoke <name>`.

## In-game / DB surface

* No new NPC entries.
* One persisted bit per player (location: see source — kept small to avoid a per-character table).
* Two chat commands: `.vip grant <player>`, `.vip revoke <player>` (both `SEC_GAMEMASTER`).

## Provenance

Originated in this fork — see [`NOTES.md`](./NOTES.md).
