#include "ViewCentralInternal.h"
#include "AppLayoutInternal.h"
#include "Palette.h"

#define EDIT_VIEW_CLASS TEXT("EDIT_VIEW")

static LRESULT CALLBACK ViewWndProc(HWND, UINT, WPARAM, LPARAM);
static BOOLEAN Evw_OnCreate(HWND, LPCREATESTRUCT);
static VOID Evw_OnCommand(HWND, INT, HWND, UINT);
static VOID Evw_OnPaint(HWND);
static VOID Evw_OnDestroy(HWND);
static VOID Evw_OnVScroll(HWND, HWND, UINT, INT);
static VOID Evw_OnHScroll(HWND, HWND, UINT, INT);
static VOID Evw_OnContextMenu(HWND, HWND, UINT, UINT);
static VOID ViewOpenInflatedFile(LPTSTR, PBOOLEAN);

HINSTANCE ghInst;
HWND ghPrntWnd;
HWND ghViewWnd;

extern HWND ghPgVwWnd;
extern HWND ghPalInsertWnd;
extern HWND ghPalBucketWnd;
extern BOOLEAN gbDockTmplView;

INT gdXmemory;
INT gdDocXdot;
INT gdDocLine;
INT gdDocMozi;

INT gdHideXdot;
INT gdViewTopLine;
SIZE gstViewArea;
INT gdDispingLine;

EXTERNED BOOLEAN gbExtract;

HFONT ghAaFont;
HFONT ghRulerFont;
HFONT ghNumFont4L;
HFONT ghNumFont5L;
HFONT ghNumFont6L;

INT gdAutoDiffBase;
UINT gdSpaceView;

BOOLEAN gbGridView;
EXTERNED UINT gdGridXpos;
EXTERNED UINT gdGridYpos;

BOOLEAN gbRitRlrView;
EXTERNED UINT gdRightRuler;

BOOLEAN gbUndRlrView;
EXTERNED UINT gdUnderRuler;

UINT gdWheelLine;
extern INT gixFocusPage;
extern INT gbTmpltDock;
extern HWND ghMainSplitWnd;
extern LONG grdSplitPos;

EXTERNED HDC hdcC;
EXTERNED HFONT hFtOldC;
EXTERNED BOOLEAN hdcCommonF = FALSE;

COLORREF gaColourTable[CLRT_COUNT] = {
    0x000000,
    0xFFFFFF, 0xABABAB, 0x0000FF, 0xAAAAAA, 0x000000,
    0xFFFFFF, 0x8080FF, 0xC0C000, 0xC0C000, 0x101010,
    0xEEEEEE, 0xFFCCCC, 0xFF0000, 0xE0E0E0, 0x00FFFF,
};

HPEN gahPen[PENT_COUNT];
HBRUSH gahBrush[BRHT_COUNT];

VOID AaFontCreate(UINT bMode)
{
    LOGFONT stFont;

    ViewingFontGet(&stFont);

    if (bMode)
    {
        ghAaFont = CreateFontIndirect(&stFont);
    }
    else
    {
        DeleteFont(ghAaFont);
    }
}

static VOID ViewOpenInflatedFile(LPTSTR ptFilePath, PBOOLEAN pbOpen)
{
    const LPARAM dNumber = DocFileInflate(ptFilePath);
    if (0 >= dNumber)
    {
        return;
    }

    if (!(*pbOpen))
    {
        MultiFileTabFirst(ptFilePath);
        *pbOpen = TRUE;
    }
    else
    {
        MultiFileTabAppend(dNumber, ptFilePath);
    }

    AppTitleChange(ptFilePath);
}

