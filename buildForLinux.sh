#!/bin/bash
# --------------------------------------------------------------------
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
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


#**********************
#     functions
#**********************

checkUser()
{
  if [ `id -u` -ne 0 ]; then
    echo "Please run the script as root, may be using fakeroot command as follow"
    echo "fakeroot ./buildDebianPackage.sh [options]"
    exit 1
  fi
}

initializeVariables()
{
  APPLICATION_NAME="OpenBoard"
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
  NOTIFY_CMD=`which notify-send`
  QMAKE_PATH="$QT_PATH/bin/qmake"
  LRELEASES="$QT_PATH/bin/lrelease"
  ZIP_PATH=`which zip`

}


notifyError(){
    if [ -e "$NOTIFY_CMD" ]; then
        $NOTIFY_CMD -t 0 -i "/usr/share/icons/oxygen/64x64/status/dialog-error.png" "$1"
    fi
    printf "\033[31merror:\033[0m $1\n"
    exit 1
}

notifyProgress(){
    if [ -e "$NOTIFY_CMD" ]; then
        $NOTIFY_CMD "$1" "$2"
    fi
    printf "\033[32m--> Achieved task:\033[0m $1:\n\t$2\n"
}

alertIfPreviousVersionInstalled(){
    APT_CACHE=`which apt-cache`
    if [ ! -e "$APT_CACHE" ]; then
        notifyError "apt-cache command not found"
    else
        SEARCH_RESULT=`$APT_CACHE search ${APPLICATION_NAME}`
        if [ `echo $SEARCH_RESULT | grep -c ${APPLICATION_NAME}` -ge 1 ]; then
            notifyError "Found a previous version of ${APPLICATION_NAME}. Remove it to avoid to put it as dependency"
        fi
    fi
}

checkDir(){
    if [ ! -d "$1" ]; then
        notifyError "Directory not found : $1"
    fi
}

checkExecutable(){
    if [ ! -e "$1" ]; then
        notifyError "$1 command not found"
    fi
}

copyQtLibrary(){
    if ls "$QT_LIBRARY_SOURCE_PATH/$1.so" &> /dev/null; then
        cp -P $QT_LIBRARY_SOURCE_PATH/$1.so.? "$QT_LIBRARY_DEST_PATH/"
        cp -P $QT_LIBRARY_SOURCE_PATH/$1.so.?.? "$QT_LIBRARY_DEST_PATH/"
        cp -P $QT_LIBRARY_SOURCE_PATH/$1.so.?.?.? "$QT_LIBRARY_DEST_PATH/"
    else
        notifyError "$1 library not found in path: $QT_LIBRARY_SOURCE_PATH"
    fi
}


buildWithStandardQt(){
  # if both Qt4 and Qt5 are installed, choose Qt5
  export QT_SELECT=5
  STANDARD_QT=`which qmake`
  if [ $? == "0" ]; then
    QT_VERSION=`$STANDARD_QT --version | grep -i "Using Qt version" | sed -e "s/Using Qt version \(.*\) in.*/\1/"`
    if [ `echo $QT_VERSION | sed -e "s/\.//g"` -gt 480 ]; then
        notifyProgress "Standard QT" "A recent enough qmake has been found. Using this one instead of custom one"
        STANDARD_QT_USED=true
        QMAKE_PATH=$STANDARD_QT
        LRELEASES=`which lrelease`
        PLUGINS_PATH="$STANDARD_QT/../plugins"
    fi
  fi
}

buildImporter(){
    IMPORTER_DIR="../OpenBoard-Importer/"
    IMPORTER_NAME="OpenBoardImporter"
    checkDir $IMPORTER_DIR
    cd ${IMPORTER_DIR}

    rm moc_*
    rm -rf debug release
    rm *.o

    #git reset --hard
    #git pull

    $QMAKE_PATH ${IMPORTER_NAME}.pro
    make clean
    make -j4
    checkExecutable $IMPORTER_NAME
    cd -
}

#**********************
#     script
#**********************
checkUser

for var in "$@"
do
   if [ $var == "notag" ]; then
     MAKE_TAG=false
   fi
