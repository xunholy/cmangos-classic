# Vendored module: twinkmaster

This directory is a vendored copy of the upstream `xunholy/cmangos-twinkmaster` repo. The `.git` directory was stripped on import. Treat the code as part of this fork — modify freely.

## Upstream provenance

| Field        | Value |
|--------------|-------|
| Upstream URL | https://github.com/xunholy/cmangos-twinkmaster |
| Branch       | main |
| Commit SHA   | 256efac9c988cd2e00f5153bbfd1f843bdf85989 |
| Commit date  | 2026-04-30 |
| Imported on  | 2026-05-21 |

## Cherry-picking upstream fixes

```bash
git clone https://github.com/xunholy/cmangos-twinkmaster.git /tmp/upstream-twinkmaster
cd /tmp/upstream-twinkmaster
git log 256efac9c988cd2e00f5153bbfd1f843bdf85989..HEAD
# Cherry-pick or copy individual changes into src/modules/twinkmaster/
```

After importing changes, bump the SHA + date in the table above so the next contributor sees the new baseline.

## Removing the module

Drop the module by:

1. Removing the `src/modules/twinkmaster/` directory.
2. Removing the `TWINKMASTER=...` line from `src/modules/modules/modules.conf`.
3. Building without `-DBUILD_MODULE_TWINKMASTER=ON`.
4. Optional DB cleanup:
   * `DROP TABLE custom_twinkmaster_player_config;` (characters DB)
   * `DROP TABLE custom_twinkmaster_vendor_categories;` (world DB)
   * `DELETE FROM creature_template WHERE entry IN (190012, 190013);` (world DB)