HWND ViewInitialise(HINSTANCE hInstance, HWND hParentWnd, LPRECT pstFrame, LPTSTR ptArgv)
{
    TCHAR atFile[MAX_PATH]{};
    WNDCLASSEX wcex{};
    RECT vwRect, rect;
    LOGFONT stFont;
    INT iFiles;
    BOOLEAN bOpen = FALSE;
    INT spPos, iOpMode, iRslt;
    auto caret = ViewCurrentCaret();
    auto origin = ViewCurrentOrigin();
    auto display = ViewDisplayStateGet();

    ghInst = hInstance;

    gdWheelLine = 0;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &gdWheelLine, 0);
    if (0 == gdWheelLine)
        gdWheelLine = 3;

    gaColourTable[CLRT_SELECTBK] = GetSysColor(COLOR_HIGHLIGHT);

    gaColourTable[CLRT_BASICPEN] = InitColourValue(INIT_LOAD, CLRV_BASICPEN, gaColourTable[CLRT_BASICPEN]);
    gaColourTable[CLRT_BASICBK] = InitColourValue(INIT_LOAD, CLRV_BASICBK, gaColourTable[CLRT_BASICBK]);
    gaColourTable[CLRT_GRID_LINE] = InitColourValue(INIT_LOAD, CLRV_GRIDLINE, gaColourTable[CLRT_GRID_LINE]);
    gaColourTable[CLRT_CRLF_MARK] = InitColourValue(INIT_LOAD, CLRV_CRLFMARK, gaColourTable[CLRT_CRLF_MARK]);
    gaColourTable[CLRT_NOSJISBK] = InitColourValue(INIT_LOAD, CLRV_NOSJISBK, gaColourTable[CLRT_NOSJISBK]);

    gahBrush[BRHT_BASICBK] = CreateSolidBrush(gaColourTable[CLRT_BASICBK]);
    gahBrush[BRHT_RULERBK] = CreateSolidBrush(gaColourTable[CLRT_RULERBK]);
    gahBrush[BRHT_SELECTBK] = CreateSolidBrush(gaColourTable[CLRT_SELECTBK]);
    gahBrush[BRHT_LASTSPWARN] = CreateSolidBrush(gaColourTable[CLRT_SPACEWARN]);
    gahBrush[BRHT_NOSJISBK] = CreateSolidBrush(gaColourTable[CLRT_NOSJISBK]);
    gahBrush[BRHT_FINDBACK] = CreateSolidBrush(gaColourTable[CLRT_FINDBACK]);

    gahPen[PENT_CRLF_MARK] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_CRLF_MARK]);
    gahPen[PENT_RULER] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_RULER]);
    gahPen[PENT_SPACEWARN] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_SPACEWARN]);
    gahPen[PENT_SPACELINE] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_SPACELINE]);
    gahPen[PENT_CARET_POS] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_CARET_POS]);
    gahPen[PENT_GRID_LINE] = CreatePen(PS_SOLID, 1, gaColourTable[CLRT_GRID_LINE]);

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = ViewWndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_IBEAM);
    wcex.hbrBackground = gahBrush[BRHT_BASICBK];
    wcex.lpszClassName = EDIT_VIEW_CLASS;

    RegisterClassEx(&wcex);

    origin.dHideXdot = 0;
    origin.dTopLine = 0;
    gdAutoDiffBase = 0;

    display.dGridXpos = InitParamValue(INIT_LOAD, VL_GRID_X_POS, 54);
    display.dGridYpos = InitParamValue(INIT_LOAD, VL_GRID_Y_POS, 54);
    display.bGridView = InitParamValue(INIT_LOAD, VL_GRID_VIEW, 0);
    MenuItemCheckOnOff(IDM_GRID_VIEW_TOGGLE, display.bGridView);

    display.dRightRuler = InitParamValue(INIT_LOAD, VL_R_RULER_POS, 800);
    display.bRightRulerView = InitParamValue(INIT_LOAD, VL_R_RULER_VIEW, 1);
    MenuItemCheckOnOff(IDM_RIGHT_RULER_TOGGLE, display.bRightRulerView);

    display.dUnderRuler = InitParamValue(INIT_LOAD, VL_U_RULER_POS, 30);
    display.bUnderRulerView = InitParamValue(INIT_LOAD, VL_U_RULER_VIEW, 1);
    MenuItemCheckOnOff(IDM_UNDER_RULER_TOGGLE, display.bUnderRulerView);

    display.dSpaceView = InitParamValue(INIT_LOAD, VL_SPACE_VIEW, TRUE);
    MenuItemCheckOnOff(IDM_SPACE_VIEW_TOGGLE, display.dSpaceView);
    OperationOnStatusBar();

    ghPrntWnd = hParentWnd;

    rect = *pstFrame;
    if (gbTmpltDock)
    {
        spPos = grdSplitPos;
        rect.right -= spPos;
    }

    ghViewWnd = CreateWindowEx(WS_EX_COMPOSITED, EDIT_VIEW_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        rect.left, rect.top, rect.right, rect.bottom, hParentWnd, nullptr, hInstance, nullptr);

    if (!ghViewWnd)
    {
        return nullptr;
    }

    ViewingFontGet(&stFont);
    stFont.lfPitchAndFamily = FIXED_PITCH;
    StringCchCopy(stFont.lfFaceName, LF_FACESIZE, TEXT("돋움"));
    ghNumFont4L = CreateFontIndirect(&stFont);

    stFont.lfHeight = 13;
    ghNumFont5L = CreateFontIndirect(&stFont);

    stFont.lfHeight = 11;
    ghNumFont6L = CreateFontIndirect(&stFont);

    stFont.lfHeight = FONTSZ_REDUCE;
    stFont.lfPitchAndFamily = DEFAULT_PITCH;
    StringCchCopy(stFont.lfFaceName, LF_FACESIZE, TEXT("돋움"));
    ghRulerFont = CreateFontIndirect(&stFont);

    GetClientRect(ghViewWnd, &vwRect);
    gstViewArea.cx = vwRect.right - LINENUM_WID;
    gstViewArea.cy = vwRect.bottom - RULER_AREA;
    gdDispingLine = gstViewArea.cy / LINE_HEIGHT;

    DocInitialise(TRUE);

    iOpMode = InitParamValue(INIT_LOAD, VL_LAST_OPEN, LASTOPEN_DO);
    if (LASTOPEN_ASK <= iOpMode)
    {
        iRslt = MessageBox(nullptr, TEXT("이전에 마지막으로 열었던 파일을 다시 열까요?"), TEXT("파일 열기 안내"), MB_YESNO | MB_ICONQUESTION);
        if (IDYES == iRslt)
            iOpMode = LASTOPEN_DO;
        else
            iOpMode = LASTOPEN_NON;
    }

    if (iOpMode)
    {
        iFiles = 0;
    }
    else
    {
        iFiles = InitMultiFileTabOpen(INIT_LOAD, -1, nullptr);
    }

    for (INT i = 0; iFiles >= i; i++)
    {
        if (iFiles == i)
        {
            StringCchCopy(atFile, MAX_PATH, ptArgv ? ptArgv : TEXT(""));
        }
        else
        {
            InitMultiFileTabOpen(INIT_LOAD, i, atFile);
        }

        ViewOpenInflatedFile(atFile, &bOpen);
    }

    if (!(bOpen))
    {
        DocActivateEmptyCreate(atFile);
    }

    ViewScrollBarAdjust(nullptr);

    ShowWindow(ghViewWnd, SW_SHOW);
    UpdateWindow(ghViewWnd);

    ViewCaretCreate(ghViewWnd, gaColourTable[CLRT_CARETFD], gaColourTable[CLRT_CARETBK]);

    caret.dXdot = 0;
    caret.dMozi = 0;
    caret.dLine = 0;
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);

    gdXmemory = 0;

    ViewNowPosStatus();

    return ghViewWnd;
}

