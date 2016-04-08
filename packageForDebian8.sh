#!/bin/bash

initializeVariables()
{
  APPLICATION_NAME="OpenBoard"
  VERSION=`cat build/linux/release/version`
  MAKE_TAG=true
  STANDARD_QT_USED=false

  PRODUCT_PATH="build/linux/release/product"
  QT_PATH="/opt/qt55"
  PLUGINS_PATH="$QT_PATH/plugins"
  GUI_TRANSLATIONS_DIRECTORY_PATH="$QT_PATH/translations"
  QT_LIBRARY_DEST_PATH="$PRODUCT_PATH/qtlib"
  QT_LIBRARY_SOURCE_PATH="$QT_PATH/lib"
  if [ -z $ARCHITECTURE ]; then
    ARCHITECTURE=`uname -m`
    if [ "$ARCHITECTURE" == "x86_64" ]; then
      ARCHITECTURE="amd64"
    fi
    if [ "$ARCHITECTURE" == "i686" ]; then
      ARCHITECTURE="i386"
    fi
  fi
  #NOTIFY_CMD=`which notify-send`
  QMAKE_PATH="$QT_PATH/bin/qmake"
  LRELEASES="$QT_PATH/bin/lrelease"
  ZIP_PATH=`which zip`
  
}

initializeVariables

BASE_WORKING_DIR="packageBuildDir"

#creating package directory
#mkdir -p $BASE_WORKING_DIR
mkdir -p "$BASE_WORKING_DIR/DEBIAN"
mkdir -p "$BASE_WORKING_DIR/usr/share/applications"
mkdir -p "$BASE_WORKING_DIR/usr/local"

cat > "$BASE_WORKING_DIR/DEBIAN/prerm" << EOF
#!/bin/bash
# --------------------------------------------------------------------
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# ---------------------------------------------------------------------

xdg-desktop-menu uninstall /usr/share/applications/${APPLICATION_NAME}.desktop
exit 0
#DEBHELPER#
EOF

cat > "$BASE_WORKING_DIR/DEBIAN/postinst" << EOF
#!/bin/bash
# --------------------------------------------------------------------
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# ---------------------------------------------------------------------

xdg-desktop-menu install --novendor /usr/share/applications/${APPLICATION_NAME}.desktop
exit 0
#DEBHELPER#
EOF


APPLICATION_DIRECTORY_NAME="${APPLICATION_NAME}-$VERSION"
PACKAGE_DIRECTORY="$BASE_WORKING_DIR/usr/local/$APPLICATION_DIRECTORY_NAME"
#move build directory to packages directory
cp -R $PRODUCT_PATH $PACKAGE_DIRECTORY


cat > $BASE_WORKING_DIR/usr/local/$APPLICATION_DIRECTORY_NAME/run.sh << EOF
#!/bin/bash
# --------------------------------------------------------------------
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# ---------------------------------------------------------------------

env LD_LIBRARY_PATH=/usr/local/$APPLICATION_DIRECTORY_NAME/qtlib:$LD_LIBRARY_PATH /usr/local/$APPLICATION_DIRECTORY_NAME/${APPLICATION_NAME}
EOF


CHANGE_LOG_FILE="$BASE_WORKING_DIR/DEBIAN/changelog-${APPLICATION_NAME}-$VERSION.txt"
CONTROL_FILE="$BASE_WORKING_DIR/DEBIAN/control"
CHANGE_LOG_TEXT="changelog.txt"

echo "${APPLICATION_NAME} ($VERSION) $ARCHITECTURE; urgency=low" > "$CHANGE_LOG_FILE"
echo >> "$CHANGE_LOG_FILE"

#cat $CHANGE_LOG_TEXT >> "$CHANGE_LOG_FILE"

echo >> "$CHANGE_LOG_FILE"
echo "-- Claudio Valerio <claudio.valerio@oe-f.org>  `date`" >> "$CHANGE_LOG_FILE"

echo "Package: ${APPLICATION_NAME}" > "$CONTROL_FILE"
echo "Version: $VERSION" >> "$CONTROL_FILE"
echo "Section: education" >> "$CONTROL_FILE"
echo "Priority: optional" >> "$CONTROL_FILE"
echo "Architecture: $ARCHITECTURE" >> "$CONTROL_FILE"
echo "Essential: no" >> "$CONTROL_FILE"
echo "Installed-Size: `du -s $PACKAGE_DIRECTORY | awk '{ print $1 }'`" >> "$CONTROL_FILE"
echo "Maintainer: ${APPLICATION_NAME} Developers team <dev@oe-f.org>" >> "$CONTROL_FILE"
echo "Homepage: http://www.openboard.org" >> "$CONTROL_FILE"
echo -n "Depends: " >> "$CONTROL_FILE"


