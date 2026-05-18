# Vendored module: achievements

This directory is a vendored copy of an upstream module. It is no longer
linked to the upstream repo via git (the `.git` directory was stripped on
import). Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/flekz-games/cmangos-achievements |
| Branch       | main |
| Commit SHA   | 70efae53ed1766b6d29153cf3706c57b7d068fef |
| Commit date  | 2026-02-08 |
| Imported on  | 2026-05-18 |

## Cherry-picking upstream fixes

```bash
git clone https://github.com/flekz-games/cmangos-achievements.git /tmp/upstream-achievements
cd /tmp/upstream-achievements
git log 70efae53ed1766b6d29153cf3706c57b7d068fef..HEAD
# Cherry-pick or copy individual changes into src/modules/achievements/
```
