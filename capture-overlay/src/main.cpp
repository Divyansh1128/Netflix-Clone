#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <string>

#ifndef WDA_EXCLUDEFROMCAPTURE
#error Install a recent Windows 10/11 SDK that defines WDA_EXCLUDEFROMCAPTURE.
#endif

namespace {
constexpr wchar_t kClassName[] = L"CaptureOverlay.Demo.Window";
enum : int { Topmost = 101, Exclusion, Opacity, Notes, Status, Platform, OpacityLabel };

struct AppState {
    HWND window{}, topmost{}, exclusion{}, opacity{}, notes{}, status{}, platform{}, opacityLabel{};
    HFONT uiFont{}, notesFont{};
    UINT dpi = 96;
    bool onTop = false;
    bool exclusionAccepted = false; // Last successful SetWindowDisplayAffinity request.
    bool exclusionSupported = false;
    int opacityPercent = 100;
    std::wstring lastError;
} app;

int Px(int logical) { return MulDiv(logical, static_cast<int>(app.dpi), 96); }

UINT WindowDpi(HWND window) {
    // Optional documented API: early Windows 10 releases do not export it.
    using DpiFunction = UINT(WINAPI*)(HWND);
    auto getDpi = reinterpret_cast<DpiFunction>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (getDpi) return getDpi(window);
    HDC dc = GetDC(window);
    if (!dc) return 96;
    const UINT dpi = static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSX));
    ReleaseDC(window, dc);
    return dpi;
}

std::wstring ErrorText(DWORD code) {
    wchar_t* buffer = nullptr;
    const DWORD size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code,
        0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring text = size && buffer ? std::wstring(buffer, size) : L"No system error description available.";
    if (buffer) LocalFree(buffer);
    while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n')) text.pop_back();
    return L"Error " + std::to_wstring(code) + L": " + text;
}

