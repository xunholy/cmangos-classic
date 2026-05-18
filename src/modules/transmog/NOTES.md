# Vendored module: transmog

This directory is a vendored copy of an upstream module. It is no longer
linked to the upstream repo via git (the `.git` directory was stripped on
import). Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/flekz-games/cmangos-transmog |
| Branch       | main |
| Commit SHA   | 0abf98b38e80724b5a2847b029ccf72e09d98ef2 |
| Commit date  | 2024-05-20 |
| Imported on  | 2026-05-18 |

## Cherry-picking upstream fixes

```bash
git clone https://github.com/flekz-games/cmangos-transmog.git /tmp/upstream-transmog
cd /tmp/upstream-transmog
git log 0abf98b38e80724b5a2847b029ccf72e09d98ef2..HEAD
# Cherry-pick or copy individual changes into src/modules/transmog/
```
