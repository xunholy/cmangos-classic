# Vendored module: trainingdummies

This directory is a vendored copy of an upstream module. It is no longer
linked to the upstream repo via git (the `.git` directory was stripped on
import). Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/flekz-games/cmangos-trainingdummies |
| Branch       | main |
| Commit SHA   | d00323521680b76eebbe03d9c498c84ec85e7bbf |
| Commit date  | 2024-05-13 |
| Imported on  | 2026-05-18 |

## Cherry-picking upstream fixes

```bash
git clone https://github.com/flekz-games/cmangos-trainingdummies.git /tmp/upstream-trainingdummies
cd /tmp/upstream-trainingdummies
git log d00323521680b76eebbe03d9c498c84ec85e7bbf..HEAD
# Cherry-pick or copy individual changes into src/modules/trainingdummies/
```
