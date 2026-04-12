#include "ViewCentralInternal.h"

static HRESULT ViewDrawMetricLine(HDC, UINT);
static BOOLEAN ViewDrawTextOut(HDC, INT, UINT, LPLETTER, UINT_PTR);
static BOOLEAN ViewDrawSpace(HDC, INT, UINT, LPTSTR, UINT_PTR, UINT);
static HRESULT ViewDrawReturnMark(HDC, INT, INT, UINT);
static INT ViewDrawEOFMark(HDC, INT, INT, UINT);
static HRESULT ViewDrawRuler(HDC);
static HRESULT ViewDrawLineNumber(HDC);
static BOOLEAN ViewIsLineVisible(INT);
static VOID ViewBuildLineRect(LPRECT, INT, INT);
static VOID ViewGetTextColours(COLORREF *, COLORREF *);
static VOID ViewPrepareTextColours(HDC, COLORREF, COLORREF *, COLORREF *);
static HBRUSH ViewHighlightBrushGet(UINT, COLORREF *);
static HBRUSH ViewReturnMarkBrushGet(UINT, COLORREF *);
static HPEN ViewPrepareSpacePen(HDC, UINT);

static BOOLEAN ViewIsLineVisible(INT rdLine)
{
    auto origin = ViewCurrentOrigin();

    if (origin.dTopLine > rdLine)
        return FALSE;
    if ((origin.dTopLine + gdDispingLine + 1) < rdLine)
        return FALSE;

    return TRUE;
}

static VOID ViewBuildLineRect(LPRECT pstRect, INT rdLine, INT right)
{
    INT dDummy = 0;

    SetRect(pstRect, 0, rdLine * LINE_HEIGHT, right, (rdLine + 1) * LINE_HEIGHT);
    ViewPositionTransform(&dDummy, (PINT)&(pstRect->top), 1);
    ViewPositionTransform(&dDummy, (PINT)&(pstRect->bottom), 1);
}

static VOID ViewGetTextColours(COLORREF *pTextColour, COLORREF *pReverseColour)
{
    COLORREF clrTrcMozi;
    COLORREF clrMozi;

    if (TraceMoziColourGet(&clrTrcMozi))
    {
        clrMozi = clrTrcMozi;
    }
    else
    {
        clrMozi = gaColourTable[CLRT_BASICPEN];
    }

    if (pTextColour)
        *pTextColour = clrMozi;

    if (pReverseColour)
    {
        *pReverseColour = ~clrMozi;
        *pReverseColour &= 0x00FFFFFF;
    }
}

static VOID ViewPrepareTextColours(HDC hdc, COLORREF clrMozi, COLORREF *pTextOld, COLORREF *pBackOld)
{
    if (pTextOld)
        *pTextOld = SetTextColor(hdc, clrMozi);
    if (pBackOld)
        *pBackOld = SetBkColor(hdc, gaColourTable[CLRT_BASICBK]);
}

static HBRUSH ViewHighlightBrushGet(UINT bStyle, COLORREF *pBackColour)
{
    if (bStyle & CT_SELECT)
    {
        if (pBackColour)
            *pBackColour = gaColourTable[CLRT_SELECTBK];
        return gahBrush[BRHT_SELECTBK];
    }
    if (bStyle & CT_FINDED)
    {
        if (pBackColour)
            *pBackColour = gaColourTable[CLRT_FINDBACK];
        return gahBrush[BRHT_FINDBACK];
    }
    if (bStyle & CT_NOSJIS)
    {
        if (pBackColour)
            *pBackColour = gaColourTable[CLRT_NOSJISBK];
        return gahBrush[BRHT_NOSJISBK];
    }

    if (pBackColour)
        *pBackColour = gaColourTable[CLRT_BASICBK];
    return nullptr;
}