COLORREF ViewMoziColourGet(LPCOLORREF pCrtColour)
{
    if (pCrtColour)
        *pCrtColour = gaColourTable[CLRT_CARETFD];
    return gaColourTable[CLRT_BASICPEN];
}

COLORREF ViewBackColourGet(LPVOID pVoid)
{
    UNREFERENCED_PARAMETER(pVoid);
    return gaColourTable[CLRT_BASICBK];
}

HRESULT ViewSizeMove(HWND hPrntWnd, LPRECT pstFrame)
{
    RECT rect;
    auto caret = ViewCurrentCaret();

    rect = *pstFrame;

    if (gbTmpltDock)
    {
        const APP_DOCK_LAYOUT stDockLayout = AppLayoutSplitBarResize(ghMainSplitWnd, pstFrame);
        const APP_DOCKED_WINDOW_LAYOUT stDockedWindows = AppLayoutDockedWindowsCalc(
            pstFrame,
            stDockLayout.dSplitPos,
            AppLayoutDockTabHeightGet(DockingTabGet()),
            gbDockTmplView);
        grdSplitPos = stDockLayout.dSplitPos;

        AppLayoutApplyDockedWindows(stDockedWindows, ghPgVwWnd, DockingTabGet(), ghPalInsertWnd, ghPalBucketWnd);

        rect.right -= grdSplitPos;
        InitParamValue(INIT_SAVE, VL_MAIN_SPLIT, grdSplitPos);
    }

    SetWindowPos(ghViewWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_SHOWWINDOW);

    GetClientRect(ghViewWnd, &rect);
    gstViewArea.cx = rect.right - LINENUM_WID;
    gstViewArea.cy = rect.bottom - RULER_AREA;
    gdDispingLine = gstViewArea.cy / LINE_HEIGHT;

    ViewScrollBarAdjust(nullptr);
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);

    return S_OK;
}

