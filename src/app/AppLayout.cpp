#include "AppLayoutInternal.h"

namespace
{
constexpr LONG_PTR kSplitBarDragging = 1;

LRESULT CALLBACK AppSplitBarWndProc(HWND, UINT, WPARAM, LPARAM);

VOID AppSplitBarOnPaint(HWND);
VOID AppSplitBarOnLButtonDown(HWND, BOOL, INT, INT, UINT);
VOID AppSplitBarOnMouseMove(HWND, INT, INT, UINT);
VOID AppSplitBarOnLButtonUp(HWND, INT, INT, UINT);

BOOL AppSplitBarIsDragging(HWND hWnd)
{
    return (GetWindowLongPtr(hWnd, GWLP_USERDATA) == kSplitBarDragging);
}

VOID AppSplitBarDraggingSet(HWND hWnd, BOOL bDragging)
{
    SetWindowLongPtr(hWnd, GWLP_USERDATA, bDragging ? kSplitBarDragging : 0);
}

RECT AppSplitBarRectGet(HWND hSplitWnd)
{
    RECT stWindowRect;
    RECT stRect{};
    POINT stPoint{};

    GetWindowRect(hSplitWnd, &stWindowRect);
    stPoint.x = stWindowRect.left;
    stPoint.y = stWindowRect.top;
    ScreenToClient(GetParent(hSplitWnd), &stPoint);

    SetRect(&stRect, stPoint.x, stPoint.y, kAppSplitBarWidth, stWindowRect.bottom - stWindowRect.top);

    return stRect;
}

LONG AppSplitBarLeftClamp(LONG dSplitBarLeft, LONG dClientWidth)
{
    if (dSplitBarLeft < kAppSplitBarLeftLimit)
        return kAppSplitBarLeftLimit;

    if (dSplitBarLeft > dClientWidth - kAppSplitBarLeftLimit)
        return dClientWidth - kAppSplitBarLeftLimit;

    return dSplitBarLeft;
}

APP_DOCK_LAYOUT AppDockLayoutFromSplitBarLeft(const RECT *pstFrame, LONG dSplitBarLeft)
{
    return AppDockLayoutCalc(pstFrame, pstFrame->right - dSplitBarLeft);
}

LRESULT CALLBACK AppSplitBarWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_PAINT, AppSplitBarOnPaint);
        HANDLE_MSG(hWnd, WM_LBUTTONDOWN, AppSplitBarOnLButtonDown);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, AppSplitBarOnMouseMove);
        HANDLE_MSG(hWnd, WM_LBUTTONUP, AppSplitBarOnLButtonUp);

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

VOID AppSplitBarOnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;

    BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
}

VOID AppSplitBarOnLButtonDown(HWND hWnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

    if (fDoubleClick)
        return;

    AppSplitBarDraggingSet(hWnd, TRUE);
    SetCapture(hWnd);
}