static HBRUSH ViewReturnMarkBrushGet(UINT dFlag, COLORREF *pBackColour)
{
    if (dFlag & CT_SELRTN)
    {
        if (pBackColour)
            *pBackColour = gaColourTable[CLRT_SELECTBK];
        return gahBrush[BRHT_SELECTBK];
    }
    if (dFlag & CT_FINDRTN)
    {
        if (pBackColour)
            *pBackColour = gaColourTable[CLRT_FINDBACK];
        return gahBrush[BRHT_FINDBACK];
    }
    if (dFlag & CT_LASTSP)
    {
        if (pBackColour)
            *pBackColour = gaColourTable[CLRT_LASTSPWARN];
        return gahBrush[BRHT_LASTSPWARN];
    }

    if (pBackColour)
        *pBackColour = gaColourTable[CLRT_BASICBK];
    return gahBrush[BRHT_BASICBK];
}

static HPEN ViewPrepareSpacePen(HDC hdc, UINT bStyle)
{
    if (bStyle & CT_WARNING)
        return SelectPen(hdc, gahPen[PENT_SPACEWARN]);

    return SelectPen(hdc, gahPen[PENT_SPACELINE]);
}

HRESULT ViewRedrawSetRect(LPRECT pstRect)
{
    RECT rect;

    if (!(pstRect))
        return E_INVALIDARG;

    rect = *pstRect;
    rect.right++;
    rect.bottom++;

    ViewPositionTransform((PINT)&(rect.left), (PINT)&(rect.top), 1);
    ViewPositionTransform((PINT)&(rect.right), (PINT)&(rect.bottom), 1);

    InvalidateRect(ghViewWnd, &rect, TRUE);

    return S_OK;
}

HRESULT ViewRedrawSetVartRuler(INT rdLine)
{
    RECT rect;
    INT dDummy = 0;

    if (!(ViewIsLineVisible(rdLine)))
        return S_FALSE;

    rect.top = rdLine * LINE_HEIGHT;
    ViewPositionTransform(&dDummy, (PINT)&(rect.top), 1);

    rect.bottom = rect.top + LINE_HEIGHT;
    rect.left = 0;
    rect.right = LINENUM_WID + 2;

    InvalidateRect(ghViewWnd, &rect, TRUE);

    return S_OK;
}

HRESULT ViewRedrawSetLine(INT rdLine)
{
    RECT rect, clRect;

    ViewScrollBarAdjust(nullptr);

    if (0 > rdLine)
    {
        InvalidateRect(ghViewWnd, nullptr, TRUE);
        return S_OK;
    }

    if (!(ViewIsLineVisible(rdLine)))
        return S_FALSE;

    GetClientRect(ghViewWnd, &clRect);

    ViewBuildLineRect(&rect, rdLine, clRect.right);

    InvalidateRect(ghViewWnd, &rect, TRUE);

    return S_OK;
}

HRESULT ViewRedrawDo(HWND hWnd, HDC hdc)
{
    LPLETTER pstTexts = nullptr;
    INT cchLen = 0, dot, iLines, i, vwLines;
    UINT dFlag = 0;
    HFONT hFtOld;
    UINT bTrace = FALSE;
    auto origin = ViewCurrentOrigin();
    const VIEWDISPLAYSTATE stDisplayState = ViewDisplayStateGet();

    UNREFERENCED_PARAMETER(hWnd);

    hFtOld = SelectFont(hdc, stDisplayState.hAaFont);

    iLines = DocPageParamGet(nullptr, nullptr);
    vwLines = gdDispingLine + 2 + origin.dTopLine;

    bTrace = TraceImageAppear(hdc, origin.dHideXdot, origin.dTopLine * LINE_HEIGHT);
    if (bTrace)
        SetBkMode(hdc, TRANSPARENT);

    ViewDrawMetricLine(hdc, 0);

    for (i = 0; iLines > i; i++)
    {
        if (origin.dTopLine > i)
            continue;
        if (vwLines <= i)
            break;

        dot = DocLineDataGetAlloc(i, 0, &(pstTexts), &cchLen, &dFlag);
        if (0 < cchLen)
        {
            ViewDrawTextOut(hdc, 0, i, pstTexts, cchLen);
        }
        FREE(pstTexts);

        if (dFlag & CT_RETURN)
        {
            ViewDrawReturnMark(hdc, dot, i, dFlag);
        }

        if (dFlag & CT_EOF)
        {
            ViewDrawEOFMark(hdc, dot, i, dFlag);
        }
    }

    SelectFont(hdc, hFtOld);

    ViewDrawRuler(hdc);
    ViewDrawLineNumber(hdc);

    return S_OK;
}

