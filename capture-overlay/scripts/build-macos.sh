#!/bin/sh
# Native fallback for Macs with Command Line Tools but without CMake.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
bundle="$project_dir/build-macos/CaptureOverlay.app"
mkdir -p "$bundle/Contents/MacOS"
xcrun clang++ -std=c++17 -fobjc-arc -Wall -Wextra -Wpedantic -Werror \
    -mmacosx-version-min=11.0 -framework Cocoa \
    "$project_dir/src/main_mac.mm" -o "$bundle/Contents/MacOS/CaptureOverlay"
cp "$project_dir/resources/Info.plist" "$bundle/Contents/Info.plist"
/usr/bin/plutil -lint "$bundle/Contents/Info.plist"
printf 'Built: %s\nLaunch: open "%s"\n' "$bundle" "$bundle"
