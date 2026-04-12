#include "Sunday.h"
#include "Palette.h"
#include "AppModuleInternal.h"

namespace
{
constexpr INT kStatusBarItems = 8;
CONST INT kStatusBarSizes[] = {50, 295, 475, 675, 825, 950, 1075, -1};

HWND ghFileTabTip;
INT giResizeWide;

VOID AppActivateAuxWindow(HWND hWnd, HWND hTargetWnd, BOOLEAN bVisibleOnly)
{
    LONG_PTR rdExStyle;
    HWND hInsertAfter;

    if (!(hTargetWnd) || (bVisibleOnly && !(IsWindowVisible(hTargetWnd))))
        return;

    rdExStyle = GetWindowLongPtr(hTargetWnd, GWL_EXSTYLE);
    hInsertAfter = (WS_EX_TOPMOST & rdExStyle) ? HWND_TOPMOST : hWnd;

    SetWindowPos(hTargetWnd, hInsertAfter, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

VOID AppSetUndockedAuxVisibility(INT nCmdShow)
{
    if (gbTmpltDock)
        return;

    ShowWindow(ghPgVwWnd, nCmdShow);
    ShowWindow(ghPalInsertWnd, nCmdShow);
    ShowWindow(ghPalBucketWnd, nCmdShow);
}

VOID AppSyncSplitBarWithClient(VOID)
{
#ifndef SPLIT_BAR_POS_FIX
    AppLayoutSyncMainSplitBar(ghMainSplitWnd, grdSplitPos);
#endif
}

VOID AppPersistWindowRect(HWND hWnd, UINT dWindowPos)
{
    RECT rect;

    GetWindowRect(hWnd, &rect);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    InitWindowPos(INIT_SAVE, dWindowPos, &rect);
}

VOID AppPersistWindowState(HWND hWnd)
{
    const DWORD dwStyle = GetWindowStyle(hWnd);

    if (dwStyle & WS_MINIMIZE)
        return;

    if (dwStyle & WS_MAXIMIZE)
    {
        InitParamValue(INIT_SAVE, VL_MAXIMISED, 1);
    }
    else
    {
        AppPersistWindowRect(ghMainWnd, WDP_MVIEW);
        InitParamValue(INIT_SAVE, VL_MAXIMISED, 0);
    }

    if (!(gbTmpltDock))
    {
        AppPersistWindowRect(ghPgVwWnd, WDP_PLIST);
        AppPersistWindowRect(ghPalInsertWnd, WDP_LNTMPL);
        AppPersistWindowRect(ghPalBucketWnd, WDP_BRTMPL);
    }
}
}

VOID Cls_OnActivate(HWND hWnd, UINT state, HWND hWndActDeact, BOOL fMinimized)
{
    TRACE(TEXT("MAIN ACTIVATE STATE[%u] HWND[%X] MIN[%u]"), state, hWndActDeact, fMinimized);

    if (WA_INACTIVE != state)
    {
        AppActivateAuxWindow(hWnd, ghPgVwWnd, FALSE);
        AppActivateAuxWindow(hWnd, ghPalInsertWnd, TRUE);
        AppActivateAuxWindow(hWnd, ghPalBucketWnd, TRUE);

        ViewFocusSet();
    }
}

BOOLEAN Cls_OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    RECT clRect, tbRect;
    RECT tiRect;
    TCITEM stTcItem;
    TTTOOLINFO stToolInfo;
    HINSTANCE lcInst = lpCreateStruct->hInstance;

    DragAcceptFiles(hWnd, TRUE);

    GetClientRect(hWnd, &clRect);

    AppUiFontsInitialise();

    ToolBarCreate(hWnd, lcInst);

    ghFileTabWnd = CreateWindowEx(0, WC_TABCONTROL, TEXT("filetab"), WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | TCS_SINGLELINE, 0, 0, clRect.right, 0, hWnd, (HMENU)IDTB_MULTIFILE, lcInst, nullptr);
    SetWindowFont(ghFileTabWnd, ghNameFont, FALSE);

    gpfOriginFileTabProc = SubclassWindow(ghFileTabWnd, gpfFileTabProc);

    ZeroMemory(&stTcItem, sizeof(TCITEM));
    stTcItem.mask = TCIF_TEXT;
    stTcItem.pszText = NAMELESS_DUMMY;
    TabCtrl_InsertItem(ghFileTabWnd, 0, &stTcItem);

    ToolBarSizeGet(&tbRect);
    TabCtrl_GetItemRect(ghFileTabWnd, 1, &tiRect);
    tiRect.bottom += tiRect.top;
    MoveWindow(ghFileTabWnd, 0, tbRect.bottom, clRect.right, tiRect.bottom, TRUE);

    ghFileTabTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, ghFileTabWnd, nullptr, lcInst, nullptr);
    AppUiFontApply(ghFileTabTip, FALSE);

    ZeroMemory(&stToolInfo, sizeof(TTTOOLINFO));
    GetClientRect(ghFileTabWnd, &stToolInfo.rect);
    stToolInfo.cbSize = sizeof(TTTOOLINFO);
    stToolInfo.uFlags = TTF_SUBCLASS;
    stToolInfo.hinst = nullptr;
    stToolInfo.hwnd = ghFileTabWnd;
    stToolInfo.uId = IDTT_TILETAB_TIP;
    stToolInfo.lpszText = LPSTR_TEXTCALLBACK;
    SendMessage(ghFileTabTip, TTM_ADDTOOL, 0, (LPARAM)&stToolInfo);
    SendMessage(ghFileTabTip, TTM_SETMAXTIPWIDTH, 0, 0);