static BOOLEAN ViewDrawTextOut(HDC hdc, INT dDot, UINT rdLine, LPLETTER pstTexts, UINT_PTR cchLen)
{
    UINT_PTR mz, cchMr;
    COLORREF clrTextOld, clrBackOld, clrMozi, clrRvsMozi, clrBack;
    INT dY;
    INT width, rdStart;
    LPTSTR ptText;
    UINT bStyle, cbSize;
    BOOLEAN bRslt, doDraw;
    RECT rect;
    HBRUSH hBrush;

    dY = rdLine * LINE_HEIGHT;

    cbSize = static_cast<UINT>((cchLen + 1) * sizeof(TCHAR));
    ptText = (LPTSTR)malloc(cbSize);
    if (!(ptText))
    {
        TRACE(TEXT("malloc error"));
        return FALSE;
    }
    ZeroMemory(ptText, cbSize);

    ViewGetTextColours(&clrMozi, &clrRvsMozi);
    ViewPrepareTextColours(hdc, clrMozi, &clrTextOld, &clrBackOld);

    bStyle = pstTexts[0].mzStyle;
    cchMr = 0;
    width = 0;
    rdStart = 0;
    doDraw = FALSE;

    ViewPositionTransform(&rdStart, &dY, 1);

    SetBkMode(hdc, TRANSPARENT);

    for (mz = 0; cchLen >= mz; mz++)
    {
        if (bStyle == pstTexts[mz].mzStyle)
        {
            ptText[cchMr++] = pstTexts[mz].cchMozi;
            width += pstTexts[mz].rdWidth;
        }
        else
        {
            doDraw = TRUE;
        }

        if (cchLen == mz)
        {
            doDraw = TRUE;
        }

        if (doDraw)
        {
            if (bStyle & CT_SPACE)
            {
                ViewDrawSpace(hdc, rdStart, dY, ptText, cchMr, bStyle);
            }
            else
            {
                hBrush = ViewHighlightBrushGet(bStyle, &clrBack);
                SetTextColor(hdc, (bStyle & CT_SELECT) ? clrRvsMozi : clrMozi);
                SetBkColor(hdc, clrBack);
                if (hBrush)
                {
                    SetRect(&rect, rdStart, dY, rdStart + width, dY + LINE_HEIGHT);
                    FillRect(hdc, &rect, hBrush);
                }

                bRslt = ExtTextOut(hdc, rdStart, dY, 0, nullptr, ptText, static_cast<UINT>(cchMr), nullptr);
                if (!(bRslt))
                {
                    TRACE(TEXT("ExtTextOut error"));
                    FREE(ptText);
                    return FALSE;
                }
            }

            rdStart += width;
            bStyle = pstTexts[mz].mzStyle;
            ZeroMemory(ptText, cbSize);
            ptText[0] = pstTexts[mz].cchMozi;
            width = pstTexts[mz].rdWidth;
            cchMr = 1;
            doDraw = FALSE;
        }
    }

    FREE(ptText);
    SetTextColor(hdc, clrTextOld);
    SetBkColor(hdc, clrBackOld);

    return TRUE;
}

