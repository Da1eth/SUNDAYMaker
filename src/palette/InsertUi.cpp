#include "Sunday.h"
#include "InsertUi.h"

VOID InsertUiUpdateViewOrigin(HWND hViewWnd, LPPOINT pstViewOrigin)
{
    RECT stViewRect;

    if (!(hViewWnd) || !(pstViewOrigin))
        return;

    GetWindowRect(hViewWnd, &stViewRect);
    pstViewOrigin->x = stViewRect.left;
    pstViewOrigin->y = stViewRect.top;
}

VOID InsertUiCaptureContentOffset(HWND hWnd, INT dTopInset, LPPOINT pstContentOffset)
{
    POINT stClientOrigin;
    RECT stWindowRect;

    if (!(hWnd) || !(pstContentOffset))
        return;

    stClientOrigin.x = 0;
    stClientOrigin.y = dTopInset;
    ClientToScreen(hWnd, &stClientOrigin);
    GetWindowRect(hWnd, &stWindowRect);

    pstContentOffset->x = stClientOrigin.x - stWindowRect.left;
    pstContentOffset->y = stClientOrigin.y - stWindowRect.top;
}

VOID InsertUiPlaceRelativeToView(HWND hViewWnd, HWND hWnd, INSERT_UI_GEOMETRY *pstGeometry, INT xOffsetFromView, INT yOffsetFromView)
{
    RECT stViewRect;
    INT x;
    INT y;

    if (!(hViewWnd) || !(hWnd) || !(pstGeometry))
        return;

    GetWindowRect(hViewWnd, &stViewRect);
    pstGeometry->stViewOrigin.x = stViewRect.left;
    pstGeometry->stViewOrigin.y = stViewRect.top;

    x = stViewRect.left + xOffsetFromView;
    y = stViewRect.top + yOffsetFromView;
    SetWindowPos(hWnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    pstGeometry->stWindowOffset.x = x - stViewRect.left;
    pstGeometry->stWindowOffset.y = y - stViewRect.top;
}

VOID InsertUiPlaceContentAtViewPoint(HWND hViewWnd, HWND hWnd, INSERT_UI_GEOMETRY *pstGeometry, INT dTopInset, INT xContentFromView, INT yContentFromView)
{
    if (!(hViewWnd) || !(hWnd) || !(pstGeometry))
        return;

    InsertUiCaptureContentOffset(hWnd, dTopInset, &(pstGeometry->stContentOffset));
    InsertUiPlaceRelativeToView(hViewWnd, hWnd, pstGeometry,
                                xContentFromView - pstGeometry->stContentOffset.x,
                                yContentFromView - pstGeometry->stContentOffset.y);
}

BOOL InsertUiSnapWindowYToViewLine(HWND hViewWnd, INSERT_UI_GEOMETRY *pstGeometry, LPWINDOWPOS pstWpos)
{
    INT dContentTop;
    INT dViewTop;
    INT dDiff;
    INT dRemain;
    BOOLEAN bMinus = FALSE;

    if (!(hViewWnd) || !(pstGeometry) || !(pstWpos))
        return TRUE;

    if (SWP_NOMOVE & pstWpos->flags)
        return TRUE;

    dContentTop = pstWpos->y + pstGeometry->stContentOffset.y;
    InsertUiUpdateViewOrigin(hViewWnd, &(pstGeometry->stViewOrigin));
    dViewTop = pstGeometry->stViewOrigin.y + RULER_AREA;

    dDiff = dViewTop - dContentTop;
    if (0 > dDiff)
    {
        dDiff *= -1;
        bMinus = TRUE;
    }

    dRemain = dDiff % LINE_HEIGHT;
    if (0 == dRemain)
        return TRUE;

    if ((LINE_HEIGHT / 2) < dRemain)
        dRemain -= LINE_HEIGHT;

    if (bMinus)
        dRemain *= -1;

    pstWpos->y += dRemain;
    return FALSE;
}

VOID InsertUiUpdateOffsetFromWindowPos(HWND hViewWnd, INSERT_UI_GEOMETRY *pstGeometry, const LPWINDOWPOS pstWpos)
{
    if (!(hViewWnd) || !(pstGeometry) || !(pstWpos))
        return;

    InsertUiUpdateViewOrigin(hViewWnd, &(pstGeometry->stViewOrigin));
    pstGeometry->stWindowOffset.x = pstWpos->x - pstGeometry->stViewOrigin.x;
    pstGeometry->stWindowOffset.y = pstWpos->y - pstGeometry->stViewOrigin.y;
}

VOID InsertUiFormatStatusText(const RECT *pstPos, const INSERT_UI_GEOMETRY *pstGeometry, INT dHideXdot, INT dTopLine, LPCTSTR ptLabel, LPTSTR ptBuffer, UINT_PTR cchBuffer)
{
    LONG xEt;
    LONG yEt;
    LONG xLy;
    LONG yLy;
    LONG xSb;
    LONG ySb;
    LONG dLine;
    LONG dRema;
    BOOLEAN bMinus = FALSE;

    if (!(pstPos) || !(pstGeometry) || !(ptLabel) || !(ptBuffer) || 0 >= cchBuffer)
        return;

    xLy = pstPos->left + pstGeometry->stContentOffset.x;
    yLy = pstPos->top + pstGeometry->stContentOffset.y;

    xEt = pstGeometry->stViewOrigin.x + LINENUM_WID;
    yEt = pstGeometry->stViewOrigin.y + RULER_AREA;

    xSb = xLy - xEt;
    ySb = yLy - yEt;

    if (0 > ySb)
    {
        ySb *= -1;
        bMinus = TRUE;
    }

    dLine = ySb / LINE_HEIGHT;
    dRema = ySb % LINE_HEIGHT;
    if ((LINE_HEIGHT / 2) < dRema)
        dLine++;

    if (bMinus)
        dLine *= -1;
    else
        dLine++;

    xSb += dHideXdot;
    dLine += dTopLine;

    StringCchPrintf(ptBuffer, cchBuffer, TEXT("%s %d번째 줄 %d도트"), ptLabel, dLine, xSb);
}

HRESULT InsertUiMoveFromView(HWND hViewWnd, HWND hFloatingWnd, UINT state, INSERT_UI_GEOMETRY *pstGeometry)
{
    POINT stWindowPoint;

    if (!(hFloatingWnd) || !(pstGeometry))
        return S_FALSE;

    if (SIZE_MINIMIZED != state)
        InsertUiUpdateViewOrigin(hViewWnd, &(pstGeometry->stViewOrigin));

    if (SIZE_MINIMIZED == state)
    {
        ShowWindow(hFloatingWnd, SW_HIDE);
    }
    else
    {
        stWindowPoint.x = pstGeometry->stWindowOffset.x + pstGeometry->stViewOrigin.x;
        stWindowPoint.y = pstGeometry->stWindowOffset.y + pstGeometry->stViewOrigin.y;
        SetWindowPos(hFloatingWnd, HWND_TOPMOST, stWindowPoint.x, stWindowPoint.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    return S_OK;
}

INT InsertUiToolbarInitialise(HWND hToolbarWnd, HIMAGELIST hImageList, TBBUTTON *pstButtons, UINT dButtonCount,
                              const UINT *pdStringButtonIndices, const LPCTSTR *pptButtonTexts, UINT dStringCount,
                              BOOL bSetButtonSize)
{
    UINT i;
    RECT stToolbarRect;

    if (!(hToolbarWnd) || !(pstButtons) || 0 >= dButtonCount)
        return 0;

    SendMessage(hToolbarWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
    if (hImageList)
    {
        SendMessage(hToolbarWnd, TB_SETIMAGELIST, 0, (LPARAM)hImageList);
    }
    if (bSetButtonSize)
    {
        SendMessage(hToolbarWnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(16, 16));
    }
    SendMessage(hToolbarWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

    for (i = 0; dStringCount > i; i++)
    {
        pstButtons[pdStringButtonIndices[i]].iString = SendMessage(hToolbarWnd, TB_ADDSTRING, 0, (LPARAM)pptButtonTexts[i]);
    }

    SendMessage(hToolbarWnd, TB_ADDBUTTONS, (WPARAM)dButtonCount, (LPARAM)pstButtons);
    SendMessage(hToolbarWnd, TB_AUTOSIZE, 0, 0);
    InvalidateRect(hToolbarWnd, nullptr, TRUE);

    GetWindowRect(hToolbarWnd, &stToolbarRect);
    return stToolbarRect.bottom - stToolbarRect.top;
}