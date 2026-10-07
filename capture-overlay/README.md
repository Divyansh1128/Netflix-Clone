# Capture Overlay

A small, locally launched native notes window for demonstrating the documented
Windows window display-affinity API. No Python, browser, server, or network
connection is needed by the compiled application.

**The Windows app requests capture exclusion. It does not establish that Teams
or any other capture application actually omits the window.** Remote observation
is a separate experiment.

## Platforms and architecture

- **Windows 10/11:** C++17 and Win32. One owned top-level `HWND`, a multiline
  Unicode `EDIT`, two checkboxes, an opacity trackbar, and status labels.
- **macOS 11 or later:** native Objective-C++/AppKit companion with an `NSWindow`
  and editable `NSTextView`. Builds for the current Mac architecture, including
  Apple Silicon. Notes, opacity, resizing, and always-on-top are supported.
  **Capture exclusion is unavailable in the Mac companion.** Its capture
  checkbox and corresponding menu command are disabled.

`SetWindowDisplayAffinity` is a Windows API. Apple documents the legacy
`NSWindow.SharingType.none` constant as no longer used by macOS; this application
does not present it as an equivalent protection mechanism.
[Apple documentation](https://developer.apple.com/documentation/appkit/nswindow/sharingtype-swift.enum)

```text
capture-overlay/
├── src/
│   ├── main.cpp             # Windows Win32 app
│   ├── main_mac.mm          # macOS AppKit companion
│   ├── windows.rc           # Embedded application manifest
│   └── windows.manifest     # OS compatibility, DPI, normal-user privileges
├── resources/Info.plist     # Mac application bundle metadata
├── scripts/build-macos.sh   # Apple compiler fallback when CMake is unavailable
├── CMakeLists.txt
├── README.md
├── VALIDATION.md
└── .gitignore
```

## Controls

| Feature | Windows | macOS |
| --- | --- | --- |
| Edit notes | Multiline text field, mouse/keyboard editing | Native text view; Edit menu includes undo/cut/copy/paste/select all |
| Always on top | Checkbox or **Ctrl+Shift+T** | Checkbox or **Cmd+Shift+T** |
| Capture exclusion | Checkbox or **Ctrl+Shift+E** | Unavailable; disabled control |
| Opacity | Slider, 30–100% | Slider, 30–100% |
| Resize | Drag native window edges | Drag native window edges |
| Minimize/close | Standard title-bar buttons | Standard title-bar buttons; Cmd+M / Cmd+W |

Shortcuts apply while this app has keyboard focus; they are not global hooks.
Always-on-top stays above ordinary windows; it does not override the secure
desktop or every application's special window level. Text is kept in memory
only and discarded on close. Capture exclusion and always-on-top start off;
opacity starts at 100%.

## Windows capture behavior

The capture checkbox calls:

```cpp
SetWindowDisplayAffinity(app.window, WDA_EXCLUDEFROMCAPTURE);
```

Turning it off calls the same API with `WDA_NONE`. Both operations check the
return value. On failure, `GetLastError()` is saved immediately, formatted into
a numeric error and system description, and displayed in a dialog and through
`OutputDebugStringW`. Attach Visual Studio's debugger and open **View → Output →
Debug** to read the log. Notes are not logged. A failed operation preserves the
last successful request state, including when disabling fails.

The UI's success message is exactly:

> Windows capture exclusion requested successfully.

It also displays that Microsoft Teams visibility has **NOT** been verified.
Success records API acceptance, not a remote capture measurement.

Microsoft documents support for `WDA_EXCLUDEFROMCAPTURE` beginning with Windows
10 version 2004 (build 19041). Earlier systems can degrade to `WDA_MONITOR`
behavior. This demo instead disables the toggle on older reported builds or
when version detection fails. The API depends on DWM composition, applies to
an owned top-level window, and offers no universal capture guarantee.
[Microsoft API documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity)

The app reports the Windows API major/minor/build using documented
`GetVersionExW`, with Windows 10 compatibility declared in its embedded manifest.
Windows 11 also reports API version 10.0. Compatibility-mode overrides can
affect this report; do not enable compatibility mode for this experiment.
This deprecated version API is used only to report/gate compatibility; it is
not an undocumented version query.
[Manifest and version reporting](https://learn.microsoft.com/en-us/windows/win32/sysinfo/targeting-your-application-at-windows-8-1)

SDK constants come from `<windows.h>`; the project does not redefine them. Only
the main window receives display affinity; error dialogs are separate windows.
Opacity uses documented `WS_EX_LAYERED`/`SetLayeredWindowAttributes` and affects
the entire window. Test capture at both 100% and reduced opacity because the
actual capture mechanism matters.

The app does not modify, inject into, or hook Teams or other processes. It
contains no DRM, driver manipulation, process tampering, or monitoring bypass.

## Building on Windows: prerequisites

Use a **Windows** computer or Windows VM with:

1. Visual Studio 2022 or its Build Tools, with **Desktop development with C++**.
2. MSVC x64/x86 build tools and a recent **Windows 10/11 SDK** (10.0.19041.0
   or newer) that declares `WDA_EXCLUDEFROMCAPTURE`.
3. **C++ CMake tools for Windows**, available through Visual Studio Installer,
   or separately installed CMake 3.20 or newer.

The shortest setup is Visual Studio Installer → Desktop development with C++ →
ensure the SDK and CMake components are selected → Install. Use
**Developer PowerShell for VS 2022** or **x64 Native Tools Command Prompt for
VS 2022** for the commands below.
[Official Visual Studio downloads](https://visualstudio.microsoft.com/downloads/)

The runtime is linked statically in MSVC builds. The resulting `.exe` runs
without a separate VC runtime installer, Python, or a server.

## Command line: build and run on Windows

Copy this project directory to the Windows machine. In Developer PowerShell,
change to that directory; for example, if copied to `C:\work\capture-overlay`:

```powershell
cd C:\work\capture-overlay
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
& .\build\Release\CaptureOverlay.exe
```

For that example, the **exact resulting executable** is:

```text
C:\work\capture-overlay\build\Release\CaptureOverlay.exe
```

Launch it later from any PowerShell directory with:

```powershell
& 'C:\work\capture-overlay\build\Release\CaptureOverlay.exe'
```

Print the absolute path for wherever you actually copied the project:

```powershell
(Resolve-Path .\build\Release\CaptureOverlay.exe).Path
```

The equivalent commands from an already-open project directory are:

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
& .\Release\CaptureOverlay.exe
```

With Ninja, output is instead `build\CaptureOverlay.exe` and configuration is
selected at configure time (`-DCMAKE_BUILD_TYPE=Release`). Do not mix generators
in the same build directory.

## Visual Studio: open, build, run

For a predictable output path, use a generated solution:

1. Run the configure command above in Developer PowerShell.
2. Open `build\CaptureOverlay.sln` in Visual Studio 2022.
3. Select **Release** and **x64** in the toolbar.
4. Right-click **CaptureOverlay** in Solution Explorer → **Set as Startup Project**.
5. Choose **Build → Build Solution** (Ctrl+Shift+B).
6. Choose **Debug → Start Without Debugging** (Ctrl+F5), or F5 to attach the debugger.

This produces `build\Release\CaptureOverlay.exe`. Debug builds produce
`build\Debug\CaptureOverlay.exe`.

You can also use **File → Open → Folder** and select `capture-overlay`; Visual
Studio recognizes `CMakeLists.txt`. Choose an x64 configuration, build, select
CaptureOverlay as the startup target, then launch. This folder workflow uses
Visual Studio's own configured build directory; the generated-solution workflow
above fixes the output location explicitly.

If the compiler reports that `WDA_EXCLUDEFROMCAPTURE` is missing, install/select
a newer SDK and configure a fresh build directory. If CMake cannot find MSVC,
install the C++ workload and use the developer shell. An Apple compiler cannot
build the Win32 target without a separate Windows toolchain and SDK; renaming
the Mac binary to `.exe` does not create a Windows executable.

## Build and run on macOS

Install Apple's Command Line Tools if needed:

```sh
xcode-select --install
```

The no-CMake fallback, used to build this project locally, is:

```sh
cd "/Users/divyanshraj/Documents/New project/capture-overlay"
sh scripts/build-macos.sh
open "$PWD/build-macos/CaptureOverlay.app"
```

The existing Mac app is:

```text
/Users/divyanshraj/Documents/New project/capture-overlay/build-macos/CaptureOverlay.app
```

Its native executable is:

```text
/Users/divyanshraj/Documents/New project/capture-overlay/build-macos/CaptureOverlay.app/Contents/MacOS/CaptureOverlay
```

Exact subsequent launch command on this machine:

```sh
open "/Users/divyanshraj/Documents/New project/capture-overlay/build-macos/CaptureOverlay.app"
```

If CMake is installed, this alternative uses the same source:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
open build/CaptureOverlay.app
```

That app path is for CMake's default single-configuration generator. The Xcode
generator places the app under `build/Release/` instead. The Mac bundle is
locally built and not notarized for distribution.

The Mac companion also supports a local smoke test:

```sh
./build-macos/CaptureOverlay.app/Contents/MacOS/CaptureOverlay --self-test
```

This tests Unicode editing, actual window level and alpha, and note-area resize.
`--self-test-window-actions` additionally waits for native minimize/restore
notifications, with a 10-second timeout. Test native minimize, restore, and close
on your normal desktop as well; the validation record distinguishes these
from the core smoke checks. Neither mode tests Windows capture exclusion.

## Testing with Microsoft Teams

Perform this experiment on **Windows 10 build 19041 or later / Windows 11**, with
two devices/accounts. Use obvious, non-sensitive test notes. A successful API
request is not a substitute for observing the receiving device.

1. Launch `CaptureOverlay.exe` on device A. Keep it visible and unminimized.
2. Enter obvious text such as `CAPTURE OVERLAY TEST 12345`.
3. Enable capture exclusion. Confirm the app says **Windows capture exclusion
   requested successfully**. If there is an error, record it and resolve it
   before interpreting this as an exclusion test.
4. Join a Teams meeting on device A and another device/account B.
5. On A, share the **entire Windows desktop/screen** that contains the overlay,
   rather than sharing one unrelated application window.
6. Observe the received shared screen on B. The local preview on A is not the
   experimental result.
7. Record whether B sees the overlay/text: absent, fully visible, blank region,
   or another outcome. Record the Windows build, Teams client/version, monitor,
   opacity, and always-on-top state. Wait for streaming frames to settle.
8. Disable capture exclusion and repeat with the same notes and settings as a
   **control**. Ensure the status shows OFF. The control should establish that
   the overlay is within the captured screen and otherwise visible.

Do not assume Teams uses a capture path that honors the request. Record what
the receiving device actually shows. Repeat at 100% and a lower opacity and
with always-on-top on/off if those settings will be used in practice.

### Independent Windows capture test

Before or after the meeting, use Windows Snipping Tool (Win+Shift+S) to capture
a region containing the visible overlay and surrounding desktop:

1. Capture with exclusion OFF. Open the resulting image and check whether the
   overlay appears; this is the control.
2. Enable exclusion and confirm API success. Capture the same region again and
   inspect the image.
3. Disable exclusion and repeat to establish reversibility. Optionally repeat
   with another ordinary Windows screenshot/recording mechanism and record
   which one was used.

Keep the overlay visible locally and ensure it overlaps the selected capture
region. An independently successful screenshot test does not prove Teams
behavior, and a Teams result does not characterize every capture mechanism.

Use this table for actual results (leave untested cells as **Not tested**):

| Windows build / Teams version / settings | Method | Exclusion OFF control | Exclusion ON observation | API accepted? |
| --- | --- | --- | --- | --- |
| Not tested | Snipping Tool | Not tested | Not tested | Not tested |
| Not tested | Teams receiver on device B | Not tested | Not tested | Not tested |

## Validation performed here

See [VALIDATION.md](VALIDATION.md). This development host is macOS, with Apple
Command Line Tools but no detected Windows compiler/SDK or CMake executable.
The Mac application was built and launched. **No Windows `.exe` has been
produced here; Windows compilation, display affinity, and Teams capture results
are unverified.** The Windows build commands above are the shortest continuation
on a Windows machine with the listed tools.
