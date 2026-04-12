#include "AppModuleInternal.h"

namespace
{
HWND AppFileTabHandleGet()
{
    return AppFileTabWindowGet();
}

HRESULT AppFileTabNumberFromSelection(INT iIndex, LPARAM *pdNumber, LPTSTR ptText,
                                      UINT cchText)
{
    TCITEM stTcItem;

    if (!(pdNumber) || 0 > iIndex)
        return E_INVALIDARG;

    ZeroMemory(&stTcItem, sizeof(TCITEM));
    stTcItem.mask = TCIF_PARAM;
    if (ptText && cchText)
    {
        stTcItem.mask |= TCIF_TEXT;
        stTcItem.pszText = ptText;
        stTcItem.cchTextMax = static_cast<int>(cchText);
    }

    if (!(TabCtrl_GetItem(AppFileTabHandleGet(), iIndex, &stTcItem)))
        return E_OUTOFMEMORY;

    *pdNumber = stTcItem.lParam;

    return S_OK;
}
}

LRESULT CALLBACK gpfFileTabProc(HWND hWnd, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hWnd, WM_MBUTTONUP, Ftb_OnMButtonUp);

    default:
        break;
    }

    return CallWindowProc(gpfOriginFileTabProc, hWnd, msg, wParam, lParam);
}

VOID Ftb_OnMButtonUp(HWND hWnd, INT x, INT y, UINT flags)
{
    INT curSel;
    TCHITTESTINFO stTcHitInfo;

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(flags);

    stTcHitInfo.pt.x = x;
    stTcHitInfo.pt.y = y;
    curSel = TabCtrl_HitTest(AppFileTabHandleGet(), &stTcHitInfo);

    TRACE(TEXT("FTAB start TAB [%d] [%d x %d]"), curSel, x, y);

    MultiFileTabClose(curSel);
}

HRESULT MultiFileTabFirst(LPTSTR ptName)
{
    TCHAR atName[MAX_PATH];
    TCITEM stTcItem;

    StringCchCopy(atName, MAX_PATH, ptName);
    PathStripPath(atName);

    ZeroMemory(&stTcItem, sizeof(TCITEM));
    stTcItem.mask = TCIF_TEXT | TCIF_PARAM;
    stTcItem.pszText = atName;
    stTcItem.lParam = 1;
    TabCtrl_InsertItem(AppFileTabHandleGet(), 1, &stTcItem);

    TabCtrl_DeleteItem(AppFileTabHandleGet(), 0);
    TabCtrl_SetCurSel(AppFileTabHandleGet(), 0);

    return S_OK;
}

HRESULT MultiFileTabAppend(LPARAM dNumber, LPTSTR ptName)
{
    INT iCount;
    TCHAR atName[MAX_PATH];
    TCITEM stTcItem;

    StringCchCopy(atName, MAX_PATH, ptName);
    PathStripPath(atName);

    iCount = TabCtrl_GetItemCount(AppFileTabHandleGet());

    ZeroMemory(&stTcItem, sizeof(TCITEM));
    stTcItem.mask = TCIF_TEXT | TCIF_PARAM;
    stTcItem.pszText = atName;
    stTcItem.lParam = dNumber;
    TabCtrl_InsertItem(AppFileTabHandleGet(), iCount, &stTcItem);

    TabCtrl_SetCurSel(AppFileTabHandleGet(), iCount);

    return S_OK;
}

INT MultiFileTabSearch(LPARAM dNumber)
{
    INT iCount;
    INT i;
    TCITEM stTcItem;

    iCount = TabCtrl_GetItemCount(AppFileTabHandleGet());

    ZeroMemory(&stTcItem, sizeof(TCITEM));
    stTcItem.mask = TCIF_PARAM;

    for (i = 0; iCount > i; i++)
    {
        TabCtrl_GetItem(AppFileTabHandleGet(), i, &stTcItem);
        if (dNumber == stTcItem.lParam)
            return i;
    }

    return -1;
}

