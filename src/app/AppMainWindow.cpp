#include "Sunday.h"
#include "AppModuleInternal.h"

static BOOLEAN WndProcHandleMiscMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HMENU hSubMenu;

    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_MBUTTONUP:
        TRACE(TEXT("MIDDLE  UP"));
        *plResult = 0;
        return TRUE;

    case WM_SYSCOMMAND:
        if (IDM_POSITION_RESET == LOWORD(wParam))
        {
            WindowPositionReset(hWnd);
            *plResult = 0;
            return TRUE;
        }
        return FALSE;

    case WM_CLOSE:
        if (!(DocFileCloseCheck(hWnd)))
        {
            *plResult = FALSE;
            return TRUE;
        }
        return FALSE;

    case WMP_BRUSH_TOGGLE:
        hSubMenu = GetSubMenu(ghMainMenu, 1);
        CheckMenuItem(hSubMenu, IDM_BRUSH_STYLE, wParam ? MF_CHECKED : MF_UNCHECKED);
        *plResult = 0;
        return TRUE;

    case WMP_PREVIEW_CLOSE:
        DestroyCaret();
        ViewFocusSet();
        ViewShowCaret();
        *plResult = 0;
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;

    switch (message)
    {
        HANDLE_MSG(hWnd, WM_CREATE, Cls_OnCreate);
        HANDLE_MSG(hWnd, WM_PAINT, Cls_OnPaint);
        HANDLE_MSG(hWnd, WM_COMMAND, Cls_OnCommand);
        HANDLE_MSG(hWnd, WM_DESTROY, Cls_OnDestroy);
        HANDLE_MSG(hWnd, WM_SIZE, Cls_OnSize);
        HANDLE_MSG(hWnd, WM_MOVE, Cls_OnMove);
        HANDLE_MSG(hWnd, WM_DROPFILES, Cls_OnDropFiles);
        HANDLE_MSG(hWnd, WM_ACTIVATE, Cls_OnActivate);
        HANDLE_MSG(hWnd, WM_NOTIFY, Cls_OnNotify);
        HANDLE_MSG(hWnd, WM_TIMER, Cls_OnTimer);
        HANDLE_MSG(hWnd, WM_CONTEXTMENU, Cls_OnContextMenu);
        HANDLE_MSG(hWnd, WM_HOTKEY, Cls_OnHotKey);
        HANDLE_MSG(hWnd, WM_DRAWITEM, Cls_OnDrawItem);
#ifdef MULTIACT_RELAY
        HANDLE_MSG(hWnd, WM_COPYDATA, Cls_OnCopyData);
#endif
        HANDLE_MSG(hWnd, WM_KEYDOWN, Evw_OnKey);
        HANDLE_MSG(hWnd, WM_KEYUP, Evw_OnKey);
        HANDLE_MSG(hWnd, WM_CHAR, Evw_OnChar);
        HANDLE_MSG(hWnd, WM_MOUSEWHEEL, Evw_OnMouseWheel);
        HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGING, Cls_OnWindowPosChanging);

    default:
        if (AppTrayIconHandleWindowMessage(hWnd, message, wParam, lParam, &lResult))
        {
            return lResult;
        }

        if (WndProcHandleMiscMessage(hWnd, message, wParam, lParam, &lResult))
        {
            return lResult;
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}