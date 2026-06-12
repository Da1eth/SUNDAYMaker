#include "Sunday.h"
#include "AppModuleInternal.h"
#include "Palette.h"

extern HWND ghFindDlg;

HWND ghColourTagDlg = nullptr;
HWND ghGradientTagDlg = nullptr;

namespace
{
BOOLEAN AppModelessDialogProcess(LPMSG pstMsg, HWND hDlg)
{
    if (!(pstMsg) || !(hDlg) || hDlg != GetForegroundWindow())
        return FALSE;

    if (TranslateAccelerator(hDlg, ghMainAccelTable, pstMsg))
        return TRUE;
    if (IsDialogMessage(hDlg, pstMsg))
        return TRUE;

    return FALSE;
}

VOID AppModelessDialogDestroy(HWND *phDlg)
{
    if (!(phDlg) || !(*phDlg))
        return;

    DestroyWindow(*phDlg);
    *phDlg = nullptr;
}
}

BOOLEAN AppModelessDialogHandleMessage(LPMSG pstMsg)
{
    if (AppModelessDialogProcess(pstMsg, ghFindDlg))
        return TRUE;

    if (AppModelessDialogProcess(pstMsg, ghColourTagDlg))
        return TRUE;
    if (AppModelessDialogProcess(pstMsg, ghGradientTagDlg))
        return TRUE;

    if (PaletteEditorHandleMessage(pstMsg))
        return TRUE;
    if (FrameEditHandleMessage(pstMsg))
        return TRUE;

    return FALSE;
}

VOID AppModelessDialogsDestroy(VOID)
{
    AppModelessDialogDestroy(&ghFindDlg);
    AppModelessDialogDestroy(&ghColourTagDlg);
    AppModelessDialogDestroy(&ghGradientTagDlg);
    PaletteEditorDestroy();
    FrameEditDestroy();
}

BOOLEAN AppModelessEditCommandForward(HWND hDlg, UINT id)
{
    HWND hFocusWnd;
    INT iTextLength;
    UINT uMessage;

    switch (id)
    {
    case IDM_PASTE:
        uMessage = WM_PASTE;
        break;
    case IDM_COPY:
        uMessage = WM_COPY;
        break;
    case IDM_CUT:
        uMessage = WM_CUT;
        break;
    case IDM_UNDO:
        uMessage = WM_UNDO;
        break;
    case IDM_ALLSEL:
        uMessage = 0;
        break;
    default:
        return FALSE;
    }

    hFocusWnd = GetFocus();
    if (!(hFocusWnd) || !(IsChild(hDlg, hFocusWnd)))
        return FALSE;

    if (IDM_ALLSEL == id)
    {
        iTextLength = GetWindowTextLength(hFocusWnd);
        SendMessage(hFocusWnd, EM_SETSEL, 0, iTextLength);
    }
    else
    {
        SendMessage(hFocusWnd, uMessage, 0, 0);
    }
    return TRUE;
}
