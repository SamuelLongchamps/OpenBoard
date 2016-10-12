#!/bin/bash

checkUser()
{
  if [ `id -u` -ne 0 ]; then
    echo "Please run the script as root"
    exit 1
  fi
}

checkBuild() {
  if [ -z "$ARCHITECTURE" ]; then
    echo "Make sure you have built the software first using ./buildForLinux.sh"
    exit 1
  fi
}

cleanPackageBuild() {
  rm -rf $PACKAGE_DIRECTORY
  rm -f $APPLICATION_SHORTCUT
}

BASE_WORKING_DIR="debianPackage"

PRODUCT_PATH="build/linux/release/product"
APPLICATION_NAME="openboard"
APPLICATION_CODE="openboard"
APPLICATION_PATH="/usr/local"
PACKAGE_DIRECTORY=$BASE_WORKING_DIR/$APPLICATION_PATH/$APPLICATION_CODE
DESKTOP_FILE_PATH="$BASE_WORKING_DIR/usr/share/applications"
APPLICATION_SHORTCUT="$DESKTOP_FILE_PATH/${APPLICATION_CODE}.desktop"
DESCRIPTION="Interactive white board using a free standard format"
VERSION=`cat build/linux/release/version`
ARCHITECTURE=`cat buildContext`

# Check script's preconditions
checkBuild
checkUser

# Make sure the build directories for the packaging
cleanPackageBuild

mkdir -p $PACKAGE_DIRECTORY
cp -R $PRODUCT_PATH/* $PACKAGE_DIRECTORY
chown -R root:root $PACKAGE_DIRECTORY

find $BASE_WORKING_DIR/usr/ -exec md5sum {} > $BASE_WORKING_DIR/DEBIAN/md5sums 2>/dev/null \;

# Create control file according to latest build
CONTROL_FILE="$BASE_WORKING_DIR/DEBIAN/control"

echo "Package: ${APPLICATION_NAME}" > "$CONTROL_FILE"
echo "Version: $VERSION" >> "$CONTROL_FILE"
echo "Section: education" >> "$CONTROL_FILE"
echo "Priority: optional" >> "$CONTROL_FILE"
echo "Architecture: $ARCHITECTURE" >> "$CONTROL_FILE"
echo "Essential: no" >> "$CONTROL_FILE"
echo "Installed-Size: `du -s $PACKAGE_DIRECTORY | awk '{ print $1 }'`" >> "$CONTROL_FILE"
echo "Maintainer: ${APPLICATION_NAME} Developers team <dev@oe-f.org>" >> "$CONTROL_FILE"
echo "Homepage: https://github.com/DIP-SEM/OpenBoard" >> "$CONTROL_FILE"
echo -n "Depends: " >> "$CONTROL_FILE"
echo -n "libpaper1, zlib1g (>= 1.2.8), libssl1.0.0 (>= 1.0.1), libx11-6, libgl1-mesa-glx, libc6 (>= 2.19), libstdc++6 (>= 4.9.2), libgomp1, libgcc1 (>= 4.9.2), libxcb-keysyms1, libxcb-render-util0, libxcb-image0, libxcb-icccm4" >> "$CONTROL_FILE"
echo "" >> "$CONTROL_FILE"
echo "Description: $DESCRIPTION" >> "$CONTROL_FILE"

# Create .desktop file
mkdir -p $DESKTOP_FILE_PATH
echo "[Desktop Entry]" > $APPLICATION_SHORTCUT
echo "Version=$VERSION" >> $APPLICATION_SHORTCUT
echo "Encoding=UTF-8" >> $APPLICATION_SHORTCUT
echo "Name=OpenBoard" >> $APPLICATION_SHORTCUT
echo "Comment=$DESCRIPTION" >> $APPLICATION_SHORTCUT
echo "Exec=$APPLICATION_CODE %f" >> $APPLICATION_SHORTCUT
echo "Icon=$APPLICATION_PATH/$APPLICATION_CODE/OpenBoard.png" >> $APPLICATION_SHORTCUT
echo "StartupNotify=true" >> $APPLICATION_SHORTCUT
echo "Terminal=false" >> $APPLICATION_SHORTCUT
echo "Type=Application" >> $APPLICATION_SHORTCUT
echo "Categories=Education;" >> $APPLICATION_SHORTCUT
cp "resources/images/OpenBoard.png" "$PACKAGE_DIRECTORY/OpenBoard.png"

# Create the package
mkdir -p "install/linux"
DEBIAN_PACKAGE_NAME="${APPLICATION_NAME}_`lsb_release -is`_`lsb_release -rs`_${VERSION}_$ARCHITECTURE.deb"

dpkg -b "$BASE_WORKING_DIR" "install/linux/$DEBIAN_PACKAGE_NAME"

# Clean the package's build folder
cleanPackageBuild

echo "${APPLICATION_NAME}" "Package built"	