# Find dependencies and put them in the depends section
# TODO: This needs to be redone since the version is not found for each package
#       Quick-fix is to put a static list
#unset tab
#declare -a tab
#let count=0
#for l in `objdump -p $PACKAGE_DIRECTORY/${APPLICATION_NAME} | grep NEEDED | awk '{ print $2 }'`; do
#    for lib in `dpkg -S  $l | awk -F":" '{ print $1 }'`; do
#        #echo $lib
#        presence=`echo ${tab[*]} | grep -c "$lib"`;
#        if [ "$presence" == "0" ]; then
#            tab[$count]=$lib;
#            ((count++));
#        fi;
#    done;
#done;

#for ((i=0;i<${#tab[@]};i++)); do
#    if [ $i -ne "0" ]; then
#        echo -n ", " >> "$CONTROL_FILE"
#    fi
#    echo -n "${tab[$i]} (>= "`dpkg -p ${tab[$i]} | grep "Version: " | awk '{      print $2 }' | sed -e 's/\([:. 0-9?]*\).*/\1/g' | sed -e 's/\.$//'`") " >> "$CONTROL_FILE"
#done
#echo -n ", onboard" >> "$CONTROL_FILE"
########### Quick fix ###################
echo -n "libpaper1 , zlib1g (>= 1.2.8) , libssl1.0.0 (>= 1.0.1) , libx11-6, libqt5webkit5, qt55webkit, qt55svg, qt55multimedia, qt55base, libqt5printsupport5, libqt5widgets5, libqt5gui5, qt55xmlpatterns, libqt5network5, libqt5xml5, qt55script, libqt5core5a, libgl1-mesa-glx, libc6 (>= 2.19), libstdc++6 (>= 4.9.2), libgomp1, libgcc1 (>= 4.9.2), caribou" >> "$CONTROL_FILE"
#########################################

echo "" >> "$CONTROL_FILE"
echo "Description: This a interactive white board that uses a free standard format." >> "$CONTROL_FILE"

find $BASE_WORKING_DIR/usr/ -exec md5sum {} > $BASE_WORKING_DIR/DEBIAN/md5sums 2>/dev/null \;
APPLICATION_SHORTCUT="$BASE_WORKING_DIR/usr/share/applications/${APPLICATION_NAME}.desktop"
echo "[Desktop Entry]" > $APPLICATION_SHORTCUT
echo "Version=$VERSION" >> $APPLICATION_SHORTCUT
echo "Encoding=UTF-8" >> $APPLICATION_SHORTCUT
echo "Name=${APPLICATION_NAME} ($VERSION)" >> $APPLICATION_SHORTCUT
echo "GenericName=${APPLICATION_NAME}" >> $APPLICATION_SHORTCUT
echo "Comment=Logiciel de création de présentations pour tableau numérique interactif (TNI)" >> $APPLICATION_SHORTCUT
echo "Exec=/usr/local/$APPLICATION_DIRECTORY_NAME/run.sh" >> $APPLICATION_SHORTCUT
echo "Icon=/usr/local/$APPLICATION_DIRECTORY_NAME/${APPLICATION_NAME}.png" >> $APPLICATION_SHORTCUT
echo "StartupNotify=true" >> $APPLICATION_SHORTCUT
echo "Terminal=false" >> $APPLICATION_SHORTCUT
echo "Type=Application" >> $APPLICATION_SHORTCUT
echo "Categories=Education;" >> $APPLICATION_SHORTCUT
cp "resources/images/${APPLICATION_NAME}.png" "$PACKAGE_DIRECTORY/${APPLICATION_NAME}.png"
chmod 755 "$BASE_WORKING_DIR/DEBIAN"
chmod 755 "$BASE_WORKING_DIR/DEBIAN/prerm"
chmod 755 "$BASE_WORKING_DIR/DEBIAN/postinst"

mkdir -p "install/linux"
DEBIAN_PACKAGE_NAME="${APPLICATION_NAME}_`lsb_release -is`_`lsb_release -rs`_${VERSION}_$ARCHITECTURE.deb"

#chown -R root:root $BASE_WORKING_DIR
dpkg -b "$BASE_WORKING_DIR" "install/linux/$DEBIAN_PACKAGE_NAME"

#clean up mess
rm -rf $BASE_WORKING_DIR

echo "${APPLICATION_NAME}" "Package built"


exit 0