static BOOLEAN ViewDrawSpace(HDC hdc, INT dX, UINT dY, LPTSTR ptText, UINT_PTR cchLen, UINT bStyle)
{
    const VIEWDISPLAYSTATE stDisplayState = ViewDisplayStateGet();
    HPEN hPenOld;
    INT width, xx, yy;
    UINT cchPos;
    SIZE stSize;
    RECT stRect;
    COLORREF clrBack;
    HBRUSH hBrush;

    xx = dX;
    yy = dY;
    dY += (LINE_HEIGHT - 2);

    hPenOld = ViewPrepareSpacePen(hdc, bStyle);

    GetTextExtentPoint32(hdc, ptText, static_cast<INT>(cchLen), &stSize);
    hBrush = ViewHighlightBrushGet(bStyle, &clrBack);
    if (hBrush)
    {
        SetRect(&stRect, xx, yy, xx + stSize.cx, yy + stSize.cy);
        if ((hBrush != gahBrush[BRHT_NOSJISBK]) || stDisplayState.dSpaceView)
            FillRect(hdc, &stRect, hBrush);
    }

    if (stDisplayState.dSpaceView || (bStyle & CT_WARNING))
    {
        for (cchPos = 0; cchLen > cchPos; cchPos++)
        {
            if (TEXT(' ') == ptText[cchPos])
            {
                width = SPACE_HAN;
            }
            else if (TEXT('　') == ptText[cchPos])
            {
                width = SPACE_ZEN;
            }
            else
            {
                width = ViewLetterWidthGet(ptText[cchPos]);
            }

            MoveToEx(hdc, dX, dY, nullptr);
            LineTo(hdc, (dX + width - 1), dY);
            dX += width;
        }
    }

    SelectPen(hdc, hPenOld);

    return TRUE;
}

static HRESULT ViewDrawMetricLine(HDC hdc, UINT bUpper)
{
    const VIEWDISPLAYSTATE stDisplayState = ViewDisplayStateGet();
    HPEN hPenOld;
    INT dX, dY;
    INT aX, aY;
    LONG width, height;

    UNREFERENCED_PARAMETER(bUpper);

    width = gstViewArea.cx + LINENUM_WID;
    height = gstViewArea.cy + RULER_AREA;

    if (stDisplayState.bGridView)
    {
        hPenOld = SelectPen(hdc, gahPen[PENT_GRID_LINE]);

        aX = stDisplayState.dGridXpos;
        aY = stDisplayState.dGridYpos;
        ViewPositionTransform(&aX, &aY, 1);

        while (height > aY)
        {
            MoveToEx(hdc, LINENUM_WID, aY, nullptr);
            LineTo(hdc, width, aY);
            aY += stDisplayState.dGridYpos;
        }

        while (width > aX)
        {
            MoveToEx(hdc, aX, RULER_AREA - 1, nullptr);
            LineTo(hdc, aX, height);
            aX += stDisplayState.dGridXpos;
        }

        SelectPen(hdc, hPenOld);
    }

    if (stDisplayState.bRightRulerView || stDisplayState.bUnderRulerView)
    {
        hPenOld = SelectPen(hdc, gahPen[PENT_SPACEWARN]);

        if (stDisplayState.bRightRulerView)
        {
            dX = stDisplayState.dRightRuler;
            dY = 0;
            ViewPositionTransform(&dX, &dY, 1);
            MoveToEx(hdc, dX, RULER_AREA - 1, nullptr);
            LineTo(hdc, dX, height);
        }

        if (stDisplayState.bUnderRulerView)
        {
            dX = 0;
            dY = stDisplayState.dUnderRuler * LINE_HEIGHT;
            ViewPositionTransform(&dX, &dY, 1);
            MoveToEx(hdc, LINENUM_WID, dY, nullptr);
            LineTo(hdc, width, dY);
        }

        SelectPen(hdc, hPenOld);
    }

    return S_OK;
}

static HRESULT ViewDrawReturnMark(HDC hdc, INT dDot, INT rdLine, UINT dFlag)
{
    HPEN hPenOld;
    INT dX, dY;
    INT aX, aY;
    COLORREF clrBackOld = 0;
    COLORREF clrBack;
    RECT rect;
    HBRUSH hBrush;

    dX = dDot;
    dY = rdLine * LINE_HEIGHT;
    ViewPositionTransform(&dX, &dY, 1);

    SetRect(&rect, dX, dY, dX + SPACE_ZEN, dY + LINE_HEIGHT);

    hBrush = ViewReturnMarkBrushGet(dFlag, &clrBack);
    clrBackOld = SetBkColor(hdc, clrBack);
    FillRect(hdc, &rect, hBrush);

    SetBkColor(hdc, clrBackOld);

    hPenOld = SelectPen(hdc, gahPen[PENT_CRLF_MARK]);
    aX = dX + 3;
    aY = dY + 3;
    MoveToEx(hdc, aX, aY, nullptr);
    LineTo(hdc, aX, aY + 12);
    LineTo(hdc, dX, aY + 9);
    MoveToEx(hdc, aX, aY + 12, nullptr);
    LineTo(hdc, aX + 3, aY + 9);

    SelectPen(hdc, hPenOld);

    return S_OK;
}

