#include "ViewCentralInternal.h"

static INT_PTR CALLBACK ColourEditDlgProc(HWND, UINT, WPARAM, LPARAM);
static UINT ColourEditChoose(HWND, LPCOLORREF);
static INT_PTR ColourEditDrawItem(HWND, CONST LPDRAWITEMSTRUCT, LPCOLOUROBJECT);

HRESULT ViewColourEditDlg(HWND hWnd)
{
    INT_PTR iRslt;
    COLORREF cadColourSet[5];

    cadColourSet[0] = gaColourTable[CLRT_BASICPEN];
    cadColourSet[1] = gaColourTable[CLRT_BASICBK];
    cadColourSet[2] = gaColourTable[CLRT_GRID_LINE];
    cadColourSet[3] = gaColourTable[CLRT_CRLF_MARK];
    cadColourSet[4] = gaColourTable[CLRT_NOSJISBK];

    iRslt = DialogBoxParam(ghInst, MAKEINTRESOURCE(IDD_COLOUR_DLG), hWnd, ColourEditDlgProc, (LPARAM)cadColourSet);
    if (IDOK == iRslt)
    {
        gaColourTable[CLRT_BASICPEN] = cadColourSet[0];
        gaColourTable[CLRT_BASICBK] = cadColourSet[1];
        gaColourTable[CLRT_GRID_LINE] = cadColourSet[2];
        gaColourTable[CLRT_CRLF_MARK] = cadColourSet[3];
        gaColourTable[CLRT_NOSJISBK] = cadColourSet[4];

        DeleteBrush(gahBrush[BRHT_BASICBK]);
        DeletePen(gahPen[PENT_CRLF_MARK]);
        DeletePen(gahPen[PENT_GRID_LINE]);
        DeleteBrush(gahBrush[BRHT_NOSJISBK]);

        gahBrush[BRHT_BASICBK] = CreateSolidBrush(gaColourTable[CLRT_BASICBK]);
        gahPen[PENT_CRLF_MARK] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_CRLF_MARK]);
        gahPen[PENT_GRID_LINE] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_GRID_LINE]);
        gahBrush[BRHT_NOSJISBK] = CreateSolidBrush(gaColourTable[CLRT_NOSJISBK]);

        InitColourValue(INIT_SAVE, CLRV_BASICPEN, gaColourTable[CLRT_BASICPEN]);
        InitColourValue(INIT_SAVE, CLRV_BASICBK, gaColourTable[CLRT_BASICBK]);
        InitColourValue(INIT_SAVE, CLRV_GRIDLINE, gaColourTable[CLRT_GRID_LINE]);
        InitColourValue(INIT_SAVE, CLRV_CRLFMARK, gaColourTable[CLRT_CRLF_MARK]);
        InitColourValue(INIT_SAVE, CLRV_NOSJISBK, gaColourTable[CLRT_NOSJISBK]);

        SetClassLongPtr(ghViewWnd, GCLP_HBRBACKGROUND, (LONG_PTR)(gahBrush[BRHT_BASICBK]));
        InvalidateRect(ghViewWnd, nullptr, TRUE);
    }

    return S_OK;
}

static INT_PTR CALLBACK ColourEditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LPCOLORREF pcadColour;
    static COLOUROBJECT cstColours;
    INT id;

    switch (message)
    {
    case WM_INITDIALOG:
        pcadColour = (LPCOLORREF)lParam;
        cstColours.dTextColour = pcadColour[0];
        cstColours.hBackBrush = CreateSolidBrush(pcadColour[1]);
        cstColours.hGridPen = CreatePen(PS_SOLID, 1, pcadColour[2]);
        cstColours.hCrLfPen = CreatePen(PS_SOLID, 1, pcadColour[3]);
        cstColours.hUniBackBrs = CreateSolidBrush(pcadColour[4]);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        id = LOWORD(wParam);
        switch (id)
        {
        case IDOK:
        case IDCANCEL:
            DeleteBrush(cstColours.hBackBrush);
            DeletePen(cstColours.hGridPen);
            DeletePen(cstColours.hCrLfPen);
            DeleteBrush(cstColours.hUniBackBrs);
            EndDialog(hDlg, id);
            return (INT_PTR)TRUE;

        case IDB_COLOUR_BASIC_PEN:
            if (ColourEditChoose(hDlg, &(pcadColour[0])))
            {
                cstColours.dTextColour = pcadColour[0];
                InvalidateRect(GetDlgItem(hDlg, IDS_COLOUR_IMAGE), nullptr, TRUE);
            }
            return (INT_PTR)TRUE;

        case IDB_COLOUR_BASIC_BACK:
            if (ColourEditChoose(hDlg, &(pcadColour[1])))
            {
                DeleteBrush(cstColours.hBackBrush);
                cstColours.hBackBrush = CreateSolidBrush(pcadColour[1]);
                InvalidateRect(GetDlgItem(hDlg, IDS_COLOUR_IMAGE), nullptr, TRUE);
            }
            return (INT_PTR)TRUE;

        case IDB_COLOUR_GRID_LINE:
            if (ColourEditChoose(hDlg, &(pcadColour[2])))
            {
                DeletePen(cstColours.hGridPen);
                cstColours.hGridPen = CreatePen(PS_SOLID, 1, pcadColour[2]);
                InvalidateRect(GetDlgItem(hDlg, IDS_COLOUR_IMAGE), nullptr, TRUE);
            }
            return (INT_PTR)TRUE;

        case IDB_COLOUR_CRLF_MARK:
            if (ColourEditChoose(hDlg, &(pcadColour[3])))
            {
                DeletePen(cstColours.hCrLfPen);
                cstColours.hCrLfPen = CreatePen(PS_SOLID, 1, pcadColour[3]);
                InvalidateRect(GetDlgItem(hDlg, IDS_COLOUR_IMAGE), nullptr, TRUE);
            }
            return (INT_PTR)TRUE;

        case IDB_COLOUR_CANT_SJIS:
            if (ColourEditChoose(hDlg, &(pcadColour[4])))
            {
                DeleteBrush(cstColours.hUniBackBrs);
                cstColours.hUniBackBrs = CreateSolidBrush(pcadColour[4]);
                InvalidateRect(GetDlgItem(hDlg, IDS_COLOUR_IMAGE), nullptr, TRUE);
            }
            return (INT_PTR)TRUE;

        default:
            return (INT_PTR)FALSE;
        }

    case WM_DRAWITEM:
        return ColourEditDrawItem(hDlg, ((CONST LPDRAWITEMSTRUCT)(lParam)), &cstColours);
    }
    return (INT_PTR)FALSE;
}

