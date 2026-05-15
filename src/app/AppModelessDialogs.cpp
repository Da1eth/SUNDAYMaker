#include "Sunday.h"
#include "AppModuleInternal.h"
#include "Palette.h"

#ifdef FIND_STRINGS
extern HWND ghFindDlg;
#endif

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
#ifdef FIND_STRINGS
    if (AppModelessDialogProcess(pstMsg, ghFindDlg))
        return TRUE;
#endif

    if (AppModelessDialogProcess(pstMsg, ghColourTagDlg))
        return TRUE;
    if (AppModelessDialogProcess(pstMsg, ghGradientTagDlg))
        return TRUE;

    if (PaletteEditorHandleMessage(pstMsg))
        return TRUE;

    return FALSE;
}

VOID AppModelessDialogsDestroy(VOID)
{
#ifdef FIND_STRINGS
    AppModelessDialogDestroy(&ghFindDlg);
#endif
    AppModelessDialogDestroy(&ghColourTagDlg);
    AppModelessDialogDestroy(&ghGradientTagDlg);
    PaletteEditorDestroy();
}

BOOLEAN AppModelessEditCommandForward(HWND hDlg, UINT id)
{
    HWND hFocusWnd;
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
    default:
        return FALSE;
    }

    hFocusWnd = GetFocus();
    if (!(hFocusWnd) || !(IsChild(hDlg, hFocusWnd)))
        return FALSE;

    SendMessage(hFocusWnd, uMessage, 0, 0);
    return TRUE;
}