#!/bin/sh

export QTDIR=/home/braer/Qt/6.8.0/gcc_64

mkdir -p install/appdir/usr/bin
mkdir -p install/appdir/usr/lib
mkdir -p install/appdir/usr/share/applications
mkdir -p install/appdir/usr/share/icons/hicolor/512x512/apps
mkdir -p install/appdir/usr/share/icons/hicolor/256x256/apps
mkdir -p install/appdir/usr/share/configs
mkdir -p install/appdir/usr/share/shaders
mkdir -p install/appdir/usr/share/res

cp src/Release/atomix install/appdir/usr/bin/
cp ../doc/atomix.desktop install/appdir/usr/share/applications/
cp ../res/icons/atomix512.png install/appdir/usr/share/icons/hicolor/512x512/apps/atomix.png
cp ../res/icons/atomix256.png install/appdir/usr/share/icons/hicolor/256x256/apps/atomix.png

cp ../configs/* install/appdir/usr/share/configs/
cp ../shaders/* install/appdir/usr/share/shaders/
cp -r ../res/fonts install/appdir/usr/share/res/

appimagetool-860-x86_64.AppImage -s deploy install/appdir/usr/share/applications/atomix.desktop
VERSION=1.0 appimagetool-860-x86_64.AppImage ./install/appdir
