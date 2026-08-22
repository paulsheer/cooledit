#!/bin/bash
# Build script for RemoteFS Android APK and AAB
set -e

export ANDROID_HOME=/opt/android-sdk
export ANDROID_SDK_ROOT=/opt/android-sdk
export ANDROID_NDK_HOME=/opt/android-sdk/ndk/28.2.13676358

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
    /opt/gradle-9.1.0/bin/gradle assembleDebug 2>&1
    APK="$SCRIPT_DIR/app/build/outputs/apk/debug/app-debug.apk"
    echo "APK: $APK"
else
    /opt/gradle-9.1.0/bin/gradle assembleRelease bundleRelease 2>&1
    APK="$SCRIPT_DIR/app/build/outputs/apk/release/app-release.apk"
    AAB="$SCRIPT_DIR/app/build/outputs/bundle/release/app-release.aab"
    cp "$APK" "$SCRIPT_DIR/../../remotefs.apk"
    cp "$AAB" "$SCRIPT_DIR/../../remotefs.aab"
    echo "APK: $APK"
    echo "AAB: $AAB"
fi
