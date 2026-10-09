#!/usr/bin/env bash
# Turns the proprietary Clickteam Android extension SDK header into the single-line base64 string
# the CI workflow expects as the repository secret CLICKTEAM_ANDROID_SDK_B64.
#
#   tools/make_sdk_b64.sh --sdk-dir /path/to/extracted-sdk [--out clickteam-android-sdk.b64] [--print]
#   tools/make_sdk_b64.sh --zip /path/to/existing.zip      [--out clickteam-android-sdk.b64]
#
# Availability note: the current Gradle archive checked for this project lacks RuntimeNative.h;
# no complete current public native C++ SDK has been verified. Use this helper ONLY after obtaining
# the authorized, compatible C++/NDK SDK/header from Clickteam. It cannot turn a Gradle-only archive
# into a native C++ SDK. See docs/BUILD.md, section 2. The header is searched for recursively.
#
# The result goes into the repository secret, not into the repository:
#   gh secret set CLICKTEAM_ANDROID_SDK_B64 --repo <owner>/<repo> < clickteam-android-sdk.b64
# or web UI: Settings -> Secrets and variables -> Actions -> New repository secret.
#
# The header is proprietary and must never be committed, published or shipped inside INI++.zip.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

SDK_DIR="${IPPP_ANDROID_SDK_DIR:-}"
ZIP_IN=""
OUT="clickteam-android-sdk.b64"
PRINT=0

while [ $# -gt 0 ]; do
	case "$1" in
		--sdk-dir) SDK_DIR="${2:?--sdk-dir needs a directory}"; shift 2 ;;
		--sdk-dir=*) SDK_DIR="${1#*=}"; shift ;;
		--zip) ZIP_IN="${2:?--zip needs a file}"; shift 2 ;;
		--zip=*) ZIP_IN="${1#*=}"; shift ;;
		--out) OUT="${2:?--out needs a file}"; shift 2 ;;
		--out=*) OUT="${1#*=}"; shift ;;
		--print) PRINT=1; shift ;;
		-h|--help) sed -n '2,21p' "$0"; exit 0 ;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

if [ -z "$SDK_DIR" ] && [ -z "$ZIP_IN" ]; then
	echo "usage: tools/make_sdk_b64.sh --sdk-dir /path/to/extracted-sdk [--out FILE] [--print]" >&2
	echo "       tools/make_sdk_b64.sh --zip /path/to/existing.zip      [--out FILE] [--print]" >&2
	echo >&2
	echo "Point --sdk-dir at the extracted Clickteam Android extension SDK (see docs/BUILD.md," >&2
	echo "section 2), or --zip at a zip that already contains RuntimeNative.h." >&2
	exit 2
fi

# ---------------------------------------------------------------------------------------------
# The stand-ins must never end up in a secret that CI then builds a distributed extension from
# ---------------------------------------------------------------------------------------------
check_header() { # <path to RuntimeNative.h>
	local header="$1"
	local what="the header"
	if grep -q "IPPP_MOCK_RUNTIME_NATIVE_H" "$header" 2>/dev/null; then
		what="android/tests/mock-sdk/RuntimeNative.h, the host-test stand-in"
	fi
	if grep -q "RuntimeNative_Shim_HeaderPlusPlus" "$header" 2>/dev/null; then
		what="android/sdk-shim/RuntimeNative.h, the documented reconstruction"
	fi
	if [ "$what" != "the header" ]; then
		echo "error: this is $what - not the Clickteam SDK header." >&2
		echo "       Builds must use the real SDK: https://www.clickteam.com/extensions-sdks" >&2
		exit 1
	fi
	if ! grep -q "RuntimeFunctions" "$header" 2>/dev/null; then
		echo "error: $header does not mention RuntimeFunctions, so it is probably not the header." >&2
		exit 1
	fi
}

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if [ -n "$ZIP_IN" ]; then
	# ---------------------------------------------------------------- from an existing zip
	[ -f "$ZIP_IN" ] || { echo "error: $ZIP_IN does not exist." >&2; exit 1; }
	command -v unzip >/dev/null 2>&1 || { echo "error: unzip is needed for --zip." >&2; exit 1; }
	unzip -q -o "$ZIP_IN" -d "$WORK/unpacked"
	HEADER="$(find "$WORK/unpacked" -name 'RuntimeNative.h' -print -quit 2>/dev/null || true)"
	[ -n "$HEADER" ] || { echo "error: $ZIP_IN contains no RuntimeNative.h." >&2; exit 1; }
	check_header "$HEADER"
	REL="${HEADER#"$WORK/unpacked"/}"
	cp -f "$ZIP_IN" "$WORK/sdk.zip"
	echo "Found the header in $ZIP_IN: $REL ($(du -h "$ZIP_IN" | cut -f1))"
