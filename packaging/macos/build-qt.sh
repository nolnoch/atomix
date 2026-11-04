#!/usr/bin/env bash
# Build the parts of Qt that atomix needs (qtbase + qtdeclarative) from source, with Vulkan enabled.
#
# Why: Qt's prebuilt macOS binaries (online installer, aqtinstall) are configured without Vulkan,
# so QVulkanWindow/QVulkanInstance don't exist in them. Qt's Vulkan support only needs the Vulkan
# headers at build time; at run time it loads the loader we bundle (see main.cpp, QT_VULKAN_LIB).
#
# Usage: packaging/macos/build-qt.sh <install-prefix> [arch]
#   <install-prefix>  where Qt is installed (use it as QT_ROOT_DIR afterwards)
#   [arch]            arm64 or x86_64 (default: this machine's)
#
# Needs: Xcode command line tools, CMake, Ninja, git, curl. Takes 30-60 minutes.
set -euo pipefail

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "usage: $0 <install-prefix> [arch]" >&2
    exit 2
fi
mkdir -p "$1"
PREFIX=$(cd "$1" && pwd)
ARCH="${2:-$(uname -m)}"

QT_VERSION=6.12.0
QTBASE_SHA256=a951bd163c7b80fc6b8c88d7668fb56abf91c152373e13c10666763238131307
QTDECLARATIVE_SHA256=311f3a2603e1973bb59baef9dfa740a376de713157d4782ee043681e889c9260
VULKAN_HEADERS_TAG=vulkan-sdk-1.3.296.0   # same version vcpkg provides to atomix
DEPLOYMENT_TARGET=14.4                    # keep in sync with CMAKE_OSX_DEPLOYMENT_TARGET

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

fetch_qt() {  # fetch_qt <module> <sha256>
    local url="https://download.qt.io/official_releases/qt/${QT_VERSION%.*}/$QT_VERSION/submodules/$1-everywhere-src-$QT_VERSION.tar.xz"
    curl -fsSL -o "$1.tar.xz" "$url"
    echo "$2  $1.tar.xz" | shasum -a 256 -c -
    tar -xf "$1.tar.xz"
}
fetch_qt qtbase "$QTBASE_SHA256"
fetch_qt qtdeclarative "$QTDECLARATIVE_SHA256"
git clone --quiet --depth 1 --branch "$VULKAN_HEADERS_TAG" https://github.com/KhronosGroup/Vulkan-Headers.git

COMMON_CMAKE_ARGS=(
    -DCMAKE_OSX_ARCHITECTURES="$ARCH"
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET"
)

# --- qtbase ---
mkdir build-qtbase
(
    cd build-qtbase
    "../qtbase-everywhere-src-$QT_VERSION/configure" \
        -prefix "$PREFIX" -release -nomake examples -nomake tests \
        -- "${COMMON_CMAKE_ARGS[@]}" -DVulkan_INCLUDE_DIR="$WORK/Vulkan-Headers/include"
    # A Qt without Vulkan builds fine and only fails later, when atomix includes QVulkanWindow.
    if ! grep -Eq '^ *Vulkan \.+ yes' config.summary; then
        echo "error: Qt configure did not enable Vulkan:" >&2
        grep -E '^ *Vulkan' config.summary >&2 || true
        exit 1
    fi
    cmake --build . --parallel
    cmake --install .
)

# --- qtdeclarative (QtQml, for QJSEngine) ---
# Without qtshadertools, Qt Quick is skipped automatically; atomix only uses QtQml.
# The QML JIT is disabled (as on iOS): the interpreter is plenty for evaluating UI expressions,
# and it means the hardened runtime needs no JIT entitlements.
mkdir build-qtdeclarative
(
    cd build-qtdeclarative
    "$PREFIX/bin/qt-configure-module" "../qtdeclarative-everywhere-src-$QT_VERSION" \
        -- "${COMMON_CMAKE_ARGS[@]}" -DFEATURE_qml_jit=OFF
    cmake --build . --parallel
    cmake --install .
)

echo "Qt $QT_VERSION ($ARCH, Vulkan enabled) installed to $PREFIX"