HRESULT ViewScrollBarAdjust(LPVOID pVoid)
{
    INT dMargin, dRange, dDot, dPos, dLines;
    auto origin = ViewCurrentOrigin();

    UNREFERENCED_PARAMETER(pVoid);

    dMargin = gstViewArea.cx / 2;

    dDot = DocPageMaxDotGet(-1, -1);
    dRange = dMargin + dDot;
    dRange -= gstViewArea.cx;

    if (0 >= dRange)
    {
        EnableScrollBar(ghViewWnd, SB_HORZ, ESB_DISABLE_BOTH);
        if (0 != origin.dHideXdot)
        {
            origin.dHideXdot = 0;
        }
    }
    else
    {
        EnableScrollBar(ghViewWnd, SB_HORZ, ESB_ENABLE_BOTH);
        SetScrollRange(ghViewWnd, SB_HORZ, 0, dRange, TRUE);
        dPos = origin.dHideXdot;
        if (0 > dPos)
            dPos = 0;
        SetScrollPos(ghViewWnd, SB_HORZ, dPos, TRUE);
    }

    dLines = DocNowFilePageLineCount();
    dRange = dLines - gdDispingLine;

    if (0 >= dRange)
    {
        EnableScrollBar(ghViewWnd, SB_VERT, ESB_DISABLE_BOTH);
        if (0 != origin.dTopLine)
        {
            origin.dTopLine = 0;
            ViewRedrawSetLine(-1);
        }
    }
    else
    {
        EnableScrollBar(ghViewWnd, SB_VERT, ESB_ENABLE_BOTH);
        SetScrollRange(ghViewWnd, SB_VERT, 0, dRange, TRUE);
        dPos = origin.dTopLine;
        if (0 > dPos)
            dPos = 0;
        SetScrollPos(ghViewWnd, SB_VERT, dPos, TRUE);
    }

    return S_OK;
}

HRESULT ViewPositionTransform(PINT pDotX, PINT pDotY, BOOLEAN bTrans)
{
    auto origin = ViewCurrentOrigin();

    assert(pDotX);
    assert(pDotY);

    if (bTrans)
    {
        *pDotX = *pDotX + LINENUM_WID;
        *pDotX = *pDotX - origin.dHideXdot;
        *pDotY = *pDotY + RULER_AREA;
        *pDotY = *pDotY - (origin.dTopLine * LINE_HEIGHT);
    }
    else
    {
        *pDotX = *pDotX + origin.dHideXdot;
        *pDotX = *pDotX - LINENUM_WID;
        *pDotY = *pDotY - RULER_AREA;
        *pDotY = *pDotY + (origin.dTopLine * LINE_HEIGHT);
    }

    return S_OK;
}

BOOLEAN ViewIsPosOnFrame(INT xx, INT yy)
{
    POINT stPoint;
    RECT stRect;

    SetRect(&stRect, 0, 0, gstViewArea.cx, gstViewArea.cy);
    stPoint.x = xx - LINENUM_WID;
    stPoint.y = yy - RULER_AREA;

    return PtInRect(&stRect, stPoint);
}