static INT ViewDrawEOFMark(HDC hdc, INT dDot, INT rdLine, UINT dFlag)
{
    INT dX, dY;
    COLORREF clrTextOld, clrBackOld = 0;
    RECT stClip;
    SIZE stSize;

    dX = dDot;
    dY = rdLine * LINE_HEIGHT;
    ViewPositionTransform(&dX, &dY, 1);

    clrTextOld = SetTextColor(hdc, gaColourTable[CLRT_EOF_MARK]);
    if (dFlag & CT_LASTSP)
    {
        clrBackOld = SetBkColor(hdc, gaColourTable[CLRT_LASTSPWARN]);
        SetBkMode(hdc, OPAQUE);
    }

    GetTextExtentPoint32(hdc, gatEOF, EOF_SIZE, &stSize);

    stClip.left = dX + 1;
    stClip.right = dX + 1 + stSize.cx;
    stClip.top = dY + 1;
    stClip.bottom = dY + LINE_HEIGHT;

    ExtTextOut(hdc, stClip.left, stClip.top, 0, &stClip, gatEOF, EOF_SIZE, nullptr);

    SetTextColor(hdc, clrTextOld);
    if (dFlag & CT_LASTSP)
    {
        SetBkColor(hdc, clrBackOld);
        SetBkMode(hdc, TRANSPARENT);
    }

    return stSize.cx;
}

HRESULT ViewRulerRedraw(INT iBgn, INT iEnd)
{
    RECT rect;

    GetClientRect(ghViewWnd, &rect);
    rect.bottom = RULER_AREA;

    if (0 <= iBgn)
    {
        rect.left = iBgn;
    }
    if (0 <= iEnd)
    {
        rect.right = iEnd;
    }

    InvalidateRect(ghViewWnd, &rect, TRUE);

    return S_OK;
}

static HRESULT ViewDrawRuler(HDC hdc)
{
    HPEN hPenOld;
    HFONT hFtOld;
    LONG width, pos, ln, start, dif, sbn, hei;
    TCHAR atStr[10];
    UINT_PTR count;
    RECT rect;
    auto caret = ViewCurrentCaret();
    auto origin = ViewCurrentOrigin();

    hPenOld = SelectPen(hdc, gahPen[PENT_RULER]);
    width = gstViewArea.cx + LINENUM_WID;

    SetBkMode(hdc, TRANSPARENT);

    SetRect(&rect, 0, 0, width, RULER_AREA);
    FillRect(hdc, &rect, gahBrush[BRHT_RULERBK]);

    MoveToEx(hdc, LINENUM_WID, RULER_AREA - 1, nullptr);
    LineTo(hdc, width, RULER_AREA - 1);

    start = origin.dHideXdot;

    dif = start % 10;
    sbn = start / 10;
    if (dif)
    {
        sbn++;
        dif = 10 - dif;
    }

    for (pos = 0, ln = sbn; width > pos; pos += 10, ln++)
    {
        hei = 6;
        if (!(ln % 5))
            hei = 3;
        if (!(ln % 10))
            hei = 0;
        MoveToEx(hdc, LINENUM_WID + pos + dif, hei, nullptr);
        LineTo(hdc, LINENUM_WID + pos + dif, RULER_AREA - 1);
    }

    SelectPen(hdc, hPenOld);

    hFtOld = SelectFont(hdc, ghRulerFont);

    dif = start % 100;
    if (dif)
        dif = 100 - dif;
    sbn = start / 100;
    if (dif)
        sbn++;
    sbn *= 100;

    for (pos = 0, ln = sbn; width > pos; pos += 100, ln += 100)
    {
        StringCchPrintf(atStr, 10, TEXT("%d"), ln);
        StringCchLength(atStr, 10, &count);
        ExtTextOut(hdc, LINENUM_WID + pos + 2 + dif, 0, 0, nullptr, atStr, static_cast<UINT>(count), nullptr);
    }

    SelectFont(hdc, hFtOld);

    hPenOld = SelectPen(hdc, gahPen[PENT_CARET_POS]);
    MoveToEx(hdc, LINENUM_WID + caret.dXdot, 1, nullptr);
    LineTo(hdc, LINENUM_WID + caret.dXdot, RULER_AREA - 1);
    SelectPen(hdc, hPenOld);

    if (1 <= gdAutoDiffBase)
    {
        hPenOld = SelectPen(hdc, gahPen[PENT_SPACEWARN]);
        MoveToEx(hdc, LINENUM_WID + gdAutoDiffBase, 1, nullptr);
        LineTo(hdc, LINENUM_WID + gdAutoDiffBase, RULER_AREA - 1);
        SelectPen(hdc, hPenOld);
    }

    return S_OK;
}

