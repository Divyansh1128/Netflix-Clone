# Local validation — 7 October 2026

Host: macOS 26.3, Apple Silicon (arm64), Apple clang 16.0.0 and Command Line
Tools SDK. No Windows compiler/SDK, MinGW compiler, Wine, or CMake executable
was detected in the inspected tool locations/PATH.

## Build

Command, from `capture-overlay`:

```sh
sh scripts/build-macos.sh
```

Result: **exit 0**, with `-Wall -Wextra -Wpedantic -Werror`. Bundle Info.plist
validation passed. `file` confirmed a Mach-O arm64 native executable; `otool -L`
showed only Apple system frameworks/libraries. No Python/server runtime needed.

Produced executable:

```text
/Users/divyanshraj/Documents/New project/capture-overlay/build-macos/CaptureOverlay.app/Contents/MacOS/CaptureOverlay
```

## Native core smoke test

```sh
./build-macos/CaptureOverlay.app/Contents/MacOS/CaptureOverlay --self-test
```

Final result: **exit 0**, six checks passed:

- Multiline Unicode editing.
- Always-on-top enabled through the shared menu/shortcut action.
- Always-on-top disabled.
- Window opacity updated to 55%.
- Window opacity restored to 100%.
- Notes area resized with the window.

## Live UI observations

The app was launched and its window inspected visually and through native
accessibility. The notes area and controls fit without clipping at the default
size. Cmd+Shift+T changed the always-on-top checkbox on/off. Unicode paste
(`café 日本語`) was confirmed in the notes. Changing the slider to 65% updated
its value and the opacity label. The capture control was disabled with a clear
Windows-only explanation. Close exited the app (the accessibility process was
then no longer present).

Native minimize animation was observed. **Minimize/restore completion remains
unverified:** the notification-based automated test did not complete within
10 seconds in this session. That OS window-action test remains available as
`--self-test-window-actions`, separate from core smoke checks; the timeout is
not recorded as a pass. Verify minimize and restoration on the normal target
desktop.

## Not validated on this host

| Item | Result |
| --- | --- |
| Windows compilation / produced `.exe` | Not tested: Windows toolchain and SDK unavailable |
| CMake configuration on either platform | Not tested: CMake unavailable |
| Windows display-affinity enable/disable and error paths | Not tested: Windows required |
| Windows DPI, resizing, keyboard shortcuts, opacity, minimize/close | Not tested: Windows required |
| Snipping Tool with exclusion ON/OFF | Not tested: Windows required |
| Microsoft Teams entire-desktop sharing on receiving device | Not tested: no two-device meeting performed |

The Win32 source has been reviewed for own-window API use, SDK constants,
manifest/version compatibility, return-value handling, and `WDA_NONE` reset.
Source review does not establish successful compilation or observed capture
behavior. No Windows `.exe` exists in the build directories at delivery.

Build on a Windows machine using the README commands, then record real ON/OFF
capture observations. Do not replace untested results with an assumed outcome.