# forcing a architecture because of cross compiling
   if [ $var == "i386" ]; then
      ARCHITECTURE="i386"
   fi
   if [ $var == "amd64" ]; then
      ARCHITECTURE="amd64"
   fi
done

initializeVariables
#buildWithStandardQt

alertIfPreviousVersionInstalled

# check of directories and executables
checkDir $QT_PATH
checkDir $PLUGINS_PATH
checkDir $GUI_TRANSLATIONS_DIRECTORY_PATH

checkExecutable $QMAKE_PATH
checkExecutable $LRELEASES
checkExecutable $ZIP_PATH

#build third party application
buildImporter
notifyProgress "OpenBoardImporter" "Built Importer"

# cleaning the build directory
rm -rf "build/linux/release"
rm -rf install

notifyProgress "QT" "Internalization"
$LRELEASES ${APPLICATION_NAME}.pro
cd $GUI_TRANSLATIONS_DIRECTORY_PATH
$LRELEASES translations.pro
cd -

notifyProgress "${APPLICATION_NAME}" "Building ${APPLICATION_NAME}"

if [ "$ARCHITECTURE" == "amd64" ]; then
    $QMAKE_PATH ${APPLICATION_NAME}.pro -spec linux-g++-64
else
    $QMAKE_PATH ${APPLICATION_NAME}.pro -spec linux-g++
fi

make -j 4 release-install

if [ ! -e "$PRODUCT_PATH/${APPLICATION_NAME}" ]; then
    notifyError "${APPLICATION_NAME} build failed"
fi

notifyProgress "Git Hub" "Make a tag of the delivered version"

VERSION=`cat build/linux/release/version`

if [ ! -f build/linux/release/version ]; then
    notifyError "version not found"
#else
#    LAST_COMMITED_VERSION="`git describe $(git rev-list --tags --max-count=1)`"
#    if [ "v$VERSION" != "$LAST_COMMITED_VERSION" ]; then
#        if [ $MAKE_TAG == true ]; then
#            git tag -a "OBv$VERSION" -m "OpenBoard setup for v$VERSION"
#            git push origin --tags
#        fi
#    fi
fi

cp resources/linux/run.sh $PRODUCT_PATH
chmod a+x $PRODUCT_PATH/run.sh

cp -R resources/linux/qtlinux/* $PRODUCT_PATH/

notifyProgress "QT" "Copying plugins and library ..."
cp -R $PLUGINS_PATH $PRODUCT_PATH/

# copying customization
cp -R resources/customizations $PRODUCT_PATH/

# copying importer
mkdir -p $PRODUCT_PATH/Importer
cp -R ${IMPORTER_DIR}/${IMPORTER_NAME} $PRODUCT_PATH/Importer

if [ $STANDARD_QT_USED == false ]; then
#copying custom qt library
  mkdir -p $QT_LIBRARY_DEST_PATH
  copyQtLibrary libQt5Core
  copyQtLibrary libQt5Gui
  copyQtLibrary libQt5Multimedia
  copyQtLibrary libQt5MultimediaWidgets
  copyQtLibrary libQt5Network
  copyQtLibrary libQt5OpenGL
  copyQtLibrary libQt5PrintSupport
  copyQtLibrary libQt5Script
  copyQtLibrary libQt5Svg
  copyQtLibrary libQt5WebKit
  copyQtLibrary libQt5WebKitWidgets
  copyQtLibrary libQt5Xml
  copyQtLibrary libQt5XmlPatterns
fi

notifyProgress "QT" "Internationalization"
if [ ! -e $PRODUCT_PATH/i18n ]; then
    mkdir $PRODUCT_PATH/i18n
fi
#copying qt gui translation
cp $GUI_TRANSLATIONS_DIRECTORY_PATH/qt_??.qm $PRODUCT_PATH/i18n/

rm -rf install/linux
mkdir -p install/linux

#Removing .svn directories ...
cd $PRODUCT_PATH
find . -name .svn -exec rm -rf {} \; 2> /dev/null
cd -

notifyProgress "Building ${APPLICATION_NAME}" "Finished build of ${APPLICATION_NAME}"

exit 0
