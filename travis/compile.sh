#!/usr/bin/env bash

set -e

BUILD_DIR=${HOME}/BUILD_${PLATFORM}
INSTALL_DIR=${BUILD_DIR}/INSTALL


TEST=0
case ${PLATFORM} in
    "native_static")
         MESON_OPTION="--default-library=static"
         TEST=1
         ;;
    "native_dyn")
         MESON_OPTION="--default-library=shared"
         TEST=1
         ;;
    "win32_static")
         MESON_OPTION="--default-library=static --cross-file ${BUILD_DIR}/meson_cross_file.txt"
         ;;
    "win32_dyn")
         MESON_OPTION="--default-library=shared --cross-file ${BUILD_DIR}/meson_cross_file.txt"
         ;;
    "android_arm")
         MESON_OPTION="-Dandroid=true --default-library=static --cross-file ${BUILD_DIR}/meson_cross_file.txt"
         ;;
    "android_arm64")
         MESON_OPTION="-Dandroid=true --default-library=static --cross-file ${BUILD_DIR}/meson_cross_file.txt"
         ;;

esac

cd ${TRAVIS_BUILD_DIR}
export PKG_CONFIG_PATH=${INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig
meson . build -Dctpp2-install-prefix=${INSTALL_DIR} ${MESON_OPTION}
cd build
ninja
if [[ "$TEST" == "1" ]]
then
    export LD_LIBRARY_PATH=${INSTALL_DIR}/lib:${INSTALL_DIR}/lib64:${INSTALL_DIR}/lib/x86_64-linux-gnu
    echo $LD_LIBRARY_PATH
    meson test --verbose
fi
