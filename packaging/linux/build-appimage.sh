#!/usr/bin/env bash
# Build atomix-<version>-x86_64.AppImage from a configured+built release tree.
#
# Usage: packaging/linux/build-appimage.sh <build-dir> <output-dir>
#   <build-dir>   CMake build tree (e.g. build/linux-release), already built
#   <output-dir>  where the .AppImage is written
#
# Needs: Qt's qmake on PATH or QMAKE set (linuxdeploy-plugin-qt uses it to find Qt's plugins),
#        curl, and the usual X11/Wayland/xkb runtime libs that Qt's xcb plugin links against.
set -euo pipefail

BUILD_DIR=$(realpath "$1")
OUT_DIR=$(realpath -m "$2")
SRC_DIR=$(realpath "$(dirname "$0")/../..")

LINUXDEPLOY_VERSION=1-alpha-20251107-1
LINUXDEPLOY_QT_VERSION=1-alpha-20250213-1

TOOLS_DIR="$BUILD_DIR/appimage-tools"
APPDIR="$BUILD_DIR/AppDir"
mkdir -p "$TOOLS_DIR" "$OUT_DIR"

fetch() {  # fetch <url> <dest>
    [ -x "$2" ] || { curl -fsSL -o "$2" "$1"; chmod +x "$2"; }
}
fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LINUXDEPLOY_VERSION/linuxdeploy-x86_64.AppImage" \
      "$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
fetch "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/$LINUXDEPLOY_QT_VERSION/linuxdeploy-plugin-qt-x86_64.AppImage" \
      "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"

# CI runners and containers usually lack FUSE; run the tool AppImages by extracting them instead.
export APPIMAGE_EXTRACT_AND_RUN=1
export PATH="$TOOLS_DIR:$PATH"

# Fresh AppDir from the CMake install rules (binary, data, .desktop, icons).
rm -rf "$APPDIR"
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

VERSION=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$BUILD_DIR/CMakeCache.txt")
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
# Only the xcb platform plugin is bundled (linuxdeploy-plugin-qt's default). On Wayland desktops Qt
# falls back to it via XWayland, which is more dependable than a partial native-Wayland deployment.
export QML_SOURCES_PATHS="$SRC_DIR/src"   # atomix has no .qml files (it only uses QJSEngine)

cd "$OUT_DIR"
# libvulkan.so.1 is left out on purpose: the system loader is the one that knows the installed GPU drivers.
"$TOOLS_DIR/linuxdeploy-x86_64.AppImage" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/atomix" \
    --desktop-file "$APPDIR/usr/share/applications/atomix.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/atomix.png" \
    --exclude-library 'libvulkan.so*' \
    --plugin qt \
    --output appimage

ls -l "$OUT_DIR"/atomix-*.AppImage
