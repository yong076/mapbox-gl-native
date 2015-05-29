#!/usr/bin/env bash

set -e
set -o pipefail

# We only install mesa on the Travis ARM debug job
if [ ${ANDROID_ABI} == "arm-v7" ] && [ ${BUILDTYPE} == "Debug" ] ; then
    source ./scripts/linux/setup.sh
fi

BUILDTYPE=${BUILDTYPE:-Release}
TESTMUNK=${TESTMUNK:-no}
export HOST=android
export MASON_PLATFORM=android
export MASON_ANDROID_ABI=${ANDROID_ABI:-arm-v7}

# Add Mason to PATH
export PATH="`pwd`/.mason:${PATH}" MASON_DIR="`pwd`/.mason"

################################################################################
# Build
################################################################################

mapbox_time "checkout_styles" \
git submodule update --init styles

mkdir -p ./android/java/MapboxGLAndroidSDKTestApp/src/main/res/raw
echo "${MAPBOX_ACCESS_TOKEN}" > ./android/java/MapboxGLAndroidSDKTestApp/src/main/res/raw/token.txt

mapbox_time "compile_library" \
make android-lib -j${JOBS} BUILDTYPE=${BUILDTYPE}

mapbox_time "build_apk" \
make android -j${JOBS} BUILDTYPE=${BUILDTYPE}

################################################################################
# Test
################################################################################

APK_OUTPUTS=./android/java/MapboxGLAndroidSDKTestApp/build/outputs/apk

# Currently release builds will not resign so we have to use debug bulds
# Also we currently only install the ARM emulator
if [ ${ANDROID_ABI} == "arm-v7" ] && [ ${BUILDTYPE} == "Debug" ] ; then

    mapbox_time_start "start_emulator"
    echo "Starting the emulator..."
    android create avd -n test -t android-22 -b armeabi-v7a -d "Nexus 5"
    emulator -avd test -no-skin -no-audio -gpu on &
    adb logcat
    #emulator -avd test -no-skin -no-audio -no-window -gpu on &
    ./scripts/android/wait_for_emulator.sh
    mapbox_time_finish

    mapbox_time_start "calabash_test"
    echo "Running Calabash tests..."

    if [ ${BUILDTYPE} == "Debug" ] ; then
        cp \
            ${APK_OUTPUTS}/MapboxGLAndroidSDKTestApp-debug.apk \
            ./test/android/test.apk
    elif [ ${BUILDTYPE} == "Release" ] ; then
        cp \
            ${APK_OUTPUTS}/MapboxGLAndroidSDKTestApp-release-unsigned.apk \
            ./test/android/test.apk
    fi

    pushd ./test/android
    calabash-android resign test.apk # This fails on release builds because we
                                     # dont have release signing set up in gradle
    calabash-android run test.apk --verbose
    popd

    # TODO upload logcat to s3 along with screenshots
    #adb logcat

    mapbox_time_finish
fi

################################################################################
# Deploy
################################################################################

if [ ! -z "${AWS_ACCESS_KEY_ID}" ] && [ ! -z "${AWS_SECRET_ACCESS_KEY}" ] ; then
    # Install and add awscli to PATH for uploading the results
    mapbox_time "install_awscli" \
    pip install --user awscli
    export PATH="`python -m site --user-base`/bin:${PATH}"

    mapbox_time_start "deploy_results"
    echo "Deploying results..."

    S3_PREFIX=s3://mapbox/mapbox-gl-native/android/build/${TRAVIS_JOB_NUMBER}

    # Upload either the debug or the release build
    if [ ${BUILDTYPE} == "Debug" ] ; then
        aws s3 cp \
            ${APK_OUTPUTS}/MapboxGLAndroidSDKTestApp-debug.apk \
            ${S3_PREFIX}/MapboxGLAndroidSDKTestApp-debug.apk
    elif [ ${BUILDTYPE} == "Release" ] ; then
        aws s3 cp \
            ${APK_OUTPUTS}/MapboxGLAndroidSDKTestApp-release-unsigned.apk \
            ${S3_PREFIX}/MapboxGLAndroidSDKTestApp-release-unsigned.apk
    fi

    mapbox_time_finish
fi