VOID AppSplitBarOnMouseMove(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    HWND hParentWnd;
    RECT stRect;
    LONG dClientWidth;
    LONG dSplitBarLeft;

    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

    if (!(AppSplitBarIsDragging(hWnd)))
        return;

    hParentWnd = GetParent(hWnd);
    GetClientRect(hParentWnd, &stRect);
    dClientWidth = stRect.right;

    stRect = AppSplitBarRectGet(hWnd);
    dSplitBarLeft = AppSplitBarLeftClamp(stRect.left + x, dClientWidth);

    SetWindowPos(hWnd, HWND_TOP, dSplitBarLeft, stRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

VOID AppSplitBarOnLButtonUp(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    HWND hParentWnd;

    UNREFERENCED_PARAMETER(keyFlags);

    if (!(AppSplitBarIsDragging(hWnd)))
        return;

    hParentWnd = GetParent(hWnd);

    ReleaseCapture();
    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#ifdef SPLIT_BAR_POS_FIX
    FORWARD_WM_SIZE(hParentWnd, kAppSplitBarMovedSize, x, y, PostMessage);
#else
    FORWARD_WM_SIZE(hParentWnd, SIZE_RESTORED, x, y, PostMessage);
#endif

    AppSplitBarDraggingSet(hWnd, FALSE);
}
}

LONG AppDockSplitPosClamp(LONG dSplitPos, const RECT *pstFrame, LONG dFallbackSplitPos)
{
    if (dSplitPos < kAppSplitBarWidth || pstFrame->right <= dSplitPos)
        return dFallbackSplitPos;

    return dSplitPos;
}

APP_DOCK_LAYOUT AppDockLayoutCalc(const RECT *pstFrame, LONG dSplitPos)
{
    APP_DOCK_LAYOUT stLayout{};

    stLayout.dSplitPos = dSplitPos;
    stLayout.dSplitBarLeft = pstFrame->right - dSplitPos;
    stLayout.dDockWidth = (dSplitPos > kAppSplitBarWidth) ? (dSplitPos - kAppSplitBarWidth) : 0;
    stLayout.stDockRect = *pstFrame;
    stLayout.stDockRect.right = stLayout.dDockWidth;
    stLayout.stDockRect.left = pstFrame->right - stLayout.dDockWidth;

    return stLayout;
}

LONG AppLayoutDockTabHeightGet(HWND hDockTabWnd)
{
    RECT stRect{};

    if (!(hDockTabWnd))
        return 0;

    GetClientRect(hDockTabWnd, &stRect);
    return stRect.bottom;
}

APP_DOCKED_WINDOW_LAYOUT AppLayoutDockedWindowsCalc(const RECT *pstFrame, LONG dSplitPos, LONG dDockTabHeight, BOOLEAN bPaletteAreaVisible)
{
    APP_DOCKED_WINDOW_LAYOUT stLayout{};
    LONG dHalfHeight;

    stLayout.stDockLayout = AppDockLayoutCalc(pstFrame, dSplitPos);
    stLayout.stPageListRect = stLayout.stDockLayout.stDockRect;
    stLayout.stDockTabRect = stLayout.stDockLayout.stDockRect;
    stLayout.stPaletteRect = stLayout.stDockLayout.stDockRect;
    stLayout.stBucketRect = stLayout.stDockLayout.stDockRect;
    stLayout.bPaletteAreaVisible = bPaletteAreaVisible;

    if (bPaletteAreaVisible)
    {
        dHalfHeight = stLayout.stDockLayout.stDockRect.bottom >> 1;

        stLayout.stPageListRect.bottom = dHalfHeight;

        stLayout.stDockTabRect.top += dHalfHeight;
        stLayout.stDockTabRect.bottom = dDockTabHeight;

        stLayout.stPaletteRect.top += dHalfHeight + dDockTabHeight;
        stLayout.stPaletteRect.bottom = dHalfHeight - dDockTabHeight;
        stLayout.stBucketRect = stLayout.stPaletteRect;
    }
    else
    {
        stLayout.stDockTabRect.top += stLayout.stDockLayout.stDockRect.bottom - dDockTabHeight;
        stLayout.stDockTabRect.bottom = dDockTabHeight;
        stLayout.stPageListRect.bottom -= dDockTabHeight;

        stLayout.stPaletteRect.top = stLayout.stDockTabRect.top;
        stLayout.stPaletteRect.bottom = 0;
        stLayout.stBucketRect = stLayout.stPaletteRect;
    }

    return stLayout;
}

VOID AppLayoutApplyDockedWindows(const APP_DOCKED_WINDOW_LAYOUT &stLayout, HWND hPageWnd, HWND hDockTabWnd, HWND hInsertWnd, HWND hBucketWnd)
{
    if (hPageWnd)
    {
        SetWindowPos(hPageWnd, HWND_TOP, stLayout.stPageListRect.left, stLayout.stPageListRect.top,
                     stLayout.stPageListRect.right, stLayout.stPageListRect.bottom, SWP_SHOWWINDOW);
    }

    if (hDockTabWnd)
    {
        MoveWindow(hDockTabWnd, stLayout.stDockTabRect.left, stLayout.stDockTabRect.top,
                   stLayout.stDockTabRect.right, stLayout.stDockTabRect.bottom, TRUE);
    }

    if (!(stLayout.bPaletteAreaVisible))
        return;

    if (hInsertWnd)
    {
        SetWindowPos(hInsertWnd, HWND_TOP, stLayout.stPaletteRect.left, stLayout.stPaletteRect.top,
                     stLayout.stPaletteRect.right, stLayout.stPaletteRect.bottom, SWP_NOZORDER);
    }

    if (hBucketWnd)
    {
        SetWindowPos(hBucketWnd, HWND_TOP, stLayout.stBucketRect.left, stLayout.stBucketRect.top,
                     stLayout.stBucketRect.right, stLayout.stBucketRect.bottom, SWP_NOZORDER);
    }
}

ATOM AppLayoutSplitBarClassRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = AppSplitBarWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_SIZEWE);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = kAppSplitBarClass;
    wcex.hIconSm = nullptr;

    return RegisterClassEx(&wcex);
}

HWND AppLayoutSplitBarCreate(HINSTANCE hInstance, HWND hParentWnd, const RECT *pstFrame, LONG dSplitPos)
{
    const APP_DOCK_LAYOUT stLayout = AppDockLayoutCalc(pstFrame, dSplitPos);
    HWND hSplitWnd;

    hSplitWnd = CreateWindowEx(WS_EX_WINDOWEDGE, kAppSplitBarClass, TEXT("SplitBar"),
                               WS_CHILD | WS_VISIBLE, stLayout.dSplitBarLeft, pstFrame->top, kAppSplitBarWidth, pstFrame->bottom,
                               hParentWnd, nullptr, hInstance, nullptr);
    AppSplitBarDraggingSet(hSplitWnd, FALSE);

    return hSplitWnd;
}

APP_DOCK_LAYOUT AppLayoutSplitBarResize(HWND hSplitWnd, const RECT *pstFrame)
{
    const RECT stSplitBarRect = AppSplitBarRectGet(hSplitWnd);
    const APP_DOCK_LAYOUT stLayout = AppDockLayoutFromSplitBarLeft(pstFrame, stSplitBarRect.left);

    SetWindowPos(hSplitWnd, HWND_TOP, stSplitBarRect.left, pstFrame->top, kAppSplitBarWidth, pstFrame->bottom, 0);

    InvalidateRect(hSplitWnd, nullptr, TRUE);
    UpdateWindow(hSplitWnd);

    return stLayout;
}

VOID AppLayoutSplitBarSync(HWND hSplitWnd, const RECT *pstFrame, LONG dSplitPos)
{
    const APP_DOCK_LAYOUT stLayout = AppDockLayoutCalc(pstFrame, dSplitPos);

    SetWindowPos(hSplitWnd, HWND_TOP, stLayout.dSplitBarLeft, pstFrame->top, 0, 0, SWP_NOSIZE);
}

VOID AppLayoutSyncMainSplitBar(HWND hSplitWnd, LONG dSplitPos)
{
    RECT stRect;

    if (!(hSplitWnd))
        return;

    AppClientAreaCalc(&stRect);
    AppLayoutSplitBarSync(hSplitWnd, &stRect, dSplitPos);
}