    ghMainStatusRedBrush = CreateSolidBrush(0xFF);

    ghMainStatusBarWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, TEXT(""), hWnd, IDSB_VIEW_STATUS_BAR);
    AppUiFontApply(ghMainStatusBarWnd, FALSE);
    SendMessage(ghMainStatusBarWnd, SB_SETPARTS, (WPARAM)kStatusBarItems, (LPARAM)(LPINT)kStatusBarSizes);

    StatusBar_SetText(ghMainStatusBarWnd, 1, TEXT(""));

    AppTrayIconInitialise(hWnd);

    return TRUE;
}

VOID Cls_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);

    UNREFERENCED_PARAMETER(hdc);
}

VOID Cls_OnTimer(HWND hWnd, UINT id)
{
    if (IDT_BACKUP_TIMER == id)
    {
        DocFileBackup(hWnd);
        return;
    }

    if (IDT_PAGE_PRELOAD_TIMER == id)
    {
        AppBackgroundPageLoadTick(hWnd);
        return;
    }
}

BOOL Cls_OnWindowPosChanging(HWND hWnd, LPWINDOWPOS pstWpos)
{
    RECT winPos;

    GetWindowRect(hWnd, &winPos);
    winPos.right = winPos.right - winPos.left;
    winPos.bottom = winPos.bottom - winPos.top;

    TRACE(TEXT("MAIN WM_WINDOWPOSCHANGING [%d %d %d %d][%d %d %d %d]"), winPos.left, winPos.top, winPos.right, winPos.bottom, pstWpos->x, pstWpos->y, pstWpos->cx, pstWpos->cy);

    giResizeWide = pstWpos->cx - winPos.right;

    return TRUE;
}

VOID Cls_OnSize(HWND hWnd, UINT state, INT cx, INT cy)
{
    static UINT ccState;
    RECT rect;

    if (SIZE_MINIMIZED == state)
    {
        AppSetUndockedAuxVisibility(SW_HIDE);
        ccState = SIZE_MINIMIZED;
        return;
    }

    if (SIZE_MINIMIZED == ccState && ccState != state)
    {
        AppSetUndockedAuxVisibility(SW_SHOW);
        ccState = SIZE_RESTORED;
    }

    if (SIZE_MAXIMIZED == state)
    {
        AppSyncSplitBarWithClient();
        ccState = SIZE_MAXIMIZED;
    }

    if (SIZE_RESTORED == state && SIZE_MAXIMIZED == ccState)
    {
        if (!(IsZoomed(hWnd)))
        {
            AppSyncSplitBarWithClient();
            ccState = SIZE_RESTORED;
        }
    }

    MoveWindow(ghMainStatusBarWnd, 0, 0, 0, 0, TRUE);

    ToolBarOnSize(hWnd, state, cx, cy);

    AppClientAreaCalc(&rect);

#ifdef SPLIT_BAR_POS_FIX
    if (ghMainSplitWnd && (kAppSplitBarMovedSize != state))
    {
        AppLayoutSyncMainSplitBar(ghMainSplitWnd, grdSplitPos);
    }
#endif

    ViewSizeMove(hWnd, &rect);
}

VOID Cls_OnMove(HWND hWnd, INT x, INT y)
{
    DWORD dwStyle;

    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    dwStyle = GetWindowStyle(hWnd);
    if (dwStyle & WS_MINIMIZE)
    {
        LayerMoveFromView(hWnd, SIZE_MINIMIZED);
        FrameMoveFromView(hWnd, SIZE_MINIMIZED);
    }
    else
    {
        LayerMoveFromView(hWnd, SIZE_RESTORED);
        FrameMoveFromView(hWnd, SIZE_RESTORED);
    }
}

VOID Cls_OnDestroy(HWND hWnd)
{
    AppTrayIconFinalise();

    AppBackgroundPageLoadStop(hWnd);
    AppModelessDialogsDestroy();

    DocInverseInit(FALSE);

    ToolBarBandInfoGet(nullptr);
    PreviewInitialise(nullptr, nullptr);
    TraceInitialise(hWnd, FALSE);
    OpenHistoryInitialise(nullptr);
    PaletteBucketInitialise(nullptr, nullptr, nullptr, nullptr);
    LayerBoxInitialise(nullptr, nullptr);
    FrameInitialise(nullptr, nullptr);
    CntxEditInitialise(nullptr, nullptr);

    AppPersistWindowState(hWnd);

    DestroyWindow(ghPgVwWnd);

    InitMultiFileTabOpen(INIT_SAVE, 0, nullptr);
    DocInitialise(FALSE);

    SetWindowFont(ghFileTabWnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);

    AppUiFontsFinalise();
    AppUiResourcesFinalise();

    DeleteBrush(ghMainStatusRedBrush);

    ToolBarDestroy();

    DestroyWindow(ghViewWnd);
    AaFontCreate(0);

    PostQuitMessage(0);
}