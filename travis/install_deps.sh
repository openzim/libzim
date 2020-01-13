#!/usr/bin/env bash

set -e

if [[ $OS_NAME == "ubuntu" ]]
then
  OS_NAME=linux
fi
ARCHIVE_NAME=deps_${OS_NAME}_${PLATFORM}_${REPO_NAME}.tar.xz


echo "REPO_NAME = $REPO_NAME"
echo "ARCHIVE_NAME = $ARCHIVE_NAME"

pip3 install --user meson==0.49.2 pytest
if [[ "$OS_NAME" == "macos" ]]
then
  wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-mac.zip
  unzip ninja-mac.zip ninja
else
  wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip
  unzip ninja-linux.zip ninja
fi

mkdir -p $HOME/bin
cp ninja $HOME/bin

# Dependencies comming from kiwix-build.
cd ${HOME}
wget http://tmp.kiwix.org/ci/${ARCHIVE_NAME}
tar xf ${HOME}/${ARCHIVE_NAME}
