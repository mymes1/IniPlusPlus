#!/usr/bin/env bash
# Mirrors the canonical CI workflow (ci/workflows/build.yml) to the path GitHub runs it from,
# .github/workflows/main.yml.
#
# The two files are byte-for-byte identical: the workflow is stored in ci/ because a push that
# creates or changes a file under .github/workflows/ needs a GitHub token with the "workflows"
# permission, which some automation does not have (see the comment at the top of the workflow).
#
# Usage:  tools/sync_workflow.sh          # write .github/workflows/main.yml
#         tools/sync_workflow.sh --check  # fail if the two files differ
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

SRC=ci/workflows/build.yml
DST=.github/workflows/main.yml

[ -f "$SRC" ] || { echo "error: $SRC is missing." >&2; exit 1; }

if [ "${1:-}" = "--check" ]; then
	if [ -f "$DST" ] && cmp -s "$SRC" "$DST"; then
		echo "$DST is up to date"
		exit 0
	fi
	echo "$DST is out of date - run tools/sync_workflow.sh" >&2
	echo "(if you have no 'workflows' permission, .github/workflows/main.yml cannot be pushed by" >&2
	echo " automation: commit it by hand, or paste the contents through the GitHub web UI)" >&2
	exit 1
fi

if [ -n "${1:-}" ] && [ "$1" != "--check" ]; then
	echo "unknown argument: $1" >&2
	exit 2
fi

mkdir -p "$(dirname "$DST")"
cp -f "$SRC" "$DST"
echo "wrote $DST"
