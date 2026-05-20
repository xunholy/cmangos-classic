# SQL migrations

Cmangos has no built-in DB migration framework. To keep server-specific data fixes versioned with the code that depends on them (and to stop carrying SQL Jobs in downstream deployment repos), this fork ships a small migration runner baked into the container image.

## TL;DR

* **Module owners** put their SQL under their module: `src/modules/<name>/sql/{install,migrations}/<dbtag>/`. If the module is removed, its SQL goes with it.
* **Server-wide fixes** go in `sql/emberstone/<dbtag>/`.
* The runner walks both, applies in deterministic order, tracks what's been applied in a `_emberstone_migrations` table per database.

## Layout

```
src/modules/<name>/sql/
    install/<dbtag>/NNNN_*.sql       # base data — applied once on first install
    migrations/<dbtag>/NNNN_*.sql    # incremental — applied once each, in order

sql/emberstone/<dbtag>/NNNN_*.sql    # server-wide fixes / overrides
```

`<dbtag>` is one of:

| Tag | Default DB | PTR sibling |
|---|---|---|
| `world` | `classicmangos` | `ptrmangos` (when `MANGOS_PTR_WORLD_DBNAME` is set) |
| `characters` | `classiccharacters` | `ptrcharacters` |
| `auth` | `classicrealmd` | (shared — no PTR sibling) |

Filenames are `NNNN_short_description.sql` where `NNNN` is a 4-digit number controlling apply order within a directory.

## Apply order per database

1. For each module (lex by module name):
   1. `install/<dbtag>/*.sql` (lex by filename)
   2. `migrations/<dbtag>/*.sql` (lex by filename)
2. `sql/emberstone/<dbtag>/*.sql` (lex by filename)

This means module install/migration SQL lands before any server overrides on the same data — server overrides win.

## Writing a migration

### Rule 1 — Idempotent

Every migration must be safe to run more than once. The tracker protects against re-application but you should also write the SQL defensively in case the tracker row gets deleted manually:

```sql
-- Good
CREATE TABLE IF NOT EXISTS custom_thing (...);
DELETE FROM creature WHERE id = 190020;
INSERT INTO creature (id, ...) VALUES (190020, ...);

-- Bad
ALTER TABLE characters ADD COLUMN foo INT;   -- fails on re-run
INSERT INTO creature_template VALUES (...);  -- duplicates on re-run
```

### Rule 2 — Fix forward; never edit an applied file

The runner records a `sha256` checksum of each file it applies. If you later edit the file, the runner will refuse to start with `checksum drift`. Add a new `NNNN_*.sql` with a higher number instead.

### Rule 3 — Module-owned vs server-owned

* If the data only makes sense when the module is enabled (e.g. seeding a module-specific NPC, adding a module-specific table), it goes in `src/modules/<name>/sql/migrations/<dbtag>/`. Removing the module removes the migration.
* If the data is server-wide (a bug fix in vanilla data, a policy choice that spans modules), it goes in `sql/emberstone/<dbtag>/`.

### Rule 4 — Substitute env vars sparingly

Files named `*.env.sql` are passed through `envsubst` before applying. Use this only for values that genuinely must be resolved at deploy time (e.g. the realmlist external IP). Most migrations should be static SQL.

## Running

```bash
# Apply all pending migrations
run-migrate

# Show what would be applied without touching the DB
run-migrate --dry-run
# or
DRY_RUN=1 run-migrate
```

The runner exits non-zero on any failure (connection error, missing env, checksum drift, SQL error). Suitable to use as a Kubernetes Job, a Helm pre-install/pre-upgrade hook, or a one-shot manual step.

## Tracker table

Each database gets:

```sql
CREATE TABLE _emberstone_migrations (
    source_id  VARCHAR(128) NOT NULL,
    filename   VARCHAR(255) NOT NULL,
    checksum   CHAR(64)     NOT NULL,
    applied_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (source_id, filename)
);
```

`source_id` is one of:

* `module:<name>:install`
* `module:<name>:migration`
* `emberstone:<dbtag>`

To force a single migration to re-apply (e.g. after manually reverting its effect for testing): delete its row from the tracker.

```sql
DELETE FROM _emberstone_migrations
 WHERE source_id = 'emberstone:world'
   AND filename  = '0003_aiplayerbot_ring_enchant_fix.sql';
```

## How to remove a module without breaking migrations

1. Delete `src/modules/<name>/` (or exclude it from the build).
2. Rebuild the image — the runner no longer finds the module's SQL directory and skips it.
3. Tracker rows for that module remain in the DB as an audit trail. They cause no harm.
4. Tables and rows the module created stay in place. If you want to drop them, write an `emberstone/<dbtag>/NNNN_drop_<module>.sql` migration.

## Deployment-side wiring

Use one Job (or one Helm hook) per cluster — invoked on every image bump:

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: cmangos-migrate
spec:
  template:
    spec:
      restartPolicy: OnFailure
      containers:
        - name: migrate
          image: ghcr.io/xunholy/cmangos-classic:<same-tag-as-mangosd>
          command: [run-migrate]
          envFrom:
            - secretRef: { name: cmangos-database-creds }
          env:
            - { name: MANGOS_DBHOST, value: cmangos-database }
            - { name: MANGOS_DBPORT, value: "3306" }
            - { name: MANGOS_PTR_WORLD_DBNAME,      value: ptrmangos }
            - { name: MANGOS_PTR_CHARACTERS_DBNAME, value: ptrcharacters }
```

Gate the worldserver Deployment on the Job completing (via `dependsOn` in a Flux Kustomization, or via Helm `--wait`) so mangosd never starts against an unmigrated DB.
