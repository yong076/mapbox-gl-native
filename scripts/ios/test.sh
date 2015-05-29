#!/usr/bin/env bash

set -e
set -o pipefail
set -u

xctool \
    -workspace ./test/ios/ios-tests.xcworkspace \
    -scheme 'Mapbox GL Tests' \
    -sdk iphonesimulator8.3 \
    test