static HRESULT ViewDrawLineNumber(HDC hdc)
{
    HPEN hPenOld;
    HFONT hFtOld;
    LONG height, num, hei;
    INT dTextLeft;
    TCHAR atStr[10];
    UINT_PTR count;
    UINT figure = 4;
    RECT rect;
    SIZE stTextSize;
    auto origin = ViewCurrentOrigin();

    hPenOld = SelectPen(hdc, gahPen[PENT_RULER]);
    height = gstViewArea.cy + RULER_AREA;

    SetBkMode(hdc, TRANSPARENT);

    SetRect(&rect, 0, 0, LINENUM_WID - 1, height);
    FillRect(hdc, &rect, gahBrush[BRHT_RULERBK]);

    MoveToEx(hdc, LINENUM_WID - 2, RULER_AREA - 1, nullptr);
    LineTo(hdc, LINENUM_WID - 2, height);

    SelectPen(hdc, hPenOld);

    num = origin.dTopLine;
    if (9999 > num)
    {
        figure = 1;
        hFtOld = SelectFont(hdc, ghNumFont4L);
    }
    else if (9999 <= num && num < 99999)
    {
        figure = 3;
        hFtOld = SelectFont(hdc, ghNumFont5L);
    }
    else
    {
        figure = 5;
        hFtOld = SelectFont(hdc, ghNumFont6L);
    }

    for (hei = 0; height > hei; hei += LINE_HEIGHT, num++)
    {
        if (1 == figure && 9999 <= num)
        {
            figure = 3;
            SelectFont(hdc, ghNumFont5L);
        }

        if (3 == figure && 99999 <= num)
        {
            figure = 5;
            SelectFont(hdc, ghNumFont6L);
        }

        if (DocBadSpaceIsExist(num))
        {
            SetRect(&rect, 0, hei + RULER_AREA, LINENUM_WID - 2, hei + RULER_AREA + LINE_HEIGHT);
            FillRect(hdc, &rect, gahBrush[BRHT_LASTSPWARN]);
        }

        switch (figure)
        {
        default:
        case 1:
            StringCchPrintf(atStr, 10, TEXT("%4d"), num + 1);
            break;
        case 3:
            StringCchPrintf(atStr, 10, TEXT("%5d"), num + 1);
            break;
        case 5:
            StringCchPrintf(atStr, 10, TEXT("%6d"), num + 1);
            break;
        }

        StringCchLength(atStr, 10, &count);
        GetTextExtentPoint32(hdc, atStr, static_cast<INT>(count), &stTextSize);
        dTextLeft = LINENUM_WID - 4 - stTextSize.cx;
        if (0 > dTextLeft)
            dTextLeft = 0;
        ExtTextOut(hdc, dTextLeft, hei + RULER_AREA + figure, 0, nullptr, atStr, static_cast<UINT>(count), nullptr);
    }

    SelectFont(hdc, hFtOld);

    return S_OK;
}