#include "Sunday.h"
#include "AppModuleInternal.h"

extern HINSTANCE ghInst;
extern HWND ghViewWnd;

extern FILES_ITR gitFileIt; // 今見てるファイルの本体
extern INT gixFocusPage;    // 注目中のページ・とりあえず０・０インデックス

extern INT gdDocXdot; // キャレットのＸドット・ドキュメント位置
extern INT gdDocLine; // キャレットのＹ行数・ドキュメント位置
extern INT gdDocMozi; // キャレットの文字数・ドキュメント位置

struct COLOURTAGINPUT
{
    TCHAR atMain[SUB_STRING]{};
    TCHAR atShadow[SUB_STRING]{};
};

struct GRADIENTTAGINPUT
{
    TCHAR atStartMain[SUB_STRING]{};
    TCHAR atStartShadow[SUB_STRING]{};
    TCHAR atEndMain[SUB_STRING]{};
    TCHAR atEndShadow[SUB_STRING]{};
    TCHAR atStartLine[MIN_STRING]{};
    TCHAR atEndLine[MIN_STRING]{};
};

struct HEXCOLOUR
{
    BYTE r{};
    BYTE g{};
    BYTE b{};
    BYTE a{0xFF};
};

static COLOURTAGINPUT gstColourTagInput;
static GRADIENTTAGINPUT gstGradientTagInput;

static INT_PTR CALLBACK ColourTagDlgProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK GradientTagDlgProc(HWND, UINT, WPARAM, LPARAM);
static HRESULT ColourTagInsertFromDialog(HWND);
static HRESULT GradientTagInsertFromDialog(HWND);
static HRESULT InsertTagPairAtSelection(LPCTSTR, LPCTSTR);
static HRESULT InsertLineTagPair(INT, LPCTSTR, LPCTSTR, BOOLEAN);
static BOOLEAN HexColourTryParse(LPCTSTR, HEXCOLOUR *);
static VOID HexColourFormat(const HEXCOLOUR &, LPTSTR, UINT_PTR);
static HEXCOLOUR HexColourInterpolate(const HEXCOLOUR &, const HEXCOLOUR &, INT, INT);
static BOOLEAN GradientLineRangeTryGet(HWND, PINT, PINT);
static BOOLEAN GradientTagInputIsValid(HWND);
static VOID GradientTagOkButtonUpdate(HWND);
static BOOLEAN ColourTagInputIsValid(HWND);
static VOID ColourTagOkButtonUpdate(HWND);
static BOOLEAN TextIsEmpty(LPCTSTR);
static BOOLEAN ColourTagBuildOpenTag(const HEXCOLOUR *, const HEXCOLOUR *, BOOLEAN, TCHAR *, UINT_PTR);

static BOOLEAN ColourSelectionSpanGet(INT dLine, PINT pdBeginDot, PINT pdEndDot)
{
    INT dDot;
    INT dBeginDot;
    INT dEndDot;
    LINE_ITR itLine;

    if (pdBeginDot)
        *pdBeginDot = 0;
    if (pdEndDot)
        *pdEndDot = 0;

    if (0 > dLine || DocNowFilePageLineCount() <= dLine)
        return FALSE;

    itLine = gitFileIt->vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, dLine);

    dDot = 0;
    dBeginDot = -1;
    dEndDot = 0;

    for (const auto &stLetter : itLine->vcLine)
    {
        if (CT_SELECT & stLetter.mzStyle)
        {
            if (0 > dBeginDot)
                dBeginDot = dDot;

            dEndDot = dDot + stLetter.rdWidth;
        }

        dDot += stLetter.rdWidth;
    }

    if (0 > dBeginDot)
        dBeginDot = 0;

    if (pdBeginDot)
        *pdBeginDot = dBeginDot;
    if (pdEndDot)
        *pdEndDot = dEndDot;

    return 0 < dEndDot || 0 < dBeginDot;
}
//-------------------------------------------------------------------------------------------------

