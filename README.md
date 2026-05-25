# Emberstone

[![Build CMaNGOS Classic (Emberstone)](../../actions/workflows/build.yaml/badge.svg)](../../actions/workflows/build.yaml) [![Ubuntu](../../actions/workflows/ubuntu.yml/badge.svg)](../../actions/workflows/ubuntu.yml) [![Windows](../../actions/workflows/windows.yml/badge.svg)](../../actions/workflows/windows.yml) [![macOS](../../actions/workflows/macos.yml/badge.svg)](../../actions/workflows/macos.yml)

A fork of [CMaNGOS Classic](https://github.com/cmangos/mangos-classic) that powers the **Emberstone** vanilla 1.12.1 realm. Vanilla-faithful where it counts; modular where it helps.

This README is the entry point. For depth, jump to [Documentation](#documentation).

## What this is

* **Vanilla 1.12.1 worldserver + realmd** built from the cmangos-classic source tree.
* **Ten vendored modules** layered on top — see [Modules](#modules).
* **A published Docker image** at `ghcr.io/xunholy/cmangos-classic:<sha>`, built on every push to `main`.
* **An idempotent SQL migration runner** baked into the image so server-specific data fixes ship with the code that depends on them — see [`docs/MIGRATIONS.md`](docs/MIGRATIONS.md).
* **Server-side patches** carried on top of upstream cmangos: the bot AI / Spell::OnCast hook wiring, the WorldportAck patch, the .gobject mangos_string format hardening, and a handful of others — see the commit log on this branch for the full list.

## Connect (for players)

* **Realmlist:** `wow.owncloud.ai`
* **Client:** vanilla 1.12.1. Launch `WoW.exe` directly — the Launcher will try to update the client and fail.
* **Registration:** [emberstone.owncloud.ai](https://emberstone.owncloud.ai)

For the in-game `realmlist.wtf` and a full how-to-connect walkthrough, see the connect page on the portal.

## Quick start (for self-hosters)

```sh
# Pull the latest image
docker pull ghcr.io/xunholy/cmangos-classic:main

# Apply SQL migrations (idempotent — safe to re-run)
docker run --rm \
  -e MANGOS_DBHOST=<host> -e MANGOS_DBPORT=3306 \
  -e MANGOS_DBUSER=<user> -e MANGOS_DBPASS=<pass> \
  ghcr.io/xunholy/cmangos-classic:main run-migrate

# Run mangosd
docker run -d --name mangosd \
  -e MANGOS_DBHOST=<host> ... \
  -p 8085:8085 \
  ghcr.io/xunholy/cmangos-classic:main mangosd

# Run realmd
docker run -d --name realmd \
  -e MANGOS_DBHOST=<host> ... \
  -p 3724:3724 \
  ghcr.io/xunholy/cmangos-classic:main realmd
```

Production deployment for the Emberstone realm is in the [k8s-gitops repo](https://github.com/xunholy/k8s-gitops/tree/main/kubernetes/apps/base/game-servers/cmangos) — useful as a reference for a real Flux/Kubernetes setup.

## Modules

All vendored in `src/modules/`. Each module has its own `README.md` (what it does, config knobs, in-game surface) and `NOTES.md` (upstream provenance for cherry-picking fixes).

| Module | What it does | Default |
|---|---|---|
| [achievements](src/modules/achievements) | WoTLK-style achievement system back-ported to vanilla | off |
| [attunement](src/modules/attunement) | Per-player XP rate + hardcore challenges (single life, drop loot on death, self-found) + dual specialization (two saved talent builds + action bars) via the **Attuner of Paths** NPC | off |
| [autoscale](src/modules/autoscale) | Dynamic dungeon/raid HP scaling for under-staffed groups (never scales up) | off |
| [barber](src/modules/barber) | Barbershop NPC for cosmetic restyles | on |
| [trainingdummies](src/modules/trainingdummies) | Stationary, immortal damage dummies for DPS testing in capital cities | off |
| [transmog](src/modules/transmog) | Cosmetic gear appearance overrides (chat-command driven: `.transmog get / apply`) | off |
| [twinkmaster](src/modules/twinkmaster) | One-stop level-19 WSG twink setup NPC | off |
| [vip](src/modules/vip) | GM-grantable VIP status with a single "master spell" that cascades raid buffs | off |
| [modules](src/modules/modules) | The cmangos-modules framework itself — hooks, lifecycle, scaffold | n/a |

To enable a module, build with `-DBUILD_MODULE_<NAME>=ON` and set `<Name>.Enable = 1` in its `.conf` (in `etc/`). The cmake flag and the runtime `Enable` flag are intentionally separate — building a module in but leaving `Enable=0` is the standard pattern for shipping a module you can toggle without rebuilding the image.

## Repository tour

```
src/
  game/                # cmangos worldserver (mangosd) — patched on top of upstream
  realmd/              # realmd auth server
  modules/             # vendored modules (see Modules table above)
    modules/           # the cmangos-modules framework hosting the rest
sql/
  base/                # full schema for fresh installs (classicmangos, classicrealmd, ...)
  updates/             # cmangos-style numbered DB updates
  emberstone/          # server-wide migrations applied by run-migrate (see docs/MIGRATIONS.md)
docker/
  builder/             # multi-stage builder entrypoint
  runner/              # runtime entrypoint + sidecars (cores-pruner, graceful-shutdown, run-migrate)
etc/
  emberstone/          # production-shaped mangosd.conf / realmd.conf templates
docs/
  MIGRATIONS.md        # SQL migration runner pattern
dep/                   # vendored third-party deps (recast/detour, libmpq, etc.)
contrib/               # legacy / one-shot tools (extractor, mmap generator, ...)
```

## Documentation

* [Migrations pattern](docs/MIGRATIONS.md) — how the `run-migrate` runner works, where to put new SQL, how to remove a module cleanly.
* [Contributing](CONTRIBUTING.md) — code style, PR flow, dev environment.
* Per-module docs — every `src/modules/*/README.md` documents its module's purpose, config knobs, DB surface, and in-game commands.
* Legacy bot docs — `doc/PlayerBot/commands.txt` still applies to AiPlayerbot's chat command surface (`/invite BOTNAME`, `/t BOTNAME ...`).
* [`AUTHORS.md`](AUTHORS.md), [`COPYRIGHT.md`](COPYRIGHT.md), [`LICENSE`](LICENSE), [`ChangeLog.md`](ChangeLog.md) — upstream cmangos boilerplate, retained verbatim.

## Building from source

```sh
git clone https://github.com/xunholy/cmangos-classic.git
cd cmangos-classic
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_MODULES=ON \
  -DBUILD_MODULE_BARBER=ON \
  -DBUILD_MODULE_TRANSMOG=ON \
  -DBUILD_MODULE_TRAININGDUMMIES=ON \
  -DBUILD_MODULE_ACHIEVEMENTS=ON \
  -DBUILD_MODULE_ATTUNEMENT=ON \
  -DBUILD_MODULE_AUTOSCALE=ON \
  -DBUILD_MODULE_TWINKMASTER=ON \
  -DBUILD_MODULE_VIP=ON
make -j$(nproc)
```

For Docker — just `docker build .` against this repo's root. The Dockerfile is multi-stage; the runner image is ~75 MB.

For per-platform CMake notes (boost, ICU on macOS, MSVC settings on Windows), see the relevant `.github/workflows/*.yml` files — they're the source of truth for what works on each platform.

## Contributing

PRs welcome. Read [`CONTRIBUTING.md`](CONTRIBUTING.md) for the cmangos-inherited contribution flow (code style, commit conventions, sign-off). For fork-specific additions:

* **SQL changes:** follow [`docs/MIGRATIONS.md`](docs/MIGRATIONS.md). Module-owned data goes under `src/modules/<name>/sql/`; server-wide fixes go under `sql/emberstone/`. Migrations are idempotent and never edited after apply (fix forward).
* **Module changes:** keep config knobs documented in `<module>.conf.dist.in` AND in the module's `README.md`. Tracking happens in `<module>/NOTES.md`.
* **Pre-commit:** install `pre-commit` and run `pre-commit install`. Shellcheck, yamllint, and sqlfluff run on every commit.
* **CI:** Ubuntu, Windows, macOS, and the Emberstone image-publish workflow all need to stay green on `main`. The build matrix is intentionally broad — fork it locally to test against your platform of choice.

## Upstream and attribution

Emberstone is a fork — the heavy lifting is cmangos. Upstream relationships:

* **cmangos/mangos-classic** — base server. We rebase / cherry-pick periodically; an `Upstream sync digest` workflow publishes a rolling diff to a tracking issue.
* **flekz-games/cmangos-modules** — the modules framework that hosts our vendored modules.
* **flekz-games/cmangos-achievements**, **flekz-games/cmangos-dualspec**, **flekz-games/cmangos-trainingdummies**, **flekz-games/cmangos-transmog** — upstream sources for four of our vendored modules.
* **celguar/cmangos-barber** — upstream source for the barber module.
* **xunholy/cmangos-twinkmaster**, **xunholy/cmangos-attunement** — Emberstone-maintained forks of upstream modules; vendored here.

See each module's `NOTES.md` for the exact commit baseline and how to cherry-pick fixes.

## License and disclaimers

GPL-2.0 for the cmangos-derived code; per-module licenses noted in each module's `LICENSE` (or in the root `LICENSE` when the module omits its own). See [`COPYRIGHT.md`](COPYRIGHT.md) for full attribution.

This project exists for **education and private-server experimentation**. Public/commercial use is illegal in many jurisdictions. We provide no support for hostile use. See `WARNING` for the upstream cmangos boilerplate on this point.
