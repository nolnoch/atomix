#!/bin/sh

LIBDIR="install/atomix.app/Contents/Frameworks"
RESDIR="install/atomix.app/Contents/Resources"
SRCDIR=".."

mkdir -p $RESDIR/configs
mkdir -p $RESDIR/shaders
mkdir -p $RESDIR/res
mkdir -p $LIBDIR

cp -r $SRCDIR/configs/* $RESDIR/configs/
cp -r $SRCDIR/shaders/* $RESDIR/shaders/
cp -r $SRCDIR/res/* $RESDIR/res/
cp $SRCDIR/res/icons/atomix.icns $RESDIR/
cp vcpkg_installed/$VCPKG_TARGET_TRIPLET/lib/libvulkan.1.3.296.dylib $LIBDIR/
cp vcpkg_installed/$VCPKG_TARGET_TRIPLET/lib/libMoltenVK.dylib $LIBDIR/

ln -s $LIBDIR/libvulkan.1.3.296.dylib $LIBDIR/libvulkan.1.dylib
ln -s $LIBDIR/libvulkan.1.3.296.dylib $LIBDIR/libvulkan.dylib
