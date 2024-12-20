#!/bin/sh

export VERSION=1.0
export QTDIR="/home/braer/Qt/6.8.0/gcc_64"

TOOL=$(ls ~/bin | grep 'appimagetool')
INS_FILE="install/appdir/AppRun"
INS_MATCH="  export GTK_THEME=Default # This one should be bundled so that it can work on systems without Gtk"
INS_ENV="  export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:/lib/x86_64-linux-gnu"
PKG="atomix-$VERSION"

mkdir -p install/appdir/usr/bin
mkdir -p install/appdir/usr/lib
mkdir -p install/appdir/usr/share/applications
mkdir -p install/appdir/usr/share/icons/hicolor/512x512/apps
mkdir -p install/appdir/usr/share/icons/hicolor/256x256/apps
# mkdir -p install/appdir/usr/configs
# mkdir -p install/appdir/usr/shaders
# mkdir -p install/appdir/usr/res

cp src/Release/atomix install/appdir/usr/bin/
cp ../doc/deploy/atomix.desktop install/appdir/usr/share/applications/
cp ../res/icons/atomix512.png install/appdir/usr/share/icons/hicolor/512x512/apps/atomix.png
cp ../res/icons/atomix256.png install/appdir/usr/share/icons/hicolor/256x256/apps/atomix.png

# cp ../configs/* install/appdir/usr/configs/
# cp ../shaders/* install/appdir/usr/shaders/
cp -r ../res/* install/appdir/usr/res/

$TOOL -s deploy install/appdir/usr/share/applications/atomix.desktop

mkdir install/appdir/home/braer/Qt/6.8.0/gcc_64/plugins/platformthemes
cp $QTDIR/plugins/platformthemes/* install/appdir/home/braer/Qt/6.8.0/gcc_64/plugins/platformthemes/

sed -i "s|$INS_MATCH|#$INS_MATCH\n$INS_ENV|g" $INS_FILE

$TOOL ./install/appdir

mkdir $PKG
cp -r ../configs $PKG/
cp -r ../shaders $PKG/
cp -r ../res $PKG/
cp atomix-$VERSION-x86_64.AppImage $PKG/
md5sum $PKG/atomix-$VERSION-x86_64.AppImage > $PKG/atomix-$VERSION-x86_64.AppImage.md5
tar -czvf $PKG/atomix-$VERSION-x86_64.AppImage.tar.gz $PKG
