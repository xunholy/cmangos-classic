# Emberstone server-wide SQL migrations

Server-wide migrations live here. Module-owned migrations live next to the module under `src/modules/<name>/sql/migrations/<dbtag>/`.

See [`docs/MIGRATIONS.md`](../../docs/MIGRATIONS.md) for the full pattern.

## Layout

```
world/         → classicmangos + ptrmangos (when MANGOS_PTR_WORLD_DBNAME is set)
characters/    → classiccharacters + ptrcharacters
auth/          → classicrealmd (PTR shares the auth DB by design)
```

## Naming

`NNNN_short_description.sql`, lex-ordered. Pick `NNNN` higher than the previous file in the same directory.

## Idempotency checklist

Before merging, verify the SQL is safe to run again with the tracker cleared:

* `CREATE TABLE` → `CREATE TABLE IF NOT EXISTS`
* `INSERT` of new rows → `INSERT IGNORE` or `DELETE`+`INSERT` keyed on a stable id
* `ALTER TABLE` → use a guard or a one-shot — these are the trickiest; prefer additive changes
* `UPDATE` → always with `WHERE`

## File extensions

* `.sql` — applied as-is
* `.env.sql` — passed through `envsubst` first. Reserve for runtime-resolved values like `${EXTERNAL_IP}`.