else
	# ---------------------------------------------------------------- from an extracted SDK
	[ -d "$SDK_DIR" ] || { echo "error: $SDK_DIR is not a directory." >&2; exit 1; }
	HEADER=""
	for candidate in "$SDK_DIR/RuntimeNative.h" "$SDK_DIR"/*/RuntimeNative.h \
	                 "$SDK_DIR/jni/RuntimeNative.h" "$SDK_DIR/android/jni/RuntimeNative.h" \
	                 "$SDK_DIR"/*/jni/RuntimeNative.h "$SDK_DIR/inc/RuntimeNative.h" \
	                 "$SDK_DIR/include/RuntimeNative.h" "$SDK_DIR"/*/*/RuntimeNative.h; do
		if [ -f "$candidate" ]; then
			HEADER="$candidate"
			break
		fi
	done
	if [ -z "$HEADER" ]; then
		HEADER="$(find "$SDK_DIR" -maxdepth 6 -name 'RuntimeNative.h' -print -quit 2>/dev/null || true)"
	fi
	[ -n "$HEADER" ] || {
		echo "error: no RuntimeNative.h under $SDK_DIR." >&2
		echo "       Is that really the extracted Clickteam Android extension SDK?  See docs/BUILD.md, section 2." >&2
		exit 1
	}
	check_header "$HEADER"
	REL="${HEADER#"$SDK_DIR"/}"
	case "$REL" in
		*/*) REL_DIR="${REL%/*}" ;;
		*)   REL_DIR="" ;;
	esac

	# Zip the header together with the other headers in its directory (it may #include them), but
	# nothing else: GitHub rejects repository secrets larger than 48 kB.
	STAGE="$WORK/sdk"
	mkdir -p "$STAGE${REL_DIR:+/$REL_DIR}"
	cp -f "$HEADER" "$STAGE/$REL"
	for other in "$(dirname "$HEADER")"/*.h "$(dirname "$HEADER")"/*.hpp; do
		[ -f "$other" ] || continue
		cp -f "$other" "$STAGE${REL_DIR:+/$REL_DIR}/$(basename "$other")"
	done
	HEADER_COUNT="$(find "$STAGE" -type f | wc -l | tr -d ' ')"
	echo "Found the header: $SDK_DIR/$REL (packing $HEADER_COUNT header file(s) from its directory)"

	if command -v zip >/dev/null 2>&1; then
		(cd "$STAGE" && zip -r -9 -q "$WORK/sdk.zip" .)
	else
		echo "note: zip is not installed, packing with python3 instead" >&2
		python3 - "$STAGE" "$WORK/sdk.zip" <<-'PY'
			import os, sys, zipfile
			stage, out = sys.argv[1], sys.argv[2]
			with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
			    for root, _, files in os.walk(stage):
			        for name in sorted(files):
			            path = os.path.join(root, name)
			            z.write(path, os.path.relpath(path, stage))
		PY
	fi
fi

# ---------------------------------------------------------------------------------------------
# base64, single line (GitHub's secret field must not contain line breaks the decoder chokes on)
# ---------------------------------------------------------------------------------------------
if command -v base64 >/dev/null 2>&1 && base64 --help 2>&1 | grep -q -- "--wrap\|-w"; then
	base64 -w0 "$WORK/sdk.zip" > "$OUT"            # GNU
elif command -v base64 >/dev/null 2>&1; then
	base64 -b 0 "$WORK/sdk.zip" > "$OUT" 2>/dev/null || base64 "$WORK/sdk.zip" | tr -d '\n' > "$OUT"
else
	python3 -c 'import base64,sys;open(sys.argv[2],"w").write(base64.b64encode(open(sys.argv[1],"rb").read()).decode())' \
		"$WORK/sdk.zip" "$OUT"
fi

ZIP_BYTES="$(wc -c < "$WORK/sdk.zip")"
B64_BYTES="$(wc -c < "$OUT")"
NEWLINES="$(tr -cd '\n' < "$OUT" | wc -c)"

echo "zip:     $ZIP_BYTES bytes ($(du -h "$WORK/sdk.zip" | cut -f1))"
echo "base64:  $B64_BYTES characters -> $OUT"
[ "$NEWLINES" -eq 0 ] || echo "warning: $NEWLINES line break(s) in $OUT; the workflow decodes them fine, but single-line is preferred" >&2

if [ "$B64_BYTES" -gt 48000 ]; then
	echo
	echo "warning: $B64_BYTES characters is close to or over GitHub's 48 kB repository secret limit." >&2
	echo "         This script packs only the header's directory; if that is still too big, pack just" >&2
	echo "         RuntimeNative.h itself (zip -j / Compress-Archive -Path on the single file) and" >&2
	echo "         check whether the build needs any header it includes." >&2
fi

echo
echo "Put it into the repository secret (never into the repository):"
echo "  gh secret set CLICKTEAM_ANDROID_SDK_B64 --repo <owner>/<repo> < $OUT"
echo "or web UI: Settings -> Secrets and variables -> Actions -> New repository secret"
[ "$PRINT" = 1 ] && cat "$OUT" && echo
