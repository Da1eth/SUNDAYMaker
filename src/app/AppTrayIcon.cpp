#include "Sunday.h"
#include "AppModuleInternal.h"

namespace
{
constexpr UINT kTrayIconId = 1;
constexpr UINT kBalloonTimeoutMs = 10'000;
constexpr UINT kTrayCommandRestore = 0x1001;
constexpr UINT kTrayCommandMinimise = 0x1002;
constexpr UINT kTrayCommandExit = 0x1003;

UINT guTaskbarCreatedMessage = 0;
bool gbTrayIconAdded = false;
bool gbWindowHiddenToTray = false;
bool gbWindowWasMaximised = false;
bool gbPageWindowVisible = false;
bool gbInsertPaletteVisible = false;
bool gbBucketPaletteVisible = false;
HWND ghTrayOwnerWnd = nullptr;

HICON AppTrayIconHandleGet()
{
    static HICON hTrayIcon = LoadIcon(ghAppInst, MAKEINTRESOURCE(IDI_SMALL));
    return hTrayIcon;
}

NOTIFYICONDATA AppTrayIconDataCreate(HWND hWnd)
{
    NOTIFYICONDATA stData{};

    stData.cbSize = sizeof(stData);
    stData.hWnd = hWnd;
    stData.uID = kTrayIconId;
    stData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    stData.uCallbackMessage = WMP_TRAYICON;
    stData.hIcon = AppTrayIconHandleGet();
    StringCchCopy(stData.szTip, ARRAYSIZE(stData.szTip), gatAppTitle);

    return stData;
}

HRESULT AppTrayIconVersionSet(HWND hWnd)
{
    auto stData = AppTrayIconDataCreate(hWnd);
    stData.uVersion = NOTIFYICON_VERSION;

    if (Shell_NotifyIcon(NIM_SETVERSION, &stData))
    {
        return S_OK;
    }

    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT AppTrayIconAddOrRefresh(HWND hWnd)
{
    auto stData = AppTrayIconDataCreate(hWnd);
    const DWORD dMessage = gbTrayIconAdded ? NIM_MODIFY : NIM_ADD;

    if (!Shell_NotifyIcon(dMessage, &stData))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ghTrayOwnerWnd = hWnd;
    gbTrayIconAdded = true;

    return AppTrayIconVersionSet(hWnd);
}

VOID AppTrayIconUndockedStateRemember()
{
    gbPageWindowVisible = ghPgVwWnd && IsWindowVisible(ghPgVwWnd);
    gbInsertPaletteVisible = ghPalInsertWnd && IsWindowVisible(ghPalInsertWnd);
    gbBucketPaletteVisible = ghPalBucketWnd && IsWindowVisible(ghPalBucketWnd);
}

VOID AppTrayIconUndockedWindowsRestore()
{
    if (gbTmpltDock)
    {
        return;
    }

    if (ghPgVwWnd && gbPageWindowVisible)
    {
        ShowWindow(ghPgVwWnd, SW_SHOW);
    }

    if (ghPalInsertWnd && gbInsertPaletteVisible)
    {
        ShowWindow(ghPalInsertWnd, SW_SHOW);
    }

    if (ghPalBucketWnd && gbBucketPaletteVisible)
    {
        ShowWindow(ghPalBucketWnd, SW_SHOW);
    }
}

VOID AppTrayIconHideToTray(HWND hWnd)
{
    if (!(hWnd) || gbWindowHiddenToTray)
    {
        return;
    }

    gbWindowWasMaximised = IsZoomed(hWnd) != FALSE;
    AppTrayIconUndockedStateRemember();

    if (!(gbTmpltDock))
    {
        if (ghPgVwWnd)
        {
            ShowWindow(ghPgVwWnd, SW_HIDE);
        }
        if (ghPalInsertWnd)
        {
            ShowWindow(ghPalInsertWnd, SW_HIDE);
        }
        if (ghPalBucketWnd)
        {
            ShowWindow(ghPalBucketWnd, SW_HIDE);
        }
    }

    ShowWindow(hWnd, SW_HIDE);
    gbWindowHiddenToTray = true;
}

VOID AppTrayIconActivate(HWND hWnd)
{
    if (!(hWnd))
    {
        return;
    }

    if (gbWindowHiddenToTray)
    {
        ShowWindow(hWnd, gbWindowWasMaximised ? SW_SHOWMAXIMIZED : SW_SHOW);
        AppTrayIconUndockedWindowsRestore();
        gbWindowHiddenToTray = false;
    }
    else if (IsIconic(hWnd))
    {
        ShowWindow(hWnd, SW_RESTORE);
    }
    else
    {
        ShowWindow(hWnd, SW_SHOW);
    }

    SetForegroundWindow(hWnd);

    if (ghViewWnd)
    {
        SetFocus(ghViewWnd);
    }
}

UINT AppTrayIconPopupMenuTrack(HWND hWnd)
{
    POINT stPoint{};
    HMENU hPopupMenu = CreatePopupMenu();
    UINT dCommand = 0;

    if (!(hPopupMenu))
    {
        return 0;
    }

    AppendMenu(hPopupMenu, MF_STRING, gbWindowHiddenToTray ? kTrayCommandRestore : kTrayCommandMinimise,
               gbWindowHiddenToTray ? TEXT("창 복구") : TEXT("트레이로 최소화"));
    AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hPopupMenu, MF_STRING, kTrayCommandExit, TEXT("종료"));

    GetCursorPos(&stPoint);
    SetForegroundWindow(hWnd);
    dCommand = TrackPopupMenu(hPopupMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, stPoint.x, stPoint.y, 0, hWnd, nullptr);
    PostMessage(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hPopupMenu);
    return dCommand;
}

BOOLEAN AppTrayIconPopupMenuExecute(HWND hWnd, UINT dCommand)
{
    switch (dCommand)
    {
    case kTrayCommandRestore:
        AppTrayIconActivate(hWnd);
        return TRUE;

    case kTrayCommandMinimise:
        AppTrayIconHideToTray(hWnd);
        return TRUE;

    case kTrayCommandExit:
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        return TRUE;

    default:
        return FALSE;
    }
}
}