INT ViewAreaSizeGet(PINT pdXdot)
{
    if (pdXdot)
        *pdXdot = gstViewArea.cx;

    return gdDispingLine;
}

HRESULT ViewEditReset(VOID)
{
    auto caret = ViewCurrentCaret();
    auto origin = ViewCurrentOrigin();

    caret.dXdot = 0;
    caret.dLine = 0;
    caret.dMozi = 0;
    origin.dHideXdot = 0;
    origin.dTopLine = 0;

    ViewDrawCaret(0, 0, TRUE);
    ViewScrollBarAdjust(nullptr);
    ViewRedrawSetLine(-1);

    return S_OK;
}

static LRESULT CALLBACK ViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HIMC hImc;
    LOGFONT stFont;

    switch (message)
    {
        HANDLE_MSG(hWnd, WM_CREATE, Evw_OnCreate);
        HANDLE_MSG(hWnd, WM_PAINT, Evw_OnPaint);
        HANDLE_MSG(hWnd, WM_COMMAND, Evw_OnCommand);
        HANDLE_MSG(hWnd, WM_DESTROY, Evw_OnDestroy);
        HANDLE_MSG(hWnd, WM_VSCROLL, Evw_OnVScroll);
        HANDLE_MSG(hWnd, WM_HSCROLL, Evw_OnHScroll);
        HANDLE_MSG(hWnd, WM_KEYDOWN, Evw_OnKey);
        HANDLE_MSG(hWnd, WM_KEYUP, Evw_OnKey);
        HANDLE_MSG(hWnd, WM_CHAR, Evw_OnChar);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, Evw_OnMouseMove);
        HANDLE_MSG(hWnd, WM_MOUSEWHEEL, Evw_OnMouseWheel);
        HANDLE_MSG(hWnd, WM_LBUTTONDOWN, Evw_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_LBUTTONDBLCLK, Evw_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_LBUTTONUP, Evw_OnLButtonUp);
        HANDLE_MSG(hWnd, WM_RBUTTONDOWN, Evw_OnRButtonDown);
        HANDLE_MSG(hWnd, WM_CONTEXTMENU, Evw_OnContextMenu);

    case WM_SETFOCUS:
        TRACE(TEXT("VIEW_WM_SETFOCUS[0x%X][0x%X]"), wParam, lParam);
        ViewShowCaret();
        break;

    case WM_KILLFOCUS:
        TRACE(TEXT("VIEW_WM_KILLFOCUS[0x%X][0x%X]"), wParam, lParam);
        ViewHideCaret();
        break;

    case WM_ACTIVATE:
        TRACE(TEXT("VIEW_WM_ACTIVATE[0x%X][0x%X]"), wParam, lParam);
        if (WA_INACTIVE == LOWORD(wParam))
        {
            ViewHideCaret();
        }
        break;

    case WM_IME_NOTIFY:
        TRACE(TEXT("WM_IME_NOTIFY[0x%X][0x%X]"), wParam, lParam);
        break;

    case WM_IME_REQUEST:
        TRACE(TEXT("WM_IME_REQUEST[0x%X][0x%X]"), wParam, lParam);
        break;

    case WM_IME_STARTCOMPOSITION:
        TRACE(TEXT("WM_IME_STARTCOMPOSITION[0x%X][0x%X]"), wParam, lParam);
        hImc = ImmGetContext(ghViewWnd);
        if (hImc)
        {
            ViewingFontGet(&stFont);
            ImmSetCompositionFont(hImc, &stFont);
            ImmReleaseContext(ghViewWnd, hImc);
        }
        break;

    case WM_IME_ENDCOMPOSITION:
        TRACE(TEXT("WM_IME_ENDCOMPOSITION[0x%X][0x%X]"), wParam, lParam);
        break;

    case WM_IME_COMPOSITION:
        Evw_OnImeComposition(hWnd, wParam, lParam);
        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static BOOLEAN Evw_OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    HINSTANCE lcInst = lpCreateStruct->hInstance;

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(lcInst);

    return TRUE;
}

static VOID Evw_OnCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    UNREFERENCED_PARAMETER(hWnd);
    OperationOnCommand(ghPrntWnd, id, hWndCtl, codeNotify);
}

