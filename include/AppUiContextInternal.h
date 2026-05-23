#pragma once

#include "Sunday.h"

extern HINSTANCE ghAppInst;
extern HWND ghFileTabWnd;
extern HWND ghMainWnd;
extern HWND ghViewWnd;
extern HWND ghPgVwWnd;
extern HWND ghPalInsertWnd;
extern HWND ghPalBucketWnd;
extern HWND ghMainSplitWnd;
extern HMENU ghMainMenu;
extern HACCEL ghMainAccelTable;
extern HWND ghMainStatusBarWnd;
extern HBRUSH ghMainStatusRedBrush;
extern INT gbTmpltDock;
extern LONG grdSplitPos;
extern UINT gdGridXpos;
extern UINT gdGridYpos;
extern UINT gdRightRuler;
extern UINT gdUnderRuler;

// 앱 UI 상태: 메인 프레임과 보조 UI 핸들/리소스 접근점
struct AppUiContext
{
    HINSTANCE hInstance{};
    HWND hMainWindow{};
    HWND hFileTabWindow{};
    HWND hPageListWindow{};
    HWND hInsertPaletteWindow{};
    HWND hBucketPaletteWindow{};
    INT bTemplateDocked{};
    HMENU hMainMenu{};
    HACCEL hMainAccelTable{};
    HWND hMainStatusBarWindow{};
    HBRUSH hMainStatusWarningBrush{};
};

// 뷰 상태: 편집/보조 뷰 핸들과 레이아웃 파라미터 접근점
struct AppViewContext
{
    HWND hEditViewWindow{};
    HWND hPagePreviewWindow{};
    HWND hMainSplitWindow{};
    LONG dSplitPos{};
    UINT dGridXpos{};
    UINT dGridYpos{};
    UINT dRightRuler{};
    UINT dUnderRuler{};
};

inline AppUiContext AppUiContextGet()
{
    AppUiContext stContext{};

    stContext.hInstance = ghAppInst;
    stContext.hMainWindow = ghMainWnd;
    stContext.hFileTabWindow = ghFileTabWnd;
    stContext.hPageListWindow = ghPgVwWnd;
    stContext.hInsertPaletteWindow = ghPalInsertWnd;
    stContext.hBucketPaletteWindow = ghPalBucketWnd;
    stContext.bTemplateDocked = gbTmpltDock;
    stContext.hMainMenu = ghMainMenu;
    stContext.hMainAccelTable = ghMainAccelTable;
    stContext.hMainStatusBarWindow = ghMainStatusBarWnd;
    stContext.hMainStatusWarningBrush = ghMainStatusRedBrush;

    return stContext;
}

inline AppViewContext AppViewContextGet()
{
    AppViewContext stContext{};

    stContext.hEditViewWindow = ghViewWnd;
    stContext.hPagePreviewWindow = ghPgVwWnd;
    stContext.hMainSplitWindow = ghMainSplitWnd;
    stContext.dSplitPos = grdSplitPos;
    stContext.dGridXpos = gdGridXpos;
    stContext.dGridYpos = gdGridYpos;
    stContext.dRightRuler = gdRightRuler;
    stContext.dUnderRuler = gdUnderRuler;

    return stContext;
}

inline HWND AppMainWindowGet()
{
    return AppUiContextGet().hMainWindow;
}

inline HWND AppFileTabWindowGet()
{
    return AppUiContextGet().hFileTabWindow;
}

inline HWND AppPageListWindowGet()
{
    return AppUiContextGet().hPageListWindow;
}