HRESULT AppTrayIconInitialise(HWND hWnd)
{
    if (!(hWnd))
    {
        return E_INVALIDARG;
    }

    if (0 == guTaskbarCreatedMessage)
    {
        guTaskbarCreatedMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));
    }

    return AppTrayIconAddOrRefresh(hWnd);
}

VOID AppTrayIconFinalise(VOID)
{
    if (!(gbTrayIconAdded) || !(ghTrayOwnerWnd))
    {
        return;
    }

    auto stData = AppTrayIconDataCreate(ghTrayOwnerWnd);
    Shell_NotifyIcon(NIM_DELETE, &stData);

    ghTrayOwnerWnd = nullptr;
    gbTrayIconAdded = false;
}

BOOLEAN AppTrayIconHandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    if (plResult == nullptr)
    {
        return FALSE;
    }

    if ((0 != guTaskbarCreatedMessage) && (guTaskbarCreatedMessage == message))
    {
        AppTrayIconAddOrRefresh(hWnd);
        *plResult = 0;
        return TRUE;
    }

    if ((WMP_TRAYICON != message) || (kTrayIconId != static_cast<UINT>(wParam)))
    {
        return FALSE;
    }

    switch (static_cast<UINT>(lParam))
    {
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case NIN_BALLOONUSERCLICK:
        AppTrayIconActivate(hWnd);
        *plResult = 0;
        return TRUE;

    case WM_CONTEXTMENU:
        AppTrayIconPopupMenuExecute(hWnd, AppTrayIconPopupMenuTrack(hWnd));
        *plResult = 0;
        return TRUE;

    default:
        return FALSE;
    }
}

HRESULT NotifyBalloonExist(LPCTSTR ptInfo, LPCTSTR ptTitle, DWORD dIconTy)
{
    if (!(ghMainWnd))
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = AppTrayIconInitialise(ghMainWnd);
    if (FAILED(hr))
    {
        return hr;
    }

    auto stData = AppTrayIconDataCreate(ghMainWnd);
    stData.uFlags = NIF_INFO;
    stData.dwInfoFlags = dIconTy;
    stData.uTimeout = kBalloonTimeoutMs;
    StringCchCopy(stData.szInfo, ARRAYSIZE(stData.szInfo), ptInfo ? ptInfo : TEXT(""));
    StringCchCopy(stData.szInfoTitle, ARRAYSIZE(stData.szInfoTitle), ptTitle ? ptTitle : TEXT(""));

    if (Shell_NotifyIcon(NIM_MODIFY, &stData))
    {
        return S_OK;
    }

    return HRESULT_FROM_WIN32(GetLastError());
}