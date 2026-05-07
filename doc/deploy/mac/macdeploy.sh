#!/bin/sh
set -e

# === Paths (edit if your install layout changes) ===
SRCDIR="/Users/braer/dev/atomix"
BLDDIR="/Users/braer/dev/atomix/build"
INSTDIR="/Users/braer/dev/atomix/build/install"
VKDIR="/Users/braer/VulkanSDK/1.3.296.0/macOS"
QTDIR="/usr/local/Qt-6.9.0"

# === Signing config ===
SIGN_ID="Developer ID Application: Wade Burch (39683CAT84)"
ENTITLEMENTS="$SRCDIR/doc/deploy/mac/atomix.entitlements"

# notarytool credential profile. Create once with:
#   xcrun notarytool store-credentials AC_PASSWORD \
#     --apple-id "braernoch@gmail.com" \
#     --team-id "39683CAT84" \
#     --password "<app-specific password from appleid.apple.com>"
# Then this script will reuse the keychain entry.
NOTARY_PROFILE="${NOTARY_PROFILE:-AC_PASSWORD}"
# Set SKIP_NOTARIZE=1 to sign-only (useful for fast iteration on a dev Mac).
SKIP_NOTARIZE="${SKIP_NOTARIZE:-0}"

LIBDIR="atomix.app/Contents/Frameworks"
RESDIR="atomix.app/Contents/Resources"
APP="atomix.app"

# ----------------------------------------------------------------------------
# Stage the bundle
# ----------------------------------------------------------------------------
mkdir -p "$INSTDIR"
rm -rf "$INSTDIR/$APP"
cp -R "src/$1/$APP" "$INSTDIR/"
echo "Leveraging src/$1/$APP for install"

cd "$INSTDIR"

mkdir -p "$RESDIR/vulkan"
mkdir -p "$LIBDIR"

cp -R "$SRCDIR/configs" "$RESDIR/"
cp -R "$SRCDIR/shaders" "$RESDIR/"
cp -R "$SRCDIR/res"     "$RESDIR/"
cp    "$SRCDIR/res/icons/atomix.icns" "$RESDIR/"

cp "$VKDIR/lib/libvulkan.1.3.296.dylib" "$LIBDIR/"
cp "$VKDIR/lib/libMoltenVK.dylib"       "$LIBDIR/"
cp -R "$VKDIR/share/vulkan/icd.d"            "$RESDIR/vulkan/"
cp -R "$VKDIR/share/vulkan/explicit_layer.d" "$RESDIR/vulkan/"
sed -i '' 's|lib/libMolten|../Frameworks/libMolten|g' \
    "$RESDIR/vulkan/icd.d/MoltenVK_icd.json"

(
    cd "$LIBDIR"
    ln -sf libvulkan.1.3.296.dylib libvulkan.1.dylib
    ln -sf libvulkan.1.dylib       libvulkan.dylib
)

# ----------------------------------------------------------------------------
# Deploy Qt frameworks (we sign explicitly below, so don't pass -codesign)
# ----------------------------------------------------------------------------
"$QTDIR/bin/macdeployqt" "$APP" -libpath="$QTDIR/lib"

# ----------------------------------------------------------------------------
# Code-sign with hardened runtime, bottom-up.
#
# Order matters: codesign must see signed children before signing the parent,
# otherwise the parent signature is invalidated. Internal dylibs / frameworks
# do NOT get the entitlements file -- entitlements only apply to the main
# executable; everything else inherits at runtime.
# ----------------------------------------------------------------------------
echo "Signing loose dylibs in Frameworks/..."
find "$APP/Contents/Frameworks" -maxdepth 1 -type f -name "*.dylib" -print0 \
    | xargs -0 -I {} codesign --force --options runtime --timestamp \
        --sign "$SIGN_ID" {}

echo "Signing Qt plugins..."
if [ -d "$APP/Contents/PlugIns" ]; then
    find "$APP/Contents/PlugIns" -type f -name "*.dylib" -print0 \
        | xargs -0 -I {} codesign --force --options runtime --timestamp \
            --sign "$SIGN_ID" {}
fi

echo "Signing frameworks (depth-first)..."
find "$APP/Contents/Frameworks" -depth -type d -name "*.framework" -print0 \
    | xargs -0 -I {} codesign --force --options runtime --timestamp \
        --sign "$SIGN_ID" {}

echo "Signing main executable with entitlements..."
codesign --force --options runtime --timestamp \
    --entitlements "$ENTITLEMENTS" \
    --sign "$SIGN_ID" "$APP/Contents/MacOS/atomix"

echo "Signing app bundle with entitlements..."
codesign --force --options runtime --timestamp \
    --entitlements "$ENTITLEMENTS" \
    --sign "$SIGN_ID" "$APP"

echo "Verifying signature..."
codesign --verify --deep --strict --verbose=2 "$APP"

# ----------------------------------------------------------------------------
# Notarize + staple
# ----------------------------------------------------------------------------
if [ "$SKIP_NOTARIZE" = "1" ]; then
    echo "SKIP_NOTARIZE=1 -- skipping notarization."
else
    echo "Submitting for notarization (this can take 1-15 minutes)..."
    ZIP="$BLDDIR/atomix-notarize.zip"
    rm -f "$ZIP"
    /usr/bin/ditto -c -k --keepParent "$APP" "$ZIP"

    xcrun notarytool submit "$ZIP" \
        --keychain-profile "$NOTARY_PROFILE" \
        --wait

    rm -f "$ZIP"

    echo "Stapling notarization ticket to bundle..."
    xcrun stapler staple "$APP"
fi

echo "Final Gatekeeper assessment:"
spctl -a -vvv -t exec "$APP" || true

echo ""
echo "Done. Bundle: $INSTDIR/$APP"
