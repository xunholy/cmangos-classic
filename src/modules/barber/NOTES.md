# Vendored module: barber

This directory is a vendored copy of an upstream module. It is no longer
linked to the upstream repo via git (the `.git` directory was stripped on
import). Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/celguar/cmangos-barber |
| Branch       | main |
| Commit SHA   | 5f214bebabfd147a150133986756b57540d5de0e |
| Commit date  | 2025-03-26 |
| Imported on  | 2026-05-18 |

## Cherry-picking upstream fixes

```bash
git clone https://github.com/celguar/cmangos-barber.git /tmp/upstream-barber
cd /tmp/upstream-barber
git log 5f214bebabfd147a150133986756b57540d5de0e..HEAD
# Cherry-pick or copy individual changes into src/modules/barber/
```