static VOID Evw_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hWnd, &ps);
    ViewRedrawDo(hWnd, hdc);
    EndPaint(hWnd, &ps);
}

static VOID Evw_OnDestroy(HWND hWnd)
{
    SetWindowFont(hWnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);
    DeleteFont(ghRulerFont);
    DeleteFont(ghNumFont4L);
    DeleteFont(ghNumFont5L);
    DeleteFont(ghNumFont6L);

    ViewCaretDelete();

    for (UINT i = 0; PENT_COUNT > i; i++)
    {
        DeletePen(gahPen[i]);
    }

    for (UINT i = 0; BRHT_COUNT > i; i++)
    {
        DeleteBrush(gahBrush[i]);
    }

    DocMultiFileCloseAll();
    PostQuitMessage(0);
}

static VOID Evw_OnHScroll(HWND hWnd, HWND hWndCtl, UINT code, INT pos)
{
    SCROLLINFO stScrollInfo{};
    auto caret = ViewCurrentCaret();
    auto origin = ViewCurrentOrigin();
    INT dDot = origin.dHideXdot;

    UNREFERENCED_PARAMETER(hWndCtl);
    UNREFERENCED_PARAMETER(pos);

    stScrollInfo.cbSize = sizeof(SCROLLINFO);
    stScrollInfo.fMask = SIF_ALL;
    GetScrollInfo(hWnd, SB_HORZ, &stScrollInfo);

    switch (code)
    {
    case SB_LINEUP:
        dDot--;
        break;
    case SB_PAGEUP:
        dDot -= gstViewArea.cx / 5;
        break;
    case SB_THUMBTRACK:
        dDot = stScrollInfo.nTrackPos;
        break;
    case SB_PAGEDOWN:
        dDot += gstViewArea.cx / 5;
        break;
    case SB_LINEDOWN:
        dDot++;
        break;
    default:
        return;
    }

    if (0 > dDot)
        dDot = 0;
    if (stScrollInfo.nMax < dDot)
        dDot = stScrollInfo.nMax;

    origin.dHideXdot = dDot;

    stScrollInfo.fMask = SIF_POS;
    stScrollInfo.nPos = dDot;
    SetScrollInfo(ghViewWnd, SB_HORZ, &stScrollInfo, TRUE);

    ViewRedrawSetLine(-1);
    ViewDrawCaret(caret.dXdot, caret.dLine, 0);
}

static VOID Evw_OnVScroll(HWND hWnd, HWND hWndCtl, UINT code, INT pos)
{
    SCROLLINFO stScrollInfo{};
    auto caret = ViewCurrentCaret();
    auto origin = ViewCurrentOrigin();
    INT dPos = origin.dTopLine, iLines, dPrev;

    UNREFERENCED_PARAMETER(hWndCtl);

    iLines = DocNowFilePageLineCount();
    if (gdDispingLine >= iLines)
        return;

    stScrollInfo.cbSize = sizeof(SCROLLINFO);
    stScrollInfo.fMask = SIF_ALL;
    GetScrollInfo(hWnd, SB_VERT, &stScrollInfo);

    TRACE(TEXT("code[%d] pos[%d] dPos[%d] InfoMax[%d]"), code, pos, dPos, stScrollInfo.nMax);

    dPrev = dPos;

    switch (code)
    {
    case SB_LINEUP:
        if (pos)
        {
            dPos = dPos - gdWheelLine;
        }
        else
        {
            dPos--;
        }
        break;
    case SB_PAGEUP:
        dPos -= gdDispingLine / 2;
        break;
    case SB_THUMBTRACK:
        dPos = stScrollInfo.nTrackPos;
        break;
    case SB_PAGEDOWN:
        dPos += gdDispingLine / 2;
        break;
    case SB_LINEDOWN:
        if (pos)
        {
            dPos = dPos + gdWheelLine;
        }
        else
        {
            dPos++;
        }
        break;
    default:
        return;
    }

    if (0 > dPos)
        dPos = 0;
    if (stScrollInfo.nMax < dPos)
        dPos = stScrollInfo.nMax;

    origin.dTopLine = dPos;

    stScrollInfo.fMask = SIF_POS;
    stScrollInfo.nPos = dPos;
    SetScrollInfo(ghViewWnd, SB_VERT, &stScrollInfo, TRUE);

    if (dPrev != dPos)
    {
        ViewRedrawSetLine(-1);
    }

    ViewDrawCaret(caret.dXdot, caret.dLine, 0);
}

