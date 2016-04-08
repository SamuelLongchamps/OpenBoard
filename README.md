# OpenBoard
OpenBoard is an open source cross-platform interactive white board application designed for use in schools. It is a fork of Open-Sankoré, which was itself based on Uniboard.

Supported platforms are Windows (7+), OS X (10.9+) and GNU/Linux (tested on Ubuntu 14.04, but should work with other distributions too).

# Contribute

Repositories OpenBoard-ThirdParty and OpenBoard-Importer are required to build. They should be checked out alongside OpenBoard's repo. OpenBoard-ThirdParty libraries should be built first; instructions are provided for each library (should be done by script, fix me!).

# Dependencies

## Qt 5.5 for GNU/Linux

Due to a shared library conflict within Qt5 on GNU/Linux (the Qt Multimedia and Qt Webkit modules are built against different versions of gstreamer by default), a specific installation of Qt5.5 is needed for all of OpenBoard's features to work correctly.

### Any
It can either be built from source, with the configure flag `-gstreamer 1.0` (see [here](http://doc.qt.io/qt-5/linux-building.html)).

### Ubuntu
Stephan Binner's PPAs on Ubuntu provides the package already built. Add his PPA and install it like so:

    sudo add-apt-repository ppa:beineri/opt-qt551-trusty
    sudo apt-get update
    sudo apt-get install qt-latest

### Debian
An archive containing all the necessary packages and an install script is available [here](https://drive.google.com/open?id=0B7NtQ39nK8wxckw5VnpwWWVaenc).

## Onboard
Onboard has replaced the built-in keyboard.

### Debian 8 Stable
Since it is not supported in Debian 8 Stable, it is currently not working. This should be fixed by the usage of a Qt keyboard if Onboard is not present (fix me!).

# Installation & Deployment

Deployment scripts are provided for all three platforms. These take care of compiling OpenBoard, including the translations (for OpenBoard and for Qt), stripping the debug symbols, creating the installers etc.
Minor modification to those scripts may be necessary depending on your configuration, to set the correct Qt path for example.

## GNU/Linux
Since different GNU/Linux distributions have different packaging systems, there are two scripts for Linux building: one common to all distributions to build the actual software (buildForLinux.sh) and one specific to the distribution. The distribution-specific script only packages the software, so it is assumed that the software has been built successfully prior to its invocation. Currently supported are:

* Debian 8 (packageForDebian8.sh)


