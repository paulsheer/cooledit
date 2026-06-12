#!/bin/bash
# Build script for RemoteFS Android APK
set -e

export ANDROID_HOME=/opt/android-sdk
export ANDROID_SDK_ROOT=/opt/android-sdk
export ANDROID_NDK_HOME=/opt/android-sdk/ndk/21.4.7075529

BUILD_TYPE="Release"
while getopts "d" opt; do
    case $opt in
        d) BUILD_TYPE="Debug" ;;
        *) echo "Usage: $0 [-d]"; exit 1 ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ "$BUILD_TYPE" = "Debug" ]; then
    /opt/gradle-7.5.1/bin/gradle assembleDebug 2>&1
    APK="$SCRIPT_DIR/app/build/outputs/apk/debug/app-debug.apk"
else
    /opt/gradle-7.5.1/bin/gradle assembleRelease 2>&1
    APK="$SCRIPT_DIR/app/build/outputs/apk/release/app-release.apk"
    cp $APK $SCRIPT_DIR/../../remotefs.apk
fi

echo "APK: $APK"