static UINT ColourEditChoose(HWND hWnd, LPCOLORREF pdTgtColour)
{
    BOOL bRslt;
    COLORREF adColourTemp[16];
    CHOOSECOLOR stChColour;

    ZeroMemory(adColourTemp, sizeof(adColourTemp));
    adColourTemp[0] = *pdTgtColour;

    ZeroMemory(&stChColour, sizeof(CHOOSECOLOR));
    stChColour.lStructSize = sizeof(CHOOSECOLOR);
    stChColour.hwndOwner = hWnd;
    stChColour.rgbResult = *pdTgtColour;
    stChColour.lpCustColors = adColourTemp;
    stChColour.Flags = CC_RGBINIT;

    bRslt = ChooseColor(&stChColour);
    if (bRslt)
    {
        *pdTgtColour = stChColour.rgbResult;
    }

    return bRslt;
}

static INT_PTR ColourEditDrawItem(HWND hDlg, CONST LPDRAWITEMSTRUCT pstDrawItem, LPCOLOUROBJECT pstColours)
{
    static constexpr TCHAR kColourSampleText[] = TEXT("フランちゃんウフフ");
    static constexpr UINT_PTR kColourSampleLength = _countof(kColourSampleText) - 1;
    static constexpr TCHAR kColourUnicodeSampleText[] = {0x2600, 0x2006, 0x2665, 0x0000};
    static constexpr UINT_PTR kColourUnicodeSampleLength = _countof(kColourUnicodeSampleText) - 1;

    RECT rect;
    INT xpos;
    INT dotlen;
    INT dX = 6, dY = 6;
    INT aX, aY;
    HFONT hFtOld;
    HPEN hPenOld;

    UNREFERENCED_PARAMETER(hDlg);

    if (IDS_COLOUR_IMAGE != pstDrawItem->CtlID)
    {
        return (INT_PTR)FALSE;
    }

    GetClientRect(GetDlgItem(hDlg, IDS_COLOUR_IMAGE), &rect);

    hFtOld = SelectFont(pstDrawItem->hDC, ghAaFont);
    SetBkMode(pstDrawItem->hDC, TRANSPARENT);
    SetTextColor(pstDrawItem->hDC, pstColours->dTextColour);

    FillRect(pstDrawItem->hDC, &(pstDrawItem->rcItem), pstColours->hBackBrush);

    hPenOld = SelectPen(pstDrawItem->hDC, pstColours->hGridPen);
    for (xpos = 40; rect.right > xpos; xpos += 40)
    {
        MoveToEx(pstDrawItem->hDC, xpos, 0, nullptr);
        LineTo(pstDrawItem->hDC, xpos, rect.bottom);
    }

    dotlen = ViewStringWidthGet(kColourSampleText);
    ExtTextOut(pstDrawItem->hDC, dX, dY, 0, nullptr, kColourSampleText, static_cast<UINT>(kColourSampleLength), nullptr);
    dX += dotlen;

    SelectPen(pstDrawItem->hDC, pstColours->hCrLfPen);
    aX = dX + 3;
    aY = dY + 3;
    MoveToEx(pstDrawItem->hDC, aX, aY, nullptr);
    LineTo(pstDrawItem->hDC, aX, aY + 12);
    LineTo(pstDrawItem->hDC, dX, aY + 9);
    MoveToEx(pstDrawItem->hDC, aX, aY + 12, nullptr);
    LineTo(pstDrawItem->hDC, aX + 3, aY + 9);

    dX = 6;
    dY += LINE_HEIGHT;
    dotlen = ViewStringWidthGet(kColourUnicodeSampleText);
    SetRect(&rect, dX, dY, dX + dotlen, dY + LINE_HEIGHT);
    FillRect(pstDrawItem->hDC, &rect, pstColours->hUniBackBrs);
    ExtTextOut(pstDrawItem->hDC, dX, dY, 0, nullptr, kColourUnicodeSampleText, static_cast<UINT>(kColourUnicodeSampleLength), nullptr);

    SelectPen(pstDrawItem->hDC, hPenOld);
    SelectFont(pstDrawItem->hDC, hFtOld);

    return (INT_PTR)TRUE;
}