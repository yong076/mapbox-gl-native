#!/usr/bin/env bash

set -e
set -o pipefail

mapbox_time "install_calabash" \
gem install calabash-android

mapbox_time "install_oily_png" \
gem install oily_png

mapbox_time "checkout_mason" \
git submodule update --init .mason

if [ ${ANDROID_ABI} == "arm-v7" ] && [ ${BUILDTYPE} == "Debug" ] ; then
    PATH="`pwd`/.mason:${PATH}" MASON_DIR="`pwd`/.mason" \
    mapbox_time "install_mesa" \
    mason install mesa 10.4.3
fi

export MASON_PLATFORM=android
export MASON_ANDROID_ABI=${ANDROID_ABI}

mapbox_time "android_toolchain" \
./scripts/android/toolchain.sh
