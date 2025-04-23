#!/usr/bin/env bash

set -e

ionic capacitor build android --prod --no-open
cap build android
$ANDROID_HOME/build-tools/36.0.0/zipalign -P 16 -f -v 4 android/app/build/outputs/apk/release/app-release-signed.apk android/app/build/outputs/apk/release/zipalign-app-release-signed.apk