static INT_PTR CALLBACK ColourTagDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    COLOURTAGINPUT *pstInput;
    INT id;
    INT code;

    switch (message)
    {
    case WM_INITDIALOG:
        pstInput = reinterpret_cast<COLOURTAGINPUT *>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pstInput));
        SetDlgItemText(hDlg, IDE_COLOUR_TAG_MAIN, pstInput->atMain);
        SetDlgItemText(hDlg, IDE_COLOUR_TAG_SHADOW, pstInput->atShadow);
        ColourTagOkButtonUpdate(hDlg);
        SetFocus(GetDlgItem(hDlg, IDE_COLOUR_TAG_MAIN));
        return (INT_PTR)FALSE;

    case WM_COMMAND:
        id = LOWORD(wParam);
        code = HIWORD(wParam);

        if (AppModelessEditCommandForward(hDlg, id))
            return (INT_PTR)TRUE;

        switch (id)
        {
        case IDB_CANCEL:
        case IDCANCEL:
            DestroyWindow(hDlg);
            return (INT_PTR)TRUE;

        case IDB_OK:
        case IDOK:
            if (SUCCEEDED(ColourTagInsertFromDialog(hDlg)))
                DestroyWindow(hDlg);
            return (INT_PTR)TRUE;
        }

        if (EN_CHANGE == code)
        {
            ColourTagOkButtonUpdate(hDlg);
            return (INT_PTR)TRUE;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return (INT_PTR)TRUE;

    case WM_DESTROY:
        ghColourTagDlg = nullptr;
        return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

static HRESULT ColourTagInsertFromDialog(HWND hDlg)
{
    COLOURTAGINPUT *pstInput;
    BOOLEAN bHasShadow;
    TCHAR atString[MAX_STRING];
    HRESULT hr;

    pstInput = reinterpret_cast<COLOURTAGINPUT *>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!(pstInput))
        return E_POINTER;

    GetDlgItemText(hDlg, IDE_COLOUR_TAG_MAIN, pstInput->atMain, SUB_STRING);
    GetDlgItemText(hDlg, IDE_COLOUR_TAG_SHADOW, pstInput->atShadow, SUB_STRING);

    if (TextIsEmpty(pstInput->atMain))
        return E_INVALIDARG;

    bHasShadow = !(TextIsEmpty(pstInput->atShadow));
    if (bHasShadow)
        hr = StringCchPrintf(atString, _countof(atString), TEXT("[clr %s %s]"), pstInput->atMain, pstInput->atShadow);
    else
        hr = StringCchPrintf(atString, _countof(atString), TEXT("[clr %s]"), pstInput->atMain);
    if (FAILED(hr))
        return E_FAIL;

    return InsertTagPairAtSelection(atString, COLOUR_TAG_CLOCLR);
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN TextIsEmpty(LPCTSTR ptText)
{
    return (!(ptText) || 0 == ptText[0]);
}
//-------------------------------------------------------------------------------------------------

static INT HexDigitValue(TCHAR cchHex)
{
    if (TEXT('0') <= cchHex && TEXT('9') >= cchHex)
        return cchHex - TEXT('0');
    if (TEXT('a') <= cchHex && TEXT('f') >= cchHex)
        return 10 + (cchHex - TEXT('a'));
    if (TEXT('A') <= cchHex && TEXT('F') >= cchHex)
        return 10 + (cchHex - TEXT('A'));
    return -1;
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN HexByteTryParse(const TCHAR *ptText, BYTE *pbValue)
{
    INT hi;
    INT lo;

    if (!(ptText) || !(pbValue))
        return FALSE;

    hi = HexDigitValue(ptText[0]);
    lo = HexDigitValue(ptText[1]);
    if (0 > hi || 0 > lo)
        return FALSE;

    *pbValue = static_cast<BYTE>((hi << 4) | lo);
    return TRUE;
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN HexColourTryParse(LPCTSTR ptText, HEXCOLOUR *pstColour)
{
    size_t cchText;
    LPCTSTR ptDigits;
    HEXCOLOUR stColour;

    if (!(ptText) || !(pstColour))
        return FALSE;

    ptDigits = ptText;
    if (TEXT('#') == ptDigits[0])
        ptDigits++;

    cchText = 0;
    if (FAILED(StringCchLength(ptDigits, STRSAFE_MAX_CCH, &cchText)))
        return FALSE;

    if (!(6 == cchText || 8 == cchText))
        return FALSE;

    if (!(HexByteTryParse(ptDigits + 0, &stColour.r))
        || !(HexByteTryParse(ptDigits + 2, &stColour.g))
        || !(HexByteTryParse(ptDigits + 4, &stColour.b)))
    {
        return FALSE;
    }

    stColour.a = 0xFF;
    if (8 == cchText && !(HexByteTryParse(ptDigits + 6, &stColour.a)))
        return FALSE;

    *pstColour = stColour;
    return TRUE;
}
//-------------------------------------------------------------------------------------------------

static VOID HexColourFormat(const HEXCOLOUR &stColour, LPTSTR ptBuffer, UINT_PTR cchBuffer)
{
    if (0xFF == stColour.a)
        StringCchPrintf(ptBuffer, cchBuffer, TEXT("#%02x%02x%02x"), stColour.r, stColour.g, stColour.b);
    else
        StringCchPrintf(ptBuffer, cchBuffer, TEXT("#%02x%02x%02x%02x"), stColour.r, stColour.g, stColour.b, stColour.a);
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN ColourTagBuildOpenTag(const HEXCOLOUR *pstMain, const HEXCOLOUR *pstShadow, BOOLEAN bHasShadow, TCHAR *ptBuffer, UINT_PTR cchBuffer)
{
    TCHAR atMainHex[16];
    TCHAR atShadowHex[16];
    HRESULT hr;

    if (!(pstMain) || !(ptBuffer) || 0 >= cchBuffer)
        return FALSE;

    HexColourFormat(*pstMain, atMainHex, _countof(atMainHex));
    if (!(bHasShadow))
    {
        hr = StringCchPrintf(ptBuffer, cchBuffer, TEXT("[clr %s]"), atMainHex);
        return SUCCEEDED(hr);
    }

    if (!(pstShadow))
        return FALSE;

    HexColourFormat(*pstShadow, atShadowHex, _countof(atShadowHex));
    hr = StringCchPrintf(ptBuffer, cchBuffer, TEXT("[clr %s %s]"), atMainHex, atShadowHex);
    return SUCCEEDED(hr);
}
//-------------------------------------------------------------------------------------------------

static BYTE HexColourLerpByte(BYTE byStart, BYTE byEnd, INT iIndex, INT iLast)
{
    INT iDelta;
    INT iValue;

    if (0 >= iLast)
        return byStart;

    iDelta = static_cast<INT>(byEnd) - static_cast<INT>(byStart);
    iValue = static_cast<INT>(byStart) + ((iDelta * iIndex + (iDelta >= 0 ? iLast / 2 : -(iLast / 2))) / iLast);
    if (0 > iValue)
        iValue = 0;
    if (255 < iValue)
        iValue = 255;
    return static_cast<BYTE>(iValue);
}
//-------------------------------------------------------------------------------------------------

static HEXCOLOUR HexColourInterpolate(const HEXCOLOUR &stStart, const HEXCOLOUR &stEnd, INT iIndex, INT iLast)
{
    HEXCOLOUR stResult;

    stResult.r = HexColourLerpByte(stStart.r, stEnd.r, iIndex, iLast);
    stResult.g = HexColourLerpByte(stStart.g, stEnd.g, iIndex, iLast);
    stResult.b = HexColourLerpByte(stStart.b, stEnd.b, iIndex, iLast);
    stResult.a = HexColourLerpByte(stStart.a, stEnd.a, iIndex, iLast);

    return stResult;
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN GradientLineRangeTryGet(HWND hDlg, PINT piStartLine, PINT piEndLine)
{
    BOOL bStartOk;
    BOOL bEndOk;
    UINT uStartLine;
    UINT uEndLine;
    INT iLineCount;

    if (!(piStartLine) || !(piEndLine))
        return FALSE;

    uStartLine = GetDlgItemInt(hDlg, IDE_GRADIENT_GSTART_LINE, &bStartOk, FALSE);
    uEndLine = GetDlgItemInt(hDlg, IDE_GRADIENT_GEND_LINE, &bEndOk, FALSE);
    if (!(bStartOk) || !(bEndOk) || 0 == uStartLine || 0 == uEndLine)
        return FALSE;

    iLineCount = static_cast<INT>(DocNowFilePageLineCount());
    if (static_cast<INT>(uStartLine) > iLineCount || static_cast<INT>(uEndLine) > iLineCount)
        return FALSE;
    if (uStartLine > uEndLine)
        return FALSE;

    *piStartLine = static_cast<INT>(uStartLine) - 1;
    *piEndLine = static_cast<INT>(uEndLine) - 1;
    return TRUE;
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN GradientTagInputIsValid(HWND hDlg)
{
    TCHAR atBuffer[SUB_STRING];
    HEXCOLOUR stDummy;
    BOOLEAN bHasStartShadow;
    BOOLEAN bHasEndShadow;
    INT iStartLine;
    INT iEndLine;

    GetDlgItemText(hDlg, IDE_GRADIENT_GSTART_MAIN, atBuffer, _countof(atBuffer));
    if (!(HexColourTryParse(atBuffer, &stDummy)))
        return FALSE;

    GetDlgItemText(hDlg, IDE_GRADIENT_GEND_MAIN, atBuffer, _countof(atBuffer));
    if (!(HexColourTryParse(atBuffer, &stDummy)))
        return FALSE;

    GetDlgItemText(hDlg, IDE_GRADIENT_GSTART_SHADOW, atBuffer, _countof(atBuffer));
    bHasStartShadow = !(TextIsEmpty(atBuffer));
    if (bHasStartShadow && !(HexColourTryParse(atBuffer, &stDummy)))
        return FALSE;

    GetDlgItemText(hDlg, IDE_GRADIENT_GEND_SHADOW, atBuffer, _countof(atBuffer));
    bHasEndShadow = !(TextIsEmpty(atBuffer));
    if (bHasEndShadow && !(HexColourTryParse(atBuffer, &stDummy)))
        return FALSE;

    if (bHasStartShadow != bHasEndShadow)
        return FALSE;

    return GradientLineRangeTryGet(hDlg, &iStartLine, &iEndLine);
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN ColourTagInputIsValid(HWND hDlg)
{
    TCHAR atBuffer[SUB_STRING];

    GetDlgItemText(hDlg, IDE_COLOUR_TAG_MAIN, atBuffer, _countof(atBuffer));
    return !(TextIsEmpty(atBuffer));
}
//-------------------------------------------------------------------------------------------------

static VOID ColourTagOkButtonUpdate(HWND hDlg)
{
    EnableWindow(GetDlgItem(hDlg, IDB_OK), ColourTagInputIsValid(hDlg));
}
//-------------------------------------------------------------------------------------------------

static VOID GradientTagOkButtonUpdate(HWND hDlg)
{
    EnableWindow(GetDlgItem(hDlg, IDB_OK), GradientTagInputIsValid(hDlg));
}
//-------------------------------------------------------------------------------------------------

static INT_PTR CALLBACK GradientTagDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    GRADIENTTAGINPUT *pstInput;
    INT id;
    INT code;

    switch (message)
    {
    case WM_INITDIALOG:
        pstInput = reinterpret_cast<GRADIENTTAGINPUT *>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pstInput));
        SetDlgItemText(hDlg, IDE_GRADIENT_GSTART_MAIN, pstInput->atStartMain);
        SetDlgItemText(hDlg, IDE_GRADIENT_GSTART_SHADOW, pstInput->atStartShadow);
        SetDlgItemText(hDlg, IDE_GRADIENT_GEND_MAIN, pstInput->atEndMain);
        SetDlgItemText(hDlg, IDE_GRADIENT_GEND_SHADOW, pstInput->atEndShadow);
        SetDlgItemText(hDlg, IDE_GRADIENT_GSTART_LINE, pstInput->atStartLine);
        SetDlgItemText(hDlg, IDE_GRADIENT_GEND_LINE, pstInput->atEndLine);
        GradientTagOkButtonUpdate(hDlg);
        SetFocus(GetDlgItem(hDlg, IDE_GRADIENT_GSTART_MAIN));
        return (INT_PTR)FALSE;

    case WM_COMMAND:
        id = LOWORD(wParam);
        code = HIWORD(wParam);

        if (AppModelessEditCommandForward(hDlg, id))
            return (INT_PTR)TRUE;

        switch (id)
        {
        case IDB_CANCEL:
        case IDCANCEL:
            DestroyWindow(hDlg);
            return (INT_PTR)TRUE;

        case IDB_OK:
        case IDOK:
            if (SUCCEEDED(GradientTagInsertFromDialog(hDlg)))
                DestroyWindow(hDlg);
            return (INT_PTR)TRUE;
        }

        if (EN_CHANGE == code)
        {
            GradientTagOkButtonUpdate(hDlg);
            return (INT_PTR)TRUE;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return (INT_PTR)TRUE;

    case WM_DESTROY:
        ghGradientTagDlg = nullptr;
        return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

static HRESULT InsertLineTagPair(INT iLine, LPCTSTR ptOpenTag, LPCTSTR ptCloseTag, BOOLEAN bFirst)
{
    INT iDot;
    INT iWorkLine;

    if (!(ptOpenTag) || !(ptCloseTag))
        return E_INVALIDARG;
    if (0 > iLine || DocNowFilePageLineCount() <= static_cast<UINT_PTR>(iLine))
        return E_INVALIDARG;

    iDot = DocLineParamGet(iLine, nullptr, nullptr);
    iWorkLine = iLine;
    DocInsertString(&iDot, &iWorkLine, nullptr, ptCloseTag, 0, bFirst);

    iDot = 0;
    iWorkLine = iLine;
    DocInsertString(&iDot, &iWorkLine, nullptr, ptOpenTag, 0, FALSE);
    return S_OK;
}
//-------------------------------------------------------------------------------------------------

static HRESULT GradientTagInsertFromDialog(HWND hDlg)
{
    GRADIENTTAGINPUT *pstInput;
    HEXCOLOUR stStartMain;
    HEXCOLOUR stStartShadow;
    HEXCOLOUR stEndMain;
    HEXCOLOUR stEndShadow;
    INT iStartLine;
    INT iEndLine;
    INT iLine;
    INT iLineCount;
    INT iCaretDot;
    INT iCaretLine;
    TCHAR atOpenTag[MAX_STRING];
    BOOLEAN bHasShadow;
    BOOLEAN bFirst;

    pstInput = reinterpret_cast<GRADIENTTAGINPUT *>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!(pstInput))
        return E_POINTER;

    GetDlgItemText(hDlg, IDE_GRADIENT_GSTART_MAIN, pstInput->atStartMain, SUB_STRING);
    GetDlgItemText(hDlg, IDE_GRADIENT_GSTART_SHADOW, pstInput->atStartShadow, SUB_STRING);
    GetDlgItemText(hDlg, IDE_GRADIENT_GEND_MAIN, pstInput->atEndMain, SUB_STRING);
    GetDlgItemText(hDlg, IDE_GRADIENT_GEND_SHADOW, pstInput->atEndShadow, SUB_STRING);
    GetDlgItemText(hDlg, IDE_GRADIENT_GSTART_LINE, pstInput->atStartLine, MIN_STRING);
    GetDlgItemText(hDlg, IDE_GRADIENT_GEND_LINE, pstInput->atEndLine, MIN_STRING);

    bHasShadow = !(TextIsEmpty(pstInput->atStartShadow));

    if (!(HexColourTryParse(pstInput->atStartMain, &stStartMain))
        || !(HexColourTryParse(pstInput->atEndMain, &stEndMain))
        || !(GradientLineRangeTryGet(hDlg, &iStartLine, &iEndLine)))
    {
        return E_INVALIDARG;
    }

    if (bHasShadow)
    {
        if (!(HexColourTryParse(pstInput->atStartShadow, &stStartShadow))
            || !(HexColourTryParse(pstInput->atEndShadow, &stEndShadow)))
        {
            return E_INVALIDARG;
        }
    }

    iCaretDot = gdDocXdot;
    iCaretLine = gdDocLine;
    iLineCount = iEndLine - iStartLine;
    bFirst = TRUE;

    for (iLine = iStartLine; iEndLine >= iLine; iLine++)
    {
        HEXCOLOUR stMainColour;
        HEXCOLOUR stShadowColour{};

        stMainColour = HexColourInterpolate(stStartMain, stEndMain, iLine - iStartLine, iLineCount);
        if (bHasShadow)
            stShadowColour = HexColourInterpolate(stStartShadow, stEndShadow, iLine - iStartLine, iLineCount);

        if (!(ColourTagBuildOpenTag(&stMainColour, bHasShadow ? &stShadowColour : nullptr, bHasShadow, atOpenTag, _countof(atOpenTag))))
            return E_FAIL;

        InsertLineTagPair(iLine, atOpenTag, COLOUR_TAG_CLOCLR, bFirst);
        bFirst = FALSE;
    }

    ViewPosResetCaret(iCaretDot, iCaretLine);
    ViewRedrawSetLine(-1);
    DocPageInfoRenew(-1, 1);
    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 선택 범위의 앞뒤에 태그 쌍을 삽입한다.
static HRESULT InsertTagPairAtSelection(LPCTSTR ptOpenTag, LPCTSTR ptCloseTag)
{
    INT iTop;
    INT iBtm;
    INT i;
    INT iBeginDot;
    INT iEndDot;
    INT iCaretDot;
    INT iCaretLine;
    size_t cchOpen;
    BOOLEAN bFirst;

    if (!(ptOpenTag) || !(ptCloseTag))
        return E_INVALIDARG;

    iCaretDot = gdDocXdot;
    iCaretLine = gdDocLine;
    bFirst = TRUE;
    cchOpen = 0;
    StringCchLength(ptOpenTag, STRSAFE_MAX_CCH, &cchOpen);

    DocSelRangeGet(&iTop, &iBtm);
    if (0 > iTop || 0 > iBtm)
    {
        INT iDot;
        INT iLine;

        iDot = gdDocXdot;
        iLine = gdDocLine;
        DocInsertString(&iDot, &iLine, nullptr, ptCloseTag, 0, TRUE);

        iDot = gdDocXdot;
        iLine = gdDocLine;
        DocInsertString(&iDot, &iLine, nullptr, ptOpenTag, 0, FALSE);

        ViewPosResetCaret(iCaretDot + (INT)cchOpen, iCaretLine);
        ViewRedrawSetLine(-1);
        DocPageInfoRenew(-1, 1);
        return S_OK;
    }

    for (i = iTop; iBtm >= i; i++)
    {
        ColourSelectionSpanGet(i, &iBeginDot, &iEndDot);
        if (iTop != iBtm && 0 == iBeginDot && 0 == iEndDot)
            iEndDot = DocLineParamGet(i, nullptr, nullptr);

        {
            INT iDot = iEndDot;
            INT iLine = i;
            DocInsertString(&iDot, &iLine, nullptr, ptCloseTag, 0, bFirst);
            bFirst = FALSE;
        }

        {
            INT iDot = iBeginDot;
            INT iLine = i;
            DocInsertString(&iDot, &iLine, nullptr, ptOpenTag, 0, FALSE);
        }
    }

    ViewSelPageAll(-1);
    ViewPosResetCaret(iCaretDot, iCaretLine);
    ViewRedrawSetLine(-1);
    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 색상 태그 삽입 또는 감싸기
INT ViewInsertColourTag(UINT dCommando)
{
    if (IDM_INSTAG_COLOUR == dCommando)
    {
        if (ghColourTagDlg != nullptr)
        {
            ShowWindow(ghColourTagDlg, SW_SHOW);
            SetForegroundWindow(ghColourTagDlg);
            SetFocus(GetDlgItem(ghColourTagDlg, IDE_COLOUR_TAG_MAIN));
            return 1;
        }

        ghColourTagDlg = CreateDialogParam(ghInst, MAKEINTRESOURCE(IDD_COLOUR_TAG_DLG), ghViewWnd, ColourTagDlgProc, reinterpret_cast<LPARAM>(&gstColourTagInput));
        if (!(ghColourTagDlg))
            return 0;

        ShowWindow(ghColourTagDlg, SW_SHOW);
        return 1;
    }

    if (IDM_INSTAG_GRADIENT == dCommando)
    {
        if (ghGradientTagDlg != nullptr)
        {
            ShowWindow(ghGradientTagDlg, SW_SHOW);
            SetForegroundWindow(ghGradientTagDlg);
            SetFocus(GetDlgItem(ghGradientTagDlg, IDE_GRADIENT_GSTART_MAIN));
            return 1;
        }

        ghGradientTagDlg = CreateDialogParam(ghInst, MAKEINTRESOURCE(IDD_GRADIENT_TAG_DLG), ghViewWnd, GradientTagDlgProc, reinterpret_cast<LPARAM>(&gstGradientTagInput));
        if (!(ghGradientTagDlg))
            return 0;

        ShowWindow(ghGradientTagDlg, SW_SHOW);
        return 1;
    }

    return 0;
}
//-------------------------------------------------------------------------------------------------

HRESULT ColourTagDialogueOpen(HINSTANCE hInst, HWND hWnd)
{
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hWnd);

    return ViewInsertColourTag(IDM_INSTAG_COLOUR) ? S_OK : E_FAIL;
}
//-------------------------------------------------------------------------------------------------

// spo 태그를 선택 범위의 양옆에 삽입한다.
HRESULT DocInsertSpoTag(UINT dStyle)
{
    UNREFERENCED_PARAMETER(dStyle);

    return InsertTagPairAtSelection(COLOUR_TAG_WHITE, COLOUR_TAG_CLOSPO);
}