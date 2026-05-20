#!/usr/bin/env bash
#
# Emberstone migration runner.
#
# Applies SQL migrations from /opt/mangos/sql/ into the cmangos databases.
# Tracks applied migrations in `_emberstone_migrations` per database;
# re-running is a fast no-op once everything is applied.
#
# Source layout in the image (rooted at $SQL_ROOT, default /opt/mangos/sql):
#
#   modules/<name>/sql/install/<dbtag>/NNNN_*.sql      ← module base data
#   modules/<name>/sql/migrations/<dbtag>/NNNN_*.sql   ← module incremental
#   emberstone/<dbtag>/NNNN_*.sql                      ← server-wide overrides
#
# <dbtag> ∈ { world, characters, auth }. Each tag is applied to one or more
# real databases — see TARGETS below.
#
# Apply order per database:
#   1. For each module (lex by module name):
#        a. install/<dbtag>/*.sql       (lex by filename)
#        b. migrations/<dbtag>/*.sql    (lex by filename)
#   2. emberstone/<dbtag>/*.sql          (lex by filename)
#
# Loose-coupling guarantees:
#   - Removing a module directory removes its migrations automatically.
#   - The runner skips any module that has no SQL for the current dbtag.
#   - Tracker rows from a deleted module stay (harmless audit trail) but
#     no longer attempt to re-apply.
#
# Idempotency:
#   - Each migration is applied at most once per database (tracked by
#     source_id + filename in _emberstone_migrations).
#   - Author migrations defensively: CREATE TABLE IF NOT EXISTS,
#     INSERT IGNORE / REPLACE / DELETE-then-INSERT, UPDATE with WHERE.
#   - Checksum drift (file edited after apply) is fatal — fix forward by
#     adding a new NNNN_*.sql, not by editing an applied one.
#
# Env-var substitution:
#   - Files named *.env.sql are passed through `envsubst` before applying.
#   - Use sparingly — only for values that must be runtime-resolved
#     (e.g. ${EXTERNAL_IP} for realmlist.address).
#
# Required env: MANGOS_DBHOST, MANGOS_DBPORT, MANGOS_DBUSER, MANGOS_DBPASS
# Optional env:
#   MANGOS_WORLD_DBNAME      (default: classicmangos)
#   MANGOS_CHARACTERS_DBNAME (default: classiccharacters)
#   MANGOS_REALMD_DBNAME     (default: classicrealmd)
#   MANGOS_PTR_WORLD_DBNAME       — if set, world migrations also apply here
#   MANGOS_PTR_CHARACTERS_DBNAME  — if set, characters migrations also apply here
#   SQL_ROOT                      — override default /opt/mangos/sql
#   DRY_RUN=1                     — print plan, do not execute
#

set -euo pipefail

