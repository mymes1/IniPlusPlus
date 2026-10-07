#!/usr/bin/env bash
# Regenerates .github/workflows/build.yml from ci/workflows/build.yml.
#
# The workflow lives at ci/workflows/build.yml because GitHub only accepts workflow files pushed by
# a token with the "workflows" permission, which some automation does not have (see the header of
# ci/workflows/build.yml).  This script keeps the ready-to-commit copy in .github/workflows in sync:
# it strips the explanatory header and points the messages back at the installed path.
#
# Usage:  tools/sync_workflow.sh          # write .github/workflows/build.yml
#         tools/sync_workflow.sh --check  # fail if it is out of date
set -euo pipefail
cd "$(dirname "$0")/.."

SRC=ci/workflows/build.yml
DST=.github/workflows/build.yml

generated="$(python3 - "$SRC" <<'PY'
import sys, pathlib
text = pathlib.Path(sys.argv[1]).read_text()
# drop the leading comment block that documents the ci/ location
lines = text.splitlines(keepends=True)
body_start = next(i for i, line in enumerate(lines) if not line.startswith('#'))
body = ''.join(lines[body_start:])
# once installed, the file is the one the messages should point at
body = body.replace('ci/workflows/build.yml', '.github/workflows/build.yml')
sys.stdout.write(body.rstrip('\n') + '\n')
PY
)"

if [ "${1:-}" = --check ]; then
	if [ "$(cat "$DST" 2>/dev/null || true)" != "$generated" ]; then
		echo "$DST is out of date - run tools/sync_workflow.sh" >&2
		exit 1
	fi
	echo "$DST is up to date"
	exit 0
fi

mkdir -p "$(dirname "$DST")"
printf '%s' "$generated" > "$DST"
echo "wrote $DST"
