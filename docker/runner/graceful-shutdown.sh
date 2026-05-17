#!/bin/sh
# graceful-shutdown: drive mangosd's in-game restart countdown via stdin.
#
# Intended to be wired as a Kubernetes preStop hook. The container's stdin
# is bound to a named pipe at /tmp/mangosd-stdin; mangosd reads commands
# from it as if they were typed at its console.
#
# `server restart <seconds>` triggers the in-game countdown + safe save +
# clean exit. We pick 240s (4-minute in-game warning), then sleep 260s to
# leave a 20s buffer for the post-countdown save before kubelet escalates
# to SIGTERM (which it does at terminationGracePeriodSeconds-2s).
#
# k8s terminationGracePeriodSeconds should be set to ~300s to give the
# whole sequence room. SIGTERM lands at ~280s, SIGKILL at 300s if anything
# is still alive.

set -eu

COUNTDOWN_SECONDS="${COUNTDOWN_SECONDS:-240}"
DRAIN_SECONDS="${DRAIN_SECONDS:-260}"
STDIN_PIPE="${STDIN_PIPE:-/tmp/mangosd-stdin}"

echo "server restart ${COUNTDOWN_SECONDS}" > "${STDIN_PIPE}"
sleep "${DRAIN_SECONDS}"