static VOID Evw_OnContextMenu(HWND hWnd, HWND hWndContext, UINT xPos, UINT yPos)
{
    const INT posX = (SHORT)xPos;
    const INT posY = (SHORT)yPos;
    HMENU hSubMenu;
    UINT dRslt;

    TRACE(TEXT("VIEW_WM_CONTEXTMENU %d x %d"), posX, posY);

    const VIEWDISPLAYSTATE stDisplayState = ViewDisplayStateGet();
    const VIEWCOMMANDSTATE stCommandState = ViewCommandStateGet();

    hSubMenu = CntxMenuGet();

    CheckMenuItem(hSubMenu, IDM_SQSELECT, stCommandState.dSquareSelect ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSubMenu, IDM_SPACE_VIEW_TOGGLE, stDisplayState.dSpaceView ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSubMenu, IDM_GRID_VIEW_TOGGLE, stDisplayState.bGridView ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSubMenu, IDM_RIGHT_RULER_TOGGLE, stDisplayState.bRightRulerView ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSubMenu, IDM_UNDER_RULER_TOGGLE, stDisplayState.bUnderRulerView ? MF_CHECKED : MF_UNCHECKED);

    dRslt = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, posX, posY, 0, hWnd, nullptr);

    FORWARD_WM_COMMAND(ghViewWnd, dRslt, hWndContext, 0, PostMessage);
}

HRESULT ViewFocusSet(VOID)
{
    SetFocus(ghViewWnd);
    SetWindowPos(ghPrntWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    TRACE(TEXT("포커스 전환"));

    return S_OK;
}

HRESULT ViewNowPosStatus(VOID)
{
    TCHAR atString[SUB_STRING];
    auto caret = ViewCurrentCaret();

    StringCchPrintf(atString, SUB_STRING, TEXT("커서 %d번째 줄 %d도트 %d문자"), caret.dLine + 1, caret.dXdot, caret.dMozi);
    MainStatusBarSetText(SB_CURSOR, atString);

    return S_OK;
}

INT ViewLetterWidthGet(TCHAR ch)
{
    if (hdcCommonF)
    {
        return ViewLetterWidthGetC(ch);
    }

    SIZE stSize;
    HDC hdc = GetDC(ghViewWnd);
    HFONT hFtOld;

    hFtOld = SelectFont(hdc, ViewAaFontGet());
    GetTextExtentPoint32(hdc, &ch, 1, &stSize);
    SelectFont(hdc, hFtOld);
    ReleaseDC(ghViewWnd, hdc);

    return stSize.cx;
}

INT GetHdcC()
{
    hdcCommonF = TRUE;
    hdcC = GetDC(ghViewWnd);
    hFtOldC = SelectFont(hdcC, ViewAaFontGet());
    return 0;
}

INT ReleaseHdcC()
{
    SelectFont(hdcC, hFtOldC);
    ReleaseDC(ghViewWnd, hdcC);
    hdcCommonF = FALSE;
    return 0;
}

INT ViewLetterWidthGetC(TCHAR ch)
{
    SIZE stSize;

    GetTextExtentPoint32(hdcC, &ch, 1, &stSize);

    return stSize.cx;
}

INT ViewStringWidthGet(LPCTSTR ptStr)
{
    SIZE stSize;
    UINT cchSize;
    HDC hdc = GetDC(ghViewWnd);
    HFONT hFtOld;

    StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);
    if (0 >= cchSize)
        return 0;

    hFtOld = SelectFont(hdc, ghAaFont);
    GetTextExtentPoint32(hdc, ptStr, cchSize, &stSize);
    SelectFont(hdc, hFtOld);
    ReleaseDC(ghViewWnd, hdc);

    return stSize.cx;
}