HRESULT MultiFileTabSelect(LPARAM dNumber)
{
    INT iRslt;

    iRslt = MultiFileTabSearch(dNumber);
    if (0 <= iRslt)
    {
        TabCtrl_SetCurSel(AppFileTabHandleGet(), iRslt);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

HRESULT MultiFileTabSelectDocument(LPARAM dNumber)
{
    HRESULT hr;

    hr = MultiFileTabSelect(dNumber);
    if (FAILED(hr))
        return hr;

    return DocMultiFileSelect(dNumber);
}

HRESULT MultiFileTabSlide(INT xDir)
{
    INT curSel;
    INT iCount;
    INT iTarget;
    LPARAM dNumber;

    if (0 == xDir)
        return S_FALSE;

    iCount = TabCtrl_GetItemCount(AppFileTabHandleGet());
    curSel = TabCtrl_GetCurSel(AppFileTabHandleGet());

    if (0 < xDir)
    {
        iTarget = curSel + 1;
        if (iCount <= iTarget)
            iTarget = 0;
    }
    else
    {
        iTarget = curSel - 1;
        if (0 > iTarget)
            iTarget = iCount - 1;
    }

    if (FAILED(AppFileTabNumberFromSelection(iTarget, &dNumber, nullptr, 0)))
        return E_OUTOFMEMORY;

    return MultiFileTabSelectDocument(dNumber);
}

HRESULT MultiFileTabRename(LPARAM dNumber, LPTSTR ptName)
{
    INT iRslt;
    TCHAR atName[MAX_PATH];
    TCITEM stTcItem;

    iRslt = MultiFileTabSearch(dNumber);
    if (0 > iRslt)
        return E_OUTOFMEMORY;

    StringCchCopy(atName, MAX_PATH, ptName);
    PathStripPath(atName);

    ZeroMemory(&stTcItem, sizeof(TCITEM));
    stTcItem.mask = TCIF_TEXT;
    stTcItem.pszText = atName;
    TabCtrl_SetItem(AppFileTabHandleGet(), iRslt, &stTcItem);

    return S_OK;
}

INT InitMultiFileTabOpen(UINT dMode, INT iTgt, LPTSTR ptFile)
{
    if (dMode)
    {
        return DocMultiFileFetch(iTgt, ptFile, gatIniPath);
    }

    DocMultiFileStore(gatIniPath);
    return 0;
}

HRESULT MultiFileTabClose(INT iSelTab)
{
    INT curSel;
    INT nowSel;
    LPARAM dSele;
    TCITEM stTcItem;
    const HWND hFileTabWnd = AppFileTabHandleGet();

    nowSel = TabCtrl_GetCurSel(hFileTabWnd);
    if (0 > iSelTab)
        curSel = nowSel;
    else
        curSel = iSelTab;

    ZeroMemory(&stTcItem, sizeof(TCITEM));
    stTcItem.mask = TCIF_PARAM;
    TabCtrl_GetItem(hFileTabWnd, curSel, &stTcItem);

    dSele = DocMultiFileClose(AppMainWindowGet(), stTcItem.lParam);
    if (dSele)
    {
        TabCtrl_DeleteItem(hFileTabWnd, curSel);
        if (curSel == nowSel)
            MultiFileTabSelect(dSele);
    }

    return S_OK;
}

HRESULT AppFileTabHandleSelectionChange(VOID)
{
    INT curSel;
    TCHAR atText[MAX_PATH];
    LPARAM dNumber;

    curSel = TabCtrl_GetCurSel(AppFileTabHandleGet());

    TRACE(TEXT("FILE TAB sel [%d]"), curSel);

    if (FAILED(AppFileTabNumberFromSelection(curSel, &dNumber, atText, MAX_PATH)))
        return E_OUTOFMEMORY;

    TRACE(TEXT("FILE [%s] param[%d]"), atText, dNumber);

    return MultiFileTabSelectDocument(dNumber);
}

BOOLEAN AppFileTabHandleContextMenu(HWND hWnd, HWND hWndContext,
                                    const POINT *pstScreenPos)
{
    HMENU hMenu;
    HMENU hSubMenu;
    UINT dRslt;
    INT curSel;
    INT iCount;
    POINT stPos;
    TCHITTESTINFO stTcHitInfo;
    MENUITEMINFO stMenuItemInfo;
    TCHAR atText[MAX_PATH];
    LPARAM dNumber;
    UINT cchSize;
    const AppUiContext stUiContext = AppUiContextGet();

    if (!pstScreenPos || AppFileTabHandleGet() != hWndContext)
        return FALSE;

    stPos = *pstScreenPos;
    iCount = TabCtrl_GetItemCount(AppFileTabHandleGet());

    hMenu = LoadMenu(stUiContext.hInstance, MAKEINTRESOURCE(IDM_MULTIFILE_POPUP));
    hSubMenu = GetSubMenu(hMenu, 0);

    stTcHitInfo.pt = stPos;
    ScreenToClient(AppFileTabHandleGet(), &(stTcHitInfo.pt));
    curSel = TabCtrl_HitTest(AppFileTabHandleGet(), &stTcHitInfo);

    if (FAILED(AppFileTabNumberFromSelection(curSel, &dNumber, atText, MAX_PATH)))
    {
        DestroyMenu(hMenu);
        return TRUE;
    }

    MultiFileTabSelectDocument(dNumber);

    StringCchCat(atText, MAX_PATH, TEXT(" 파일 닫기(&Q)"));
    StringCchLength(atText, MAX_PATH, &cchSize);

    ZeroMemory(&stMenuItemInfo, sizeof(MENUITEMINFO));
    stMenuItemInfo.cbSize = sizeof(MENUITEMINFO);
    stMenuItemInfo.fMask = MIIM_TYPE;
    stMenuItemInfo.fType = MFT_STRING;
    stMenuItemInfo.cch = cchSize;
    stMenuItemInfo.dwTypeData = atText;

    if (1 >= iCount)
    {
        stMenuItemInfo.fMask |= MIIM_STATE;
        stMenuItemInfo.fState = MFS_GRAYED;
    }

    SetMenuItemInfo(hSubMenu, IDM_FILE_CLOSE, FALSE, &stMenuItemInfo);

    dRslt = TrackPopupMenu(hSubMenu, 0, stPos.x, stPos.y, 0, hWnd, nullptr);
    UNREFERENCED_PARAMETER(dRslt);

    DestroyMenu(hMenu);
    return TRUE;
}

LPCTSTR AppFileTabTooltipTextGet(const POINT *pstScreenPos, LPTSTR ptFallback,
                                 UINT cchFallback)
{
    INT curSel;
    TCHITTESTINFO stTcHitTest;
    LPCTSTR ptText;

    if (!pstScreenPos)
        return nullptr;

    stTcHitTest.pt = *pstScreenPos;
    ScreenToClient(AppFileTabHandleGet(), &(stTcHitTest.pt));

    curSel = TabCtrl_HitTest(AppFileTabHandleGet(), &stTcHitTest);
    TRACE(TEXT("FILE TAB under [%d]"), curSel);

    ptText = DocMultiFileNameGet(curSel);
    if (ptText)
        return ptText;

    if (ptFallback && cchFallback)
    {
        StringCchCopy(ptFallback, cchFallback, TEXT(" "));
        return ptFallback;
    }

    return nullptr;
}