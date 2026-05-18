# Vendored module: dualspec

This directory is a vendored copy of an upstream module. It is no longer
linked to the upstream repo via git (the `.git` directory was stripped on
import). Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/flekz-games/cmangos-dualspec |
| Branch       | main |
| Commit SHA   | b211cbab0ee6767a065e0817507aaaa045550651 |
| Commit date  | 2024-11-06 |
| Imported on  | 2026-05-18 |

## Cherry-picking upstream fixes

```bash
git clone https://github.com/flekz-games/cmangos-dualspec.git /tmp/upstream-dualspec
cd /tmp/upstream-dualspec
git log b211cbab0ee6767a065e0817507aaaa045550651..HEAD
# Cherry-pick or copy individual changes into src/modules/dualspec/
```
