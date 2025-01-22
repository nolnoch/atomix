#!/bin/sh

SRCDIR="/Users/braer/dev/atomix"
BLDDIR="/Users/braer/dev/atomix/build"
INSTDIR="/Users/braer/dev/atomix/build/install"
VKDIR="/Users/braer/VulkanSDK/1.3.296.0/macOS"
QTDIR="/usr/local/Qt-6.8.0"
LIBDIR="atomix.app/Contents/Frameworks"
RESDIR="atomix.app/Contents/Resources"

mkdir -p $INSTDIR 
cp -r src/$1/atomix.app $INSTDIR/
echo "Leveraging src/$1/atomix.app for install"

cd $INSTDIR

mkdir -p $RESDIR/vulkan
mkdir -p $LIBDIR

cp -r $SRCDIR/configs $RESDIR/
cp -r $SRCDIR/shaders $RESDIR/
cp -r $SRCDIR/res $RESDIR/
cp $SRCDIR/res/icons/atomix.icns $RESDIR/
cp $VKDIR/lib/libvulkan.1.3.296.dylib $LIBDIR/
cp $VKDIR/lib/libMoltenVK.dylib $LIBDIR/
cp -r $VKDIR/share/vulkan/icd.d $RESDIR/vulkan/
cp -r $VKDIR/share/vulkan/explicit_layer.d $RESDIR/vulkan/
sed -i '' 's|lib/libMolten|../Frameworks/libMolten|g' $RESDIR/vulkan/icd.d/MoltenVK_icd.json

cd $LIBDIR

ln -s libvulkan.1.3.296.dylib libvulkan.1.dylib
ln -s libvulkan.1.dylib libvulkan.dylib

cd $INSTDIR

$QTDIR/bin/macdeployqt atomix.app -libpath=$QTDIR/lib
codesign --force --deep --sign "Apple Development: braernoch@gmail.com (PC3TVMS8YJ)" atomix.app
codesign --verify --verbose atomix.app

# cd $BLDDIR

# mkdir package
# cp install/atomix.app package/
