#!/bin/sh
# cores-pruner: keep /opt/mangos/cores from filling its PVC.
#
# Runs as a sidecar next to mangosd. Every 10 min: delete zero-byte cores
# (mangosd writes those when the disk is full — they're useless and just
# crowd the dir), then keep only the 2 newest non-zero cores by mtime and
# delete the rest.
#
# Background: May 2026 incident — a runaway crash loop filled the 40Gi
# cores PVC with multi-GB dumps; once full, mangosd started writing
# zero-byte cores, which blinded post-mortem analysis. This pruner caps
# the dir at 2 useful cores so a future crash loop can still capture
# diagnostic dumps.
#
# Runs as UID 1001 (matching mangosd) — without CAP_DAC_OVERRIDE (we
# drop all caps) root can't delete files owned by 1001 at mode 0600,
# so the pruner would otherwise be a no-op.

set -eu

CORES_DIR="${CORES_DIR:-/opt/mangos/cores}"
KEEP="${KEEP:-2}"
INTERVAL="${INTERVAL:-600}"

while true; do
    if cd "${CORES_DIR}" 2>/dev/null; then
        find . -maxdepth 1 -name 'core.*' -size 0 -delete 2>/dev/null
        # shellcheck disable=SC2012
        ls -1t core.* 2>/dev/null | tail -n "+$((KEEP + 1))" | xargs -r rm -f --
    fi
    sleep "${INTERVAL}"
done
