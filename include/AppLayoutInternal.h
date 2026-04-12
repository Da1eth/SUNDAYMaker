#pragma once

#include "Sunday.h"

inline constexpr LPCTSTR kAppSplitBarClass = TEXT("CSplitBar");
inline constexpr INT kAppSplitBarWidth = 4;
inline constexpr INT kAppSplitBarLeftLimit = 120;
inline constexpr UINT kAppSplitBarMovedSize = 0xFFFF;

struct APP_DOCK_LAYOUT
{
    LONG dSplitPos{};
    LONG dSplitBarLeft{};
    LONG dDockWidth{};
    RECT stDockRect{};
};

struct APP_DOCKED_WINDOW_LAYOUT
{
    APP_DOCK_LAYOUT stDockLayout{};
    RECT stPageListRect{};
    RECT stDockTabRect{};
    RECT stPaletteRect{};
    RECT stBucketRect{};
    BOOLEAN bPaletteAreaVisible{};
};

LONG AppDockSplitPosClamp(LONG dSplitPos, const RECT *pstFrame, LONG dFallbackSplitPos);
APP_DOCK_LAYOUT AppDockLayoutCalc(const RECT *pstFrame, LONG dSplitPos);
LONG AppLayoutDockTabHeightGet(HWND hDockTabWnd);
APP_DOCKED_WINDOW_LAYOUT AppLayoutDockedWindowsCalc(const RECT *pstFrame, LONG dSplitPos, LONG dDockTabHeight, BOOLEAN bPaletteAreaVisible);
VOID AppLayoutApplyDockedWindows(const APP_DOCKED_WINDOW_LAYOUT &stLayout, HWND hPageWnd, HWND hDockTabWnd, HWND hInsertWnd, HWND hBucketWnd);
ATOM AppLayoutSplitBarClassRegister(HINSTANCE hInstance);
HWND AppLayoutSplitBarCreate(HINSTANCE hInstance, HWND hParentWnd, const RECT *pstFrame, LONG dSplitPos);
APP_DOCK_LAYOUT AppLayoutSplitBarResize(HWND hSplitWnd, const RECT *pstFrame);
VOID AppLayoutSplitBarSync(HWND hSplitWnd, const RECT *pstFrame, LONG dSplitPos);
VOID AppLayoutSyncMainSplitBar(HWND hSplitWnd, LONG dSplitPos);