void UpdateStatus() {
    std::wstring message;
    if (!app.exclusionSupported) {
        message = L"Capture exclusion unavailable: requires Windows 10 2004 (build 19041) or later.";
    } else if (app.exclusionAccepted) {
        message = L"Windows capture exclusion requested successfully.\r\nMicrosoft Teams visibility has NOT been verified.";
    } else {
        message = L"Capture exclusion OFF (WDA_NONE).\r\nMicrosoft Teams visibility has NOT been verified.";
    }
    if (!app.lastError.empty()) message += L"\r\nLast action failed; see error dialog / debugger log.";
    SetWindowTextW(app.status, message.c_str());
    SendMessageW(app.exclusion, BM_SETCHECK, app.exclusionAccepted ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(app.topmost, BM_SETCHECK, app.onTop ? BST_CHECKED : BST_UNCHECKED, 0);
    InvalidateRect(app.status, nullptr, TRUE);
}

void ReportError(const wchar_t* operation, DWORD code) {
    app.lastError = std::wstring(operation) + L" failed.\r\n" + ErrorText(code);
    OutputDebugStringW((app.lastError + L"\r\n").c_str());
    UpdateStatus();
    MessageBoxW(app.window, app.lastError.c_str(), L"Capture Overlay — API error", MB_OK | MB_ICONERROR);
}

void ToggleTopmost() {
    const bool requested = !app.onTop;
    if (!SetWindowPos(app.window, requested ? HWND_TOPMOST : HWND_NOTOPMOST,
                      0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)) {
        const DWORD error = GetLastError();
        ReportError(L"SetWindowPos", error);
        return;
    }
    app.onTop = requested;
    app.lastError.clear();
    UpdateStatus();
}

void ToggleExclusion() {
    if (!app.exclusionSupported) return;
    const bool requested = !app.exclusionAccepted;
    const DWORD affinity = requested ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
    // This is our own process's top-level HWND; never a child EDIT or another app.
    if (!SetWindowDisplayAffinity(app.window, affinity)) {
        const DWORD error = GetLastError(); // Save immediately, before another API call.
        ReportError(requested ? L"SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE)"
                              : L"SetWindowDisplayAffinity(WDA_NONE)", error);
        return; // Preserve the last successful request, including a failed disable.
    }
    app.exclusionAccepted = requested;
    app.lastError.clear();
    OutputDebugStringW(requested
        ? L"Capture Overlay: WDA_EXCLUDEFROMCAPTURE accepted; remote visibility unverified.\r\n"
        : L"Capture Overlay: WDA_NONE accepted.\r\n");
    UpdateStatus();
}

void SetOpacity() {
    const int requested = static_cast<int>(SendMessageW(app.opacity, TBM_GETPOS, 0, 0));
    const BYTE alpha = static_cast<BYTE>(MulDiv(requested, 255, 100));
    if (!SetLayeredWindowAttributes(app.window, 0, alpha, LWA_ALPHA)) {
        const DWORD error = GetLastError();
        SendMessageW(app.opacity, TBM_SETPOS, TRUE, app.opacityPercent);
        ReportError(L"SetLayeredWindowAttributes", error);
        return;
    }
    app.opacityPercent = requested;
    SetWindowTextW(app.opacityLabel, (L"Opacity: " + std::to_wstring(requested) + L"%").c_str());
}

void SetFonts() {
    HFONT oldUI = app.uiFont, oldNotes = app.notesFont;
    app.uiFont = CreateFontW(-MulDiv(10, static_cast<int>(app.dpi), 72), 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    app.notesFont = CreateFontW(-MulDiv(18, static_cast<int>(app.dpi), 72), 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    for (HWND control : { app.topmost, app.exclusion, app.opacityLabel, app.status, app.platform })
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(app.uiFont), TRUE);
    SendMessageW(app.notes, WM_SETFONT, reinterpret_cast<WPARAM>(app.notesFont), TRUE);
    if (oldUI) DeleteObject(oldUI);
    if (oldNotes) DeleteObject(oldNotes);
}

void Layout() {
    RECT rect{};
    GetClientRect(app.window, &rect);
    const int margin = Px(12), width = std::max(0, static_cast<int>(rect.right) - 2 * margin);
    MoveWindow(app.topmost, margin, Px(10), width, Px(26), TRUE);
    MoveWindow(app.exclusion, margin, Px(40), width, Px(26), TRUE);
    MoveWindow(app.opacityLabel, margin, Px(80), Px(110), Px(24), TRUE);
    MoveWindow(app.opacity, Px(126), Px(74), std::max(0, width - Px(114)), Px(32), TRUE);
    MoveWindow(app.status, margin, Px(114), width, Px(64), TRUE);
    MoveWindow(app.notes, margin, Px(190), width,
               std::max(Px(40), static_cast<int>(rect.bottom) - Px(244)), TRUE);
    MoveWindow(app.platform, margin, static_cast<int>(rect.bottom) - Px(44), width, Px(36), TRUE);
}

void DetectWindows() {
    OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    // Documented, manifest-sensitive API. Used only for the build compatibility gate.
    // windows.manifest declares Windows 10 support; Windows 11 also reports 10.0.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
    const BOOL known = GetVersionExW(&version);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    app.exclusionSupported = known && (version.dwMajorVersion > 10 ||
        (version.dwMajorVersion == 10 && (version.dwMinorVersion > 0 || version.dwBuildNumber >= 19041)));
    std::wstring text = known ? L"Windows API version " + std::to_wstring(version.dwMajorVersion) + L"." +
        std::to_wstring(version.dwMinorVersion) + L", build " + std::to_wstring(version.dwBuildNumber)
        : L"Windows version could not be determined; exclusion disabled.";
    text += L"\r\nNotes are temporary and discarded when this window closes.";
    SetWindowTextW(app.platform, text.c_str());
    OutputDebugStringW((text + L"\r\n").c_str());
    EnableWindow(app.exclusion, app.exclusionSupported);
}

HWND Control(const wchar_t* type, const wchar_t* text, DWORD style, int id, DWORD extended = 0) {
    return CreateWindowExW(extended, type, text, WS_CHILD | WS_VISIBLE | style,
        0, 0, 0, 0, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        app.window = window;
        app.dpi = WindowDpi(window);
        app.topmost = Control(L"BUTTON", L"Always on top  (Ctrl+Shift+T)", BS_CHECKBOX | WS_TABSTOP, Topmost);
        app.exclusion = Control(L"BUTTON", L"Request Windows capture exclusion  (Ctrl+Shift+E)", BS_CHECKBOX | WS_TABSTOP, Exclusion);
        app.opacityLabel = Control(L"STATIC", L"Opacity: 100%", SS_NOPREFIX, OpacityLabel);
        app.opacity = Control(TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, Opacity);
        app.status = Control(L"STATIC", L"", SS_LEFT | SS_NOPREFIX, Status);
        app.notes = Control(L"EDIT", L"Type your test notes here…\r\n\r\nCAPTURE OVERLAY TEST 12345",
            ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL | WS_TABSTOP, Notes, WS_EX_CLIENTEDGE);
        app.platform = Control(L"STATIC", L"", SS_LEFT | SS_NOPREFIX, Platform);
        if (!app.topmost || !app.exclusion || !app.opacity || !app.opacityLabel || !app.status || !app.notes || !app.platform) {
            const DWORD error = GetLastError();
            MessageBoxW(window, ErrorText(error).c_str(), L"Unable to create controls", MB_OK | MB_ICONERROR);
            return -1;
        }
        SendMessageW(app.opacity, TBM_SETRANGE, TRUE, MAKELPARAM(30, 100));
        SendMessageW(app.opacity, TBM_SETPOS, TRUE, 100);
        SendMessageW(app.opacity, TBM_SETTICFREQ, 10, 0);
        SendMessageW(app.notes, EM_SETLIMITTEXT, 1024 * 1024, 0);
        SetFonts();
        DetectWindows();
        UpdateStatus();
        Layout();
        if (!SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA)) {
            const DWORD error = GetLastError();
            ReportError(L"SetLayeredWindowAttributes (initialization)", error);
        }
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == Topmost) { ToggleTopmost(); return 0; }
        if (LOWORD(wParam) == Exclusion) { ToggleExclusion(); return 0; }
        break;
    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == app.opacity) { SetOpacity(); return 0; }
        break;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) Layout();
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize = { Px(560), Px(410) };
        return 0;
    }
    case WM_DPICHANGED: {
        app.dpi = HIWORD(wParam);
        SetFonts();
        const auto* rect = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(window, nullptr, rect->left, rect->top, rect->right - rect->left,
            rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        Layout();
        return 0;
    }
    case WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) == app.status) {
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetTextColor(dc, !app.lastError.empty() || !app.exclusionSupported ? RGB(150, 70, 0)
                : app.exclusionAccepted ? RGB(0, 110, 45) : RGB(45, 65, 100));
            SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }
        break;
    case WM_SETFOCUS:
        SetFocus(app.notes);
        return 0;
    case WM_DESTROY:
        if (app.uiFont) DeleteObject(app.uiFont);
        if (app.notesFont) DeleteObject(app.notesFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    INITCOMMONCONTROLSEX controls{ sizeof(controls), ICC_BAR_CLASSES };
    if (!InitCommonControlsEx(&controls)) {
        const DWORD error = GetLastError();
        MessageBoxW(nullptr, ErrorText(error).c_str(), L"Common control initialization failed", MB_OK | MB_ICONERROR);
        return 1;
    }
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    windowClass.lpszClassName = kClassName;
    if (!RegisterClassExW(&windowClass)) {
        const DWORD error = GetLastError();
        MessageBoxW(nullptr, ErrorText(error).c_str(), L"Window registration failed", MB_OK | MB_ICONERROR);
        return 1;
    }
    HWND window = CreateWindowExW(WS_EX_LAYERED, kClassName, L"Capture Overlay — Windows API demonstration",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 650, 520,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 1;
    RECT initial{ 0, 0, Px(620), Px(460) };
    AdjustWindowRectEx(&initial, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, FALSE, WS_EX_LAYERED);
    if (!SetWindowPos(window, nullptr, 0, 0, initial.right - initial.left,
                      initial.bottom - initial.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) {
        const DWORD error = GetLastError();
        ReportError(L"SetWindowPos (initial size)", error);
    }
    ACCEL shortcuts[] = {
        { FVIRTKEY | FCONTROL | FSHIFT, 'T', Topmost },
        { FVIRTKEY | FCONTROL | FSHIFT, 'E', Exclusion }
    };
    HACCEL accelerators = CreateAcceleratorTableW(shortcuts, 2);
    if (!accelerators) {
        const DWORD error = GetLastError();
        ReportError(L"CreateAcceleratorTable", error);
        DestroyWindow(window);
        return 1;
    }
    ShowWindow(window, show);
    UpdateWindow(window);
    MSG message{};
    BOOL result;
    while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (!TranslateAcceleratorW(window, accelerators, &message) && !IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (result == -1) {
        const DWORD error = GetLastError();
        OutputDebugStringW((L"GetMessage failed: " + ErrorText(error)).c_str());
    }
    DestroyAcceleratorTable(accelerators);
    return result == -1 ? 1 : static_cast<int>(message.wParam);
}
