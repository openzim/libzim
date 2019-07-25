#!/usr/bin/env bash

set -e

REPO_NAME=${TRAVIS_REPO_SLUG#*/}
ARCHIVE_NAME=deps_${TRAVIS_OS_NAME}_${PLATFORM}_${REPO_NAME}.tar.xz

# Ninja
cd $HOME
if [[ "$TRAVIS_OS_NAME" == "osx" ]]
then
  brew update
  brew install gcovr
#  brew upgrade python3
  pip3 install meson==0.49.2 pytest

  wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-mac.zip
  unzip ninja-mac.zip ninja
else
  wget https://bootstrap.pypa.io/get-pip.py
  python3.5 get-pip.py --user
  python3.5 -m pip install --user meson==0.49.2

  wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip
  unzip ninja-linux.zip ninja
fi

mkdir -p $HOME/bin
cp ninja $HOME/bin

# Dependencies comming from kiwix-build.
cd ${HOME}
wget http://tmp.kiwix.org/ci/${ARCHIVE_NAME}
tar xf ${HOME}/${ARCHIVE_NAME}