SQL_ROOT="${SQL_ROOT:-/opt/mangos/sql}"
DRY_RUN="${DRY_RUN:-0}"
[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=1

for v in MANGOS_DBHOST MANGOS_DBPORT MANGOS_DBUSER MANGOS_DBPASS; do
    if [[ -z "${!v:-}" ]]; then
        echo "FATAL: $v is required" >&2
        exit 2
    fi
done

MARIADB_OPTS=(-h "$MANGOS_DBHOST" -P "$MANGOS_DBPORT" -u "$MANGOS_DBUSER" -p"$MANGOS_DBPASS")
WORLD_DB="${MANGOS_WORLD_DBNAME:-classicmangos}"
CHARS_DB="${MANGOS_CHARACTERS_DBNAME:-classiccharacters}"
AUTH_DB="${MANGOS_REALMD_DBNAME:-classicrealmd}"

# Tag → space-separated list of real DB names.
declare -A TARGETS
TARGETS[world]="$WORLD_DB"
[[ -n "${MANGOS_PTR_WORLD_DBNAME:-}" ]] && TARGETS[world]+=" ${MANGOS_PTR_WORLD_DBNAME}"
TARGETS[characters]="$CHARS_DB"
[[ -n "${MANGOS_PTR_CHARACTERS_DBNAME:-}" ]] && TARGETS[characters]+=" ${MANGOS_PTR_CHARACTERS_DBNAME}"
TARGETS[auth]="$AUTH_DB"   # PTR shares the auth DB with live by design

mariadb_run() {
    local db="$1"; shift
    mariadb "${MARIADB_OPTS[@]}" "$db" "$@"
}

ensure_tracker() {
    local db="$1"
    mariadb_run "$db" -e "
        CREATE TABLE IF NOT EXISTS _emberstone_migrations (
            source_id  VARCHAR(128) NOT NULL,
            filename   VARCHAR(255) NOT NULL,
            checksum   CHAR(64)     NOT NULL,
            applied_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (source_id, filename)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    " > /dev/null
}

# Returns 0 if applied, 1 if not. Echoes recorded checksum (if any) to stdout.
applied_checksum() {
    local db="$1" source_id="$2" name="$3"
    mariadb -BN "${MARIADB_OPTS[@]}" "$db" -e \
        "SELECT checksum FROM _emberstone_migrations
         WHERE source_id='${source_id}' AND filename='${name}'"
}

apply_file() {
    local db="$1" source_id="$2" file="$3"
    local name; name=$(basename "$file")
    local rendered sum existing
    if [[ "$name" == *.env.sql ]]; then
        rendered=$(envsubst < "$file")
    else
        rendered=$(cat "$file")
    fi
    sum=$(printf '%s' "$rendered" | sha256sum | cut -d' ' -f1)
    existing=$(applied_checksum "$db" "$source_id" "$name")

    if [[ -n "$existing" ]]; then
        if [[ "$existing" != "$sum" ]]; then
            echo "FATAL: $db / $source_id / $name was modified after apply (checksum drift)" >&2
            echo "  recorded: $existing" >&2
            echo "  current : $sum" >&2
            echo "  fix forward: add a new migration; do not edit applied files" >&2
            exit 3
        fi
        return 0
    fi

    echo "  [$db] apply $source_id / $name"
    if [[ "$DRY_RUN" == "1" ]]; then
        echo "    (dry-run, not executed)"
        return 0
    fi
    printf '%s' "$rendered" | mariadb "${MARIADB_OPTS[@]}" "$db"
    mariadb_run "$db" -e \
        "INSERT INTO _emberstone_migrations (source_id, filename, checksum)
         VALUES ('${source_id}', '${name}', '${sum}');" > /dev/null
}

# Apply a directory of *.sql files in sorted order, if it exists.
apply_dir() {
    local db="$1" source_id="$2" dir="$3"
    [[ -d "$dir" ]] || return 0
    local f
    while IFS= read -r -d '' f; do
        apply_file "$db" "$source_id" "$f"
    done < <(find "$dir" -maxdepth 1 -type f -name '*.sql' -print0 | sort -z)
}

echo "=== Emberstone migration runner ==="
echo "SQL_ROOT=$SQL_ROOT  DRY_RUN=$DRY_RUN"

for tag in world characters auth; do
    dbs="${TARGETS[$tag]:-}"
    [[ -z "$dbs" ]] && continue
    for db in $dbs; do
        echo
        echo "[tag=$tag db=$db]"
        ensure_tracker "$db"

        # Modules — discover by walking modules/*/sql/{install,migrations}/<tag>/
        if [[ -d "$SQL_ROOT/modules" ]]; then
            for mod_dir in "$SQL_ROOT/modules"/*/; do
                [[ -d "$mod_dir" ]] || continue
                local_mod_name=$(basename "$mod_dir")
                apply_dir "$db" "module:${local_mod_name}:install"   "${mod_dir}sql/install/$tag"
                apply_dir "$db" "module:${local_mod_name}:migration" "${mod_dir}sql/migrations/$tag"
            done
        fi

        # Server-wide
        apply_dir "$db" "emberstone:$tag" "$SQL_ROOT/emberstone/$tag"
    done
done

echo
echo "=== migrations complete ==="
