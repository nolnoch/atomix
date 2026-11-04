#!/usr/bin/env bash
# Sign atomix.app, wrap it in a DMG, and (when credentials are present) notarize and staple it.
#
# Usage: packaging/macos/package-dmg.sh <install-dir> <out-dmg>
#   <install-dir>  `cmake --install` prefix holding atomix.app (Qt already deployed into it)
#   <out-dmg>      DMG to write, e.g. dist/atomix-0.2.0-macos-arm64.dmg
#
# Signing (optional; without an identity the app is ad-hoc signed and only runs on other Macs
# after the user clears the quarantine flag):
#   MACOS_SIGN_IDENTITY   "Developer ID Application: Name (TEAMID)" or its SHA-1 hash. If unset, the
#                         first Developer ID Application identity in the keychain is used, if any.
# Notarization (optional; needs a Developer ID identity too). App Store Connect API key:
#   APPLE_API_KEY_ID      key ID (10 characters)
#   APPLE_API_ISSUER_ID   issuer UUID
#   APPLE_API_KEY_PATH    path to the AuthKey_<id>.p8 file
#
# Written for the bash 3.2 that ships with macOS.
set -euo pipefail

if [ $# -ne 2 ]; then
    echo "usage: $0 <install-dir> <out-dmg>" >&2
    exit 2
fi
INSTALL_DIR=$(cd "$1" && pwd)
OUT_DMG="$2"
APP="$INSTALL_DIR/atomix.app"

[ -d "$APP" ] || { echo "error: $APP not found (run cmake --install first)" >&2; exit 1; }
mkdir -p "$(dirname "$OUT_DMG")"
OUT_DMG="$(cd "$(dirname "$OUT_DMG")" && pwd)/$(basename "$OUT_DMG")"

# All Mach-O files in the bundle (main executable, dylibs, framework binaries, plugins).
macho_files() {
    find "$APP/Contents" -type f -print0 | while IFS= read -r -d '' f; do
        if file -b "$f" | grep -q '^Mach-O'; then printf '%s\n' "$f"; fi
    done
}

# --- 1. Linkage check --------------------------------------------------------------------------
# Every dependency must resolve inside the bundle (@rpath/@executable_path/@loader_path) or to the
# OS. An absolute path to the build machine (vcpkg_installed, Qt install dir, Homebrew) would work
# on the CI runner and fail on every other Mac.
echo "== Checking library references"
bad=0
while IFS= read -r f; do
    # Dependency lines from otool -L start with a tab; header lines (file name, architecture) don't.
    deps=$(otool -L "$f" | awk '/^\t/ { print $1 }')
    for d in $deps; do
        case "$d" in
            @rpath/*|@executable_path/*|@loader_path/*|/System/Library/*|/usr/lib/*) ;;
            *) echo "  ${f#"$APP"/}: links $d"; bad=1 ;;
        esac
    done
done < <(macho_files)
if [ "$bad" -ne 0 ]; then
    echo "error: bundle references libraries outside the app and the OS" >&2
    exit 1
fi

# --- 2. Signing --------------------------------------------------------------------------------
IDENTITY="${MACOS_SIGN_IDENTITY:-}"
if [ -z "$IDENTITY" ]; then
    IDENTITY=$(security find-identity -v -p codesigning 2>/dev/null \
        | awk '/"Developer ID Application: / { print $2; exit }') || true
fi

# Finder metadata or resource forks anywhere in the bundle make codesign fail.
xattr -cr "$APP"

if [ -n "$IDENTITY" ]; then
    echo "== Signing with Developer ID ($IDENTITY), hardened runtime"
    SIGN=(codesign --force --timestamp --options runtime --sign "$IDENTITY")
else
    # Ad-hoc: no Team ID, so no hardened runtime (its library validation would reject the
    # ad-hoc-signed frameworks) and no timestamp.
    echo "== No Developer ID identity: ad-hoc signing (the DMG will not pass Gatekeeper)"
    SIGN=(codesign --force --sign -)
fi

# Inside-out: loose Mach-O files (dylibs, plugins), then framework bundles, then the app itself.
# --force replaces signatures that Qt and MoltenVK binaries arrive with.
while IFS= read -r f; do
    case "$f" in
        *.framework/*) ;;                          # signed as part of its framework below
        "$APP/Contents/MacOS/atomix") ;;           # signed with the app bundle
        *) "${SIGN[@]}" "$f" ;;
    esac
done < <(macho_files)
find "$APP/Contents" -type d -name '*.framework' -prune -print | while IFS= read -r fw; do
    "${SIGN[@]}" "$fw"
done
# No entitlements: the Qt we build has the QML JIT disabled (build-qt.sh), so the hardened runtime
# needs no exceptions.
"${SIGN[@]}" "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"

# --- 3. DMG ------------------------------------------------------------------------------------
echo "== Creating $OUT_DMG"
STAGING=$(mktemp -d)
trap 'rm -rf "$STAGING"' EXIT
ditto "$APP" "$STAGING/atomix.app"
ln -s /Applications "$STAGING/Applications"
rm -f "$OUT_DMG"
# hdiutil on CI runners sometimes fails with "Resource busy"; retry a few times.
for attempt in 1 2 3 4; do
    if hdiutil create -volname atomix -srcfolder "$STAGING" -fs HFS+ -format UDZO -ov "$OUT_DMG"; then
        break
    fi
    [ "$attempt" -lt 4 ] || { echo "error: hdiutil create failed" >&2; exit 1; }
    echo "hdiutil create failed (attempt $attempt), retrying"
    sleep $((attempt * 5))
done

if [ -z "$IDENTITY" ]; then
    echo "== Done (ad-hoc): $OUT_DMG"
    exit 0
fi
codesign --force --timestamp --sign "$IDENTITY" "$OUT_DMG"

# --- 4. Notarize + staple ----------------------------------------------------------------------
if [ -z "${APPLE_API_KEY_ID:-}" ] || [ -z "${APPLE_API_ISSUER_ID:-}" ] || [ -z "${APPLE_API_KEY_PATH:-}" ]; then
    echo "== Signed but NOT notarized (APPLE_API_KEY_ID / APPLE_API_ISSUER_ID / APPLE_API_KEY_PATH not all set): $OUT_DMG"
    exit 0
fi

echo "== Notarizing (this usually takes a few minutes)"
NOTARY_AUTH=(--key "$APPLE_API_KEY_PATH" --key-id "$APPLE_API_KEY_ID" --issuer "$APPLE_API_ISSUER_ID")
RESULT_JSON="$STAGING/notary.json"
set +e
xcrun notarytool submit "$OUT_DMG" "${NOTARY_AUTH[@]}" --wait --timeout 30m --output-format json > "$RESULT_JSON"
submit_rc=$?
set -e
cat "$RESULT_JSON"; echo
SUBMISSION_ID=$(plutil -extract id raw -o - "$RESULT_JSON" 2>/dev/null || true)
STATUS=$(plutil -extract status raw -o - "$RESULT_JSON" 2>/dev/null || true)
# The log names each rejected binary and the reason; print it whatever the outcome.
if [ -n "$SUBMISSION_ID" ]; then
    xcrun notarytool log "$SUBMISSION_ID" "${NOTARY_AUTH[@]}" || true
fi
if [ "$submit_rc" -ne 0 ] || [ "$STATUS" != "Accepted" ]; then
    echo "error: notarization failed (exit $submit_rc, status '${STATUS:-unknown}')" >&2
    exit 1
fi

xcrun stapler staple "$OUT_DMG"
xcrun stapler validate "$OUT_DMG"
spctl --assess --type open --context context:primary-signature --verbose=2 "$OUT_DMG"
echo "== Done (signed + notarized): $OUT_DMG"
