#include "Sunday.h"
#include "Palette.h"
#include "AppModuleInternal.h"

namespace
{
BOOLEAN AppDispatchMainCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    if (AppHandleMainCommand(hWnd, id))
    {
        return TRUE;
    }

    OperationOnCommand(hWnd, id, hWndCtl, codeNotify);
    return TRUE;
}

VOID AppHandleDockTabSelection(INT curSel)
{
    const AppUiContext stUiContext = AppUiContextGet();

    if (!(gbDockTmplView))
        return;

    switch (curSel)
    {
    case 0:
        ShowWindow(stUiContext.hInsertPaletteWindow, SW_SHOW);
        ShowWindow(stUiContext.hBucketPaletteWindow, SW_HIDE);
        break;

    case 1:
        ShowWindow(stUiContext.hInsertPaletteWindow, SW_HIDE);
        ShowWindow(stUiContext.hBucketPaletteWindow, SW_SHOW);
        break;

    default:
        break;
    }
}

BOOLEAN AppHandleMainContextMenuRoute(HWND hWnd, HWND hWndContext, const POINT *pstPos)
{
    if (ToolBarOnContextMenu(hWnd, hWndContext, pstPos->x, pstPos->y))
    {
        return TRUE;
    }

    if (SUCCEEDED(DockingTabContextMenu(hWnd, hWndContext, pstPos->x, pstPos->y)))
    {
        return TRUE;
    }

    if (AppFileTabHandleContextMenu(hWnd, hWndContext, pstPos))
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN AppHandleMainNotifyRoute(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    INT curSel;

    UNREFERENCED_PARAMETER(hWnd);

    if (IDTB_DOCK_TAB == idFrom && TCN_SELCHANGE == pstNmhdr->code)
    {
        curSel = TabCtrl_GetCurSel(pstNmhdr->hwndFrom);

        TRACE(TEXT("TMPL TAB sel [%d]"), curSel);

        AppHandleDockTabSelection(curSel);
        return TRUE;
    }

    if (IDTB_MULTIFILE == idFrom && TCN_SELCHANGE == pstNmhdr->code)
    {
        AppFileTabHandleSelectionChange();
        return TRUE;
    }

    return FALSE;
}

BOOLEAN AppHandleMainTooltipNotify(INT idFrom, LPNMHDR pstNmhdr)
{
    LPNMTTDISPINFO pstDispInfo;
    POINT stCursorPos;

    if (IDTT_TILETAB_TIP != idFrom || TTN_GETDISPINFO != pstNmhdr->code)
    {
        return FALSE;
    }

    GetCursorPos(&stCursorPos);

    pstDispInfo = (LPNMTTDISPINFO)pstNmhdr;

    ZeroMemory(&(pstDispInfo->szText), sizeof(pstDispInfo->szText));

    pstDispInfo->lpszText = const_cast<LPTSTR>(
        AppFileTabTooltipTextGet(&stCursorPos, pstDispInfo->szText, 80));

    return TRUE;
}
}

VOID Cls_OnCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    AppDispatchMainCommand(hWnd, id, hWndCtl, codeNotify);
}

VOID Cls_OnContextMenu(HWND hWnd, HWND hWndContext, UINT xPos, UINT yPos)
{
    POINT stPost;

    stPost.x = (SHORT)xPos;
    stPost.y = (SHORT)yPos;

    TRACE(TEXT("MAIN CONTEXT[%d x %d]"), stPost.x, stPost.y);

    AppHandleMainContextMenuRoute(hWnd, hWndContext, &stPost);
}

LRESULT Cls_OnNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    ToolBarOnNotify(hWnd, idFrom, pstNmhdr);

    if (AppHandleMainNotifyRoute(hWnd, idFrom, pstNmhdr))
    {
        return 0;
    }

    if (AppHandleMainTooltipNotify(idFrom, pstNmhdr))
    {
        return 0;
    }

    return 0;
}

#ifdef MULTIACT_RELAY
void Cls_OnCopyData(HWND hWnd, HWND hWndFrom, PCOPYDATASTRUCT pstCopyData)
{
    TCHAR atBuff[MAX_PATH];

    UNREFERENCED_PARAMETER(hWndFrom);

    StringCchCopy(atBuff, MAX_PATH, (LPTSTR)(pstCopyData->lpData));

    TRACE(TEXT("COPYDATA[%s]"), atBuff);

    AppHandleDocumentOpenPath(hWnd, atBuff);
}
#endif

VOID Cls_OnDropFiles(HWND hWnd, HDROP hDrop)
{
    TCHAR atFileName[MAX_PATH];

    ZeroMemory(atFileName, sizeof(atFileName));

    DragQueryFile(hDrop, 0, atFileName, MAX_PATH);
    DragFinish(hDrop);

    TRACE(TEXT("DROP[%s]"), atFileName);

    AppHandleDocumentOpenPath(hWnd, atFileName);
}

VOID Cls_OnHotKey(HWND hWnd, INT idHotKey, UINT fuModifiers, UINT vk)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(idHotKey);
    UNREFERENCED_PARAMETER(fuModifiers);

    if (VK_D == vk)
    {
        TRACE(TEXT("Hotkey Incoming!!"));
        DocThreadDropCopy();
    }
}

VOID Cls_OnDrawItem(HWND hWnd, CONST DRAWITEMSTRUCT *pstDrawItem)
{
    UINT dBytes;
    TCHAR atBuff[SUB_STRING];
    RECT rect;

    UNREFERENCED_PARAMETER(hWnd);

    if (IDSB_VIEW_STATUS_BAR != pstDrawItem->CtlID)
        return;

    if (SB_BYTECNT == pstDrawItem->itemID)
    {
        const AppUiContext stUiContext = AppUiContextGet();

        SetBkMode(pstDrawItem->hDC, TRANSPARENT);
        rect = pstDrawItem->rcItem;

        dBytes = pstDrawItem->itemData;
        StringCchPrintf(atBuff, SUB_STRING, TEXT("%d 바이트"), dBytes);

        if (gdPageByteMax < dBytes)
            FillRect(pstDrawItem->hDC, &(pstDrawItem->rcItem), stUiContext.hMainStatusWarningBrush);

        DrawText(pstDrawItem->hDC, atBuff, -1, &(rect), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}