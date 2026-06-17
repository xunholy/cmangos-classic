# Vendored module framework: modules

This directory is a vendored copy of the `flekz-games/cmangos-modules`
framework — the hook system (`ModuleMgr`, `Module`, `ModuleConfig`) that the
other modules in `src/modules/` attach to. The `.git` directory was stripped
on import. Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/flekz-games/cmangos-modules |
| Branch       | main |

## The `patches/` directory is reference-only — do NOT maintain it in-fork

`patches/classic.patch` and `patches/tbc.patch` are **onboarding artifacts**
from the upstream framework. They exist so someone applying this module system
to a *pristine* cmangos core can `git apply` the hook insertions by hand (see
`README.md` → "Use a patch"). They are:

* **Not applied at build time.** Nothing in CMake or any script runs them. Our
  core already has every hook baked directly into the source (e.g.
  `sModuleMgr.OnUpdateSkill(...)` in `src/game/Entities/Player.cpp`).
* **Owned by upstream**, not us. They target vanilla cmangos at the line
  numbers / function shapes that existed when upstream generated them. Our core
  is 100+ commits diverged, so the hunks would not apply here regardless.

Consequently they drift out of date whenever upstream core refactors a hooked
function — for example the 2026 skill-up rework changed `bool Player::UpdateSkill`
to `void Player::UpdateSkill`, so the patches' `UpdateSkill` hunk no longer
matches current vanilla core. **This is expected and harmless.** Do not "fix"
the patch context in this fork: it provides zero build benefit and only
diverges the files from their upstream source. If you ever need fresh patches
(e.g. to re-derive hooks against a newer core), regenerate them from
`flekz-games/cmangos-modules`, don't hand-edit these.
