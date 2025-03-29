#!/usr/bin/env bash

set -e

firebase appdistribution:distribute android/app/build/outputs/apk/release/zipalign-app-release-signed.apk --app 1:30527513892:android:a92a8bd70821d60141ffb1 --release-notes "Hello" --testers "maquinadecafecoado@gmail.com"

