#include "Sunday.h"
#include "InsertUi.h"
#include "DocViewBridgeInternal.h"
#include "UiText.h"

#define FRAMEINSERTBOX_CLASS TEXT("FRAMEINSBOX_CLASS")
#define FIB_WIDTH 600
#define FIB_HEIGHT 200

#define TRANCE_COLOUR RGB(0x66, 0x77, 0x88)
#define IDCB_FRMINSBOX_FRAMESEL 1264

#define FIB_SEP_SMALL 8
#define FIB_FRAMESEL_WIDTH 220
#define FIB_INDEX_FRAMESEL_SPACE 2

struct FRAME_EDIT_PART_BINDING
{
    INT dControlId;
    FRAMEITEM FRAMEINFO::*pfItem;
};

struct FRAME_HORIZONTAL_BAND_SPEC
{
    LPFRAMEITEM pstLeft;
    LPFRAMEITEM pstCenter;
    LPFRAMEITEM pstRight;
    INT iBandLines;
    INT iRepeatCount;
    INT iRestDot;
    INT iLeftOffset;
    UINT bMultiPadd;
    BOOLEAN bAlignBottom;
    BOOLEAN bPadMissingRight;
};

struct FRAME_VERTICAL_BAND_SPEC
{
    LPFRAMEITEM pstLeft;
    LPFRAMEITEM pstRight;
    INT iBandLines;
    INT iRightDot;
    INT iNegativeLeftOffset;
};

static const FRAME_EDIT_PART_BINDING gstFrameEditPartBindings[] = {
    {IDE_BOXP_MORNING, &FRAMEINFO::stMorning},
    {IDE_BOXP_NOON, &FRAMEINFO::stNoon},
    {IDE_BOXP_AFTERNOON, &FRAMEINFO::stAfternoon},
    {IDE_BOXP_DAYBREAK, &FRAMEINFO::stDaybreak},
    {IDE_BOXP_NIGHTFALL, &FRAMEINFO::stNightfall},
    {IDE_BOXP_TWILIGHT, &FRAMEINFO::stTwilight},
    {IDE_BOXP_MIDNIGHT, &FRAMEINFO::stMidnight},
    {IDE_BOXP_DAWN, &FRAMEINFO::stDawn}};

static const UINT gdFibToolbarStringIndices[] = {0, 4, 6};

#define TB_ITEMS 7
static TBBUTTON gstFIBTBInfo[] = {
    {0, IDM_FRAME_INS_DECIDE, TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {FIB_SEP_SMALL, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {FIB_FRAMESEL_WIDTH, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {FIB_SEP_SMALL, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {1, IDM_FRMINSBOX_QCLOSE, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {FIB_SEP_SMALL, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {2, IDM_FRMINSBOX_PADDING, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0}};

//-------------------------------------------------------------------------------------------------

extern FILES_ITR gitFileIt; // 今見てるファイルの本体
extern INT gixFocusPage;    // 注目中のページ・とりあえず０・０インデックス

extern HWND ghViewWnd; // ビューウインドウハンドル

extern INT gdHideXdot;    // 左の隠れ部分
extern INT gdViewTopLine; // 表示中の最上部行番号
extern INT gdDocXdot;     // キャレットのＸドット・ドキュメント位置
extern INT gdDocLine;     // キャレットのＹ行数・ドキュメント位置

extern HFONT ghAaFont; // AA用フォント

struct FRAME_STORAGE_STATE
{
    TCHAR atJsonPath[MAX_PATH]{};
    UINT dFrameCount{};
    vector<JSON_FRAME_RECORD> vcFrameRecords;
};

struct FRAME_EDIT_STATE
{
    INT dNowSel{};
    RECT stOrigRect{};
    LPTSTR ptSample{};
    RECT stSamplePos{};
    BOOLEAN bFrameListChanged{};
    vector<FRAMEINFO> vcFrameInfo;
};

struct FRAME_INSERT_BOX_STATE
{
    HWND hWnd{};
    HWND hToolbarWnd{};
    HWND hComboWnd{};
    WNDPROC pfOrigToolbarProc{};
    HBRUSH hBgBrush{};
    HMENU hMainMenu{};
    HIMAGELIST hImageList{};
    INSERT_UI_GEOMETRY stGeometry{};
    INT dToolBarHeight{};
    INT dFrameSelHeight{};
    UINT dSelected{};
    BOOLEAN bQuickClose{TRUE};
    FRAMEINFO stNowFrameInfo{};
    LPTSTR ptFrameBox{};
    UINT bMultiPaddingTemp{};
};

static FRAME_STORAGE_STATE gstFrameStorage;
static FRAME_EDIT_STATE gstFrameEdit;
static FRAME_INSERT_BOX_STATE gstFrameInsert;

extern HFONT ghAaFont; //    AA用フォント
//-------------------------------------------------------------------------------------------------

INT_PTR CALLBACK FrameEditDlgProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR Frm_OnInitDialog(HWND, HWND, LPARAM);
INT_PTR Frm_OnCommand(HWND, INT, HWND, UINT);
INT_PTR Frm_OnDrawItem(HWND, CONST LPDRAWITEMSTRUCT);
INT_PTR Frm_OnNotify(HWND, INT, LPNMHDR);

HRESULT FrameRecordAccess(UINT, UINT, LPFRAMEINFO);

HRESULT FramePartsUpdate(HWND, HWND, LPFRAMEITEM);

HRESULT FrameDataGet(UINT, LPFRAMEINFO);
HRESULT FrameInfoDisp(HWND);
UINT FrameCountGet(VOID);

VOID FrameDataTranslate(LPTSTR, UINT);

UINT FrameMultiSubstring(LPCTSTR, CONST UINT, LPTSTR, CONST UINT_PTR, CONST INT);
UINT FrameMakeMultiSubLine(CONST BOOLEAN, LPFRAMEITEM, LPTSTR, CONST UINT_PTR);
UINT StringWidthAdjust(CONST UINT, LPTSTR, CONST UINT_PTR, CONST INT);

INT FrameMultiSizeGet(LPFRAMEINFO, PINT, PINT);
LPTSTR FrameMakeOutsideBoundary(CONST INT, CONST INT, LPFRAMEINFO);
LPTSTR FrameMakeInsideBoundary(UINT, PINT, LPFRAMEINFO);

INT FrameInsBoxSizeGet(LPRECT);
VOID FrameInsBoxFrmDraw(HDC);
VOID FrameDrawItem(HDC, INT, LPTSTR);
INT_PTR Frm_OnSize(HWND, UINT, INT, INT);
INT_PTR Frm_OnWindowPosChanging(HWND, LPWINDOWPOS);

HRESULT FrameInsBoxDoInsert(HWND);

LRESULT CALLBACK FrameInsProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK gpfFrameTBProc(HWND, UINT, WPARAM, LPARAM);
VOID Fib_OnPaint(HWND);
VOID Fib_OnCommand(HWND, INT, HWND, UINT);
VOID Fib_OnDestroy(HWND);
VOID Fib_OnMoving(HWND, LPRECT);
VOID Fib_OnKey(HWND, UINT, BOOL, INT, UINT);
BOOL Fib_OnWindowPosChanging(HWND, LPWINDOWPOS);
VOID Fib_OnWindowPosChanged(HWND, const LPWINDOWPOS);
static UINT FrameCountRefresh(VOID);
static HRESULT FrameSelectionApply(HWND, UINT);
static HRESULT FrameRecordsLoad(VOID);
static HRESULT FrameRecordsWrite(VOID);
static HRESULT FrameRecordsEnsureDefault(VOID);
static VOID FrameTextCopyToBuffer(LPTSTR, UINT_PTR, wstring_view);
static VOID FrameRecordToFrameInfo(const JSON_FRAME_RECORD &, LPFRAMEINFO);
static JSON_FRAME_RECORD FrameInfoToFrameRecord(const FRAMEINFO &);
static JSON_FRAME_RECORD FrameCreateDefaultRecord(VOID);
static VOID FrameDataRefreshMetrics(LPFRAMEINFO);
static LPFRAMEINFO FrameEditCurrentInfo(VOID);
static LPFRAMEITEM FrameEditItemFromControlId(LPFRAMEINFO, INT);
static BOOLEAN FrameEditTryHandlePartUpdate(HWND, INT, HWND, UINT, LPFRAMEINFO);
static VOID FrameEditSampleLayoutInit(HWND);
static VOID FrameEditSampleFree(VOID);
static VOID FrameEditSampleRefresh(HWND);
static HRESULT FrameEditAppendNewRecord(HWND);
static VOID FrameEditFramesReload(VOID);
static VOID FrameInsBoxPreviewFree(VOID);
static VOID FrameInsBoxPreviewRefresh(HWND);
static VOID FramePartsResetLoopState(LPFRAMEITEM *, UINT);
static BOOLEAN FrameBandPartEnabled(INT, INT, LPFRAMEITEM, BOOLEAN);
static VOID FrameAppendHorizontalBandLine(wstring &, INT, const FRAME_HORIZONTAL_BAND_SPEC &);
static VOID FrameBuildHorizontalBand(vector<wstring> &, INT, const FRAME_HORIZONTAL_BAND_SPEC &);
static VOID FrameBuildVerticalBand(vector<wstring> &, INT, const FRAME_VERTICAL_BAND_SPEC &);
static LPTSTR FrameJoinLines(const vector<wstring> &, BOOLEAN);
static HWND FrameInsBoxCreateWindow(HINSTANCE);
static HWND FrameInsBoxCreateToolbar(HINSTANCE);
static HWND FrameInsBoxCreateCombo(HINSTANCE);
static VOID FrameInsBoxLayoutCombo(VOID);
static VOID FrameInsBoxPopulateCombo(VOID);
static VOID FrameInsBoxSyncFrames(VOID);
static VOID FrameInsBoxUpdateControls(VOID);
//-------------------------------------------------------------------------------------------------

static VOID FrameTextCopyToBuffer(LPTSTR ptDest, UINT_PTR cchDest, wstring_view wsText)
{
    size_t cchCopy;

    if (!(ptDest) || 0 >= cchDest)
        return;

    cchCopy = wsText.size();
    if (cchDest <= cchCopy)
        cchCopy = cchDest - 1;

    CopyMemory(ptDest, wsText.data(), cchCopy * sizeof(TCHAR));
    ptDest[cchCopy] = 0;
}

static VOID FrameRecordToFrameInfo(const JSON_FRAME_RECORD &stRecord, LPFRAMEINFO pstInfo)
{
    if (!(pstInfo))
        return;

    *pstInfo = FRAMEINFO{};
    FrameTextCopyToBuffer(pstInfo->atFrameName, MAX_STRING, stRecord.wsName);
    FrameTextCopyToBuffer(pstInfo->stDaybreak.atParts, PARTS_CCH, stRecord.wsDaybreak);
    FrameTextCopyToBuffer(pstInfo->stMorning.atParts, PARTS_CCH, stRecord.wsMorning);
    FrameTextCopyToBuffer(pstInfo->stNoon.atParts, PARTS_CCH, stRecord.wsNoon);
    FrameTextCopyToBuffer(pstInfo->stAfternoon.atParts, PARTS_CCH, stRecord.wsAfternoon);
    FrameTextCopyToBuffer(pstInfo->stNightfall.atParts, PARTS_CCH, stRecord.wsNightfall);
    FrameTextCopyToBuffer(pstInfo->stTwilight.atParts, PARTS_CCH, stRecord.wsTwilight);
    FrameTextCopyToBuffer(pstInfo->stMidnight.atParts, PARTS_CCH, stRecord.wsMidnight);
    FrameTextCopyToBuffer(pstInfo->stDawn.atParts, PARTS_CCH, stRecord.wsDawn);
    pstInfo->dLeftOffset = stRecord.dLeftOffset;
    pstInfo->dRightOffset = stRecord.dRightOffset;
    pstInfo->dRestPadd = stRecord.bRestPadding;
}

static JSON_FRAME_RECORD FrameInfoToFrameRecord(const FRAMEINFO &stInfo)
{
    JSON_FRAME_RECORD stRecord;

    stRecord.wsName = stInfo.atFrameName;
    stRecord.wsDaybreak = stInfo.stDaybreak.atParts;
    stRecord.wsMorning = stInfo.stMorning.atParts;
    stRecord.wsNoon = stInfo.stNoon.atParts;
    stRecord.wsAfternoon = stInfo.stAfternoon.atParts;
    stRecord.wsNightfall = stInfo.stNightfall.atParts;
    stRecord.wsTwilight = stInfo.stTwilight.atParts;
    stRecord.wsMidnight = stInfo.stMidnight.atParts;
    stRecord.wsDawn = stInfo.stDawn.atParts;
    stRecord.dLeftOffset = stInfo.dLeftOffset;
    stRecord.dRightOffset = stInfo.dRightOffset;
    stRecord.bRestPadding = stInfo.dRestPadd;

    return stRecord;
}

static JSON_FRAME_RECORD FrameCreateDefaultRecord(VOID)
{
    JSON_FRAME_RECORD stRecord;

    stRecord.wsName = TEXT("새 말풍선");
    stRecord.wsDaybreak = TEXT("|　");
    stRecord.wsMorning = TEXT("／");
    stRecord.wsNoon = TEXT("￣");
    stRecord.wsAfternoon = TEXT("＼");
    stRecord.wsNightfall = TEXT("|");
    stRecord.wsTwilight = TEXT("／");
    stRecord.wsMidnight = TEXT("＿");
    stRecord.wsDawn = TEXT("＼");
    stRecord.dLeftOffset = 0;
    stRecord.dRightOffset = 13;
    stRecord.bRestPadding = FALSE;

    return stRecord;
}

static HRESULT FrameRecordsLoad(VOID)
{
    gstFrameStorage.vcFrameRecords.clear();

    return AppLoadFrameRecords(gstFrameStorage.atJsonPath, &gstFrameStorage.vcFrameRecords);
}

static HRESULT FrameRecordsWrite(VOID)
{
    return AppWriteFrameRecords(gstFrameStorage.atJsonPath, gstFrameStorage.vcFrameRecords);
}

static HRESULT FrameRecordsEnsureDefault(VOID)
{
    HRESULT hRslt;

    hRslt = FrameRecordsLoad();
    if (SUCCEEDED(hRslt) && !(gstFrameStorage.vcFrameRecords.empty()))
        return S_OK;

    gstFrameStorage.vcFrameRecords.clear();
    gstFrameStorage.vcFrameRecords.push_back(FrameCreateDefaultRecord());
    FrameCountRefresh();

    hRslt = FrameRecordsWrite();
    if (FAILED(hRslt))
    {
        gstFrameStorage.vcFrameRecords.clear();
        FrameCountRefresh();
        return hRslt;
    }

    return S_OK;
}

static VOID FrameDataRefreshMetrics(LPFRAMEINFO pstFrame)
{
    if (!(pstFrame))
        return;

    pstFrame->stDaybreak.iLine = DocStringInfoCount(pstFrame->stDaybreak.atParts, 0, &(pstFrame->stDaybreak.dDot), nullptr);
    pstFrame->stMorning.iLine = DocStringInfoCount(pstFrame->stMorning.atParts, 0, &(pstFrame->stMorning.dDot), nullptr);
    pstFrame->stNoon.iLine = DocStringInfoCount(pstFrame->stNoon.atParts, 0, &(pstFrame->stNoon.dDot), nullptr);
    pstFrame->stAfternoon.iLine = DocStringInfoCount(pstFrame->stAfternoon.atParts, 0, &(pstFrame->stAfternoon.dDot), nullptr);
    pstFrame->stNightfall.iLine = DocStringInfoCount(pstFrame->stNightfall.atParts, 0, &(pstFrame->stNightfall.dDot), nullptr);
    pstFrame->stTwilight.iLine = DocStringInfoCount(pstFrame->stTwilight.atParts, 0, &(pstFrame->stTwilight.dDot), nullptr);
    pstFrame->stMidnight.iLine = DocStringInfoCount(pstFrame->stMidnight.atParts, 0, &(pstFrame->stMidnight.dDot), nullptr);
    pstFrame->stDawn.iLine = DocStringInfoCount(pstFrame->stDawn.atParts, 0, &(pstFrame->stDawn.dDot), nullptr);
}

static LPFRAMEINFO FrameEditCurrentInfo(VOID)
{
    if (gstFrameEdit.vcFrameInfo.empty())
        return nullptr;

    if (0 > gstFrameEdit.dNowSel)
        return nullptr;

    if (gstFrameEdit.vcFrameInfo.size() <= (size_t)gstFrameEdit.dNowSel)
        return nullptr;

    return &(gstFrameEdit.vcFrameInfo[gstFrameEdit.dNowSel]);
}

static LPFRAMEITEM FrameEditItemFromControlId(LPFRAMEINFO pstInfo, INT dControlId)
{
    UINT i;

    if (!(pstInfo))
        return nullptr;

    for (i = 0; std::size(gstFrameEditPartBindings) > i; i++)
    {
        if (gstFrameEditPartBindings[i].dControlId == dControlId)
            return &(pstInfo->*(gstFrameEditPartBindings[i].pfItem));
    }

    return nullptr;
}

static BOOLEAN FrameEditTryHandlePartUpdate(HWND hDlg, INT id, HWND hWndCtl, UINT codeNotify, LPFRAMEINFO pstInfo)
{
    LPFRAMEITEM pstItem;

    if (EN_UPDATE != codeNotify)
        return FALSE;

    pstItem = FrameEditItemFromControlId(pstInfo, id);
    if (!(pstItem))
        return FALSE;

    FramePartsUpdate(hDlg, hWndCtl, pstItem);
    return TRUE;
}

static VOID FrameEditSampleLayoutInit(HWND hDlg)
{
    RECT rect;
    UINT ofs;
    POINT point;

    if (!(hDlg))
        return;

    SetWindowFont(GetDlgItem(hDlg, IDS_FRAME_IMAGE), ghAaFont, FALSE);

    GetWindowRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), &gstFrameEdit.stSamplePos);
    gstFrameEdit.stSamplePos.right -= gstFrameEdit.stSamplePos.left;
    gstFrameEdit.stSamplePos.bottom -= gstFrameEdit.stSamplePos.top;

    GetClientRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), &rect);
    ofs = gstFrameEdit.stSamplePos.right - rect.right;
    gstFrameEdit.stSamplePos.right += ofs;
    ofs = gstFrameEdit.stSamplePos.bottom - rect.bottom;
    gstFrameEdit.stSamplePos.bottom += ofs;

    point.x = gstFrameEdit.stSamplePos.left;
    point.y = gstFrameEdit.stSamplePos.top;
    ScreenToClient(hDlg, &point);
    gstFrameEdit.stSamplePos.left = point.x;
    gstFrameEdit.stSamplePos.top = point.y;

    GetClientRect(hDlg, &rect);
    gstFrameEdit.stSamplePos.right = rect.right - gstFrameEdit.stSamplePos.right;
    gstFrameEdit.stSamplePos.bottom = rect.bottom - gstFrameEdit.stSamplePos.bottom;
}

static VOID FrameEditSampleFree(VOID)
{
    FREE(gstFrameEdit.ptSample);
}

static VOID FrameEditSampleRefresh(HWND hDlg)
{
    LPFRAMEINFO pstInfo;
    RECT rect;

    pstInfo = FrameEditCurrentInfo();
    if (!(hDlg) || !(pstInfo))
        return;

    GetClientRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), &rect);
    FrameEditSampleFree();
    gstFrameEdit.ptSample = FrameMakeOutsideBoundary(rect.right, rect.bottom, pstInfo);
}

static HRESULT FrameEditAppendNewRecord(HWND hDlg)
{
    const JSON_FRAME_RECORD stRecord = FrameCreateDefaultRecord();
    FRAMEINFO stInfo;
    HWND hCmboxWnd;
    const UINT dNewIndex = (UINT)gstFrameStorage.vcFrameRecords.size();
    HRESULT hRslt;

    gstFrameStorage.vcFrameRecords.push_back(stRecord);
    FrameCountRefresh();

    hRslt = FrameRecordsWrite();
    if (FAILED(hRslt))
    {
        gstFrameStorage.vcFrameRecords.pop_back();
        FrameCountRefresh();
        return hRslt;
    }

    FrameRecordToFrameInfo(stRecord, &stInfo);
    FrameDataRefreshMetrics(&stInfo);
    gstFrameEdit.vcFrameInfo.push_back(stInfo);
    gstFrameEdit.bFrameListChanged = TRUE;

    hCmboxWnd = GetDlgItem(hDlg, IDCB_BOX_NAME_SEL);
    EnableWindow(hCmboxWnd, TRUE);
    EnableWindow(GetDlgItem(hDlg, IDB_APPLY), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDB_OK), TRUE);
    ComboBox_AddString(hCmboxWnd, stInfo.atFrameName);

    gstFrameEdit.dNowSel = dNewIndex;
    ComboBox_SetCurSel(hCmboxWnd, gstFrameEdit.dNowSel);
    FrameInfoDisp(hDlg);
    FrameEditSampleRefresh(hDlg);
    InvalidateRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), nullptr, TRUE);

    SetFocus(GetDlgItem(hDlg, IDE_BOXP_NAME_EDIT));
    Edit_SetSel(GetDlgItem(hDlg, IDE_BOXP_NAME_EDIT), 0, -1);

    return S_OK;
}

static VOID FrameEditFramesReload(VOID)
{
    UINT i;

    gstFrameEdit.vcFrameInfo.clear();
    gstFrameEdit.vcFrameInfo.resize(gstFrameStorage.dFrameCount);

    for (i = 0; gstFrameStorage.dFrameCount > i; i++)
    {
        FrameDataGet(i, &(gstFrameEdit.vcFrameInfo[i]));
    }
}

static VOID FrameInsBoxPreviewFree(VOID)
{
    FREE(gstFrameInsert.ptFrameBox);
}

static VOID FrameInsBoxPreviewRefresh(HWND hWnd)
{
    RECT stFrmRct;

    if (0 == gstFrameStorage.dFrameCount)
        return;

    FrameInsBoxSizeGet(&stFrmRct);
    FrameInsBoxPreviewFree();
    gstFrameInsert.ptFrameBox = FrameMakeOutsideBoundary(stFrmRct.right, stFrmRct.bottom, &gstFrameInsert.stNowFrameInfo);

    if (hWnd)
    {
        InvalidateRect(hWnd, nullptr, TRUE);
    }
}

static HWND FrameInsBoxCreateWindow(HINSTANCE hInst)
{
    gstFrameInsert.hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                                         FRAMEINSERTBOX_CLASS, TEXT("말풍선 박스"),
                                         WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
                                         0, 0, FIB_WIDTH, FIB_HEIGHT, nullptr, nullptr, hInst, nullptr);
    SetLayeredWindowAttributes(gstFrameInsert.hWnd, TRANCE_COLOUR, 0xFF, LWA_COLORKEY);

    return gstFrameInsert.hWnd;
}

static HWND FrameInsBoxCreateToolbar(HINSTANCE hInst)
{
    LPCTSTR aptToolbarTexts[] = {
        TEXT("삽입하기"),
        TEXT("삽입하고 창 닫기"),
        TEXT("박스 크기에 말풍선 맞추기\r\n(빈 공간이 생길 수 있습니다.)")};

    gstFrameInsert.hToolbarWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TOOLBARCLASSNAME, TEXT("fibtoolbar"),
                                                WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
                                                0, 0, 0, 0, gstFrameInsert.hWnd, (HMENU)IDTB_FRMINSBOX_TOOLBAR, hInst, nullptr);
    AppUiFontApply(gstFrameInsert.hToolbarWnd, FALSE);
    gstFrameInsert.dToolBarHeight = InsertUiToolbarInitialise(gstFrameInsert.hToolbarWnd, gstFrameInsert.hImageList,
                                                              gstFIBTBInfo, TB_ITEMS,
                                                              gdFibToolbarStringIndices, aptToolbarTexts, (UINT)std::size(aptToolbarTexts), TRUE);
    gstFrameInsert.pfOrigToolbarProc = SubclassWindow(gstFrameInsert.hToolbarWnd, gpfFrameTBProc);

    return gstFrameInsert.hToolbarWnd;
}

static HWND FrameInsBoxCreateCombo(HINSTANCE hInst)
{
    gstFrameInsert.hComboWnd = CreateWindowEx(0, WC_COMBOBOX, TEXT("framecombo"),
                                              WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                              0, 0, FIB_WIDTH, 220, gstFrameInsert.hToolbarWnd, (HMENU)IDCB_FRMINSBOX_FRAMESEL, hInst, nullptr);
    SetWindowFont(gstFrameInsert.hComboWnd, AppUiDropDownFontGet(), TRUE);

    return gstFrameInsert.hComboWnd;
}

static VOID FrameInsBoxLayoutCombo(VOID)
{
    RECT rect;

    SendMessage(gstFrameInsert.hToolbarWnd, TB_GETITEMRECT, FIB_INDEX_FRAMESEL_SPACE, (LPARAM)&rect);
    MoveWindow(gstFrameInsert.hComboWnd, rect.left + 1, 2, (rect.right - rect.left) - 2, 200, TRUE);
    SetWindowPos(gstFrameInsert.hComboWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

static VOID FrameInsBoxPopulateCombo(VOID)
{
    UINT d;
    TCHAR atBuffer[MAX_STRING];

    for (d = 0; gstFrameStorage.dFrameCount > d; d++)
    {
        FrameNameLoad(d, atBuffer, MAX_STRING);
        ComboBox_AddString(gstFrameInsert.hComboWnd, atBuffer);
    }
}

static VOID FrameInsBoxSyncFrames(VOID)
{
    UINT dSelection;

    if (!(gstFrameInsert.hWnd) || !(gstFrameInsert.hToolbarWnd) || !(gstFrameInsert.hComboWnd))
        return;

    ComboBox_ResetContent(gstFrameInsert.hComboWnd);
    FrameInsBoxPopulateCombo();

    if (0 == gstFrameStorage.dFrameCount)
    {
        FrameInsBoxUpdateControls();
        return;
    }

    EnableWindow(gstFrameInsert.hComboWnd, TRUE);
    SendMessage(gstFrameInsert.hToolbarWnd, TB_ENABLEBUTTON, IDM_FRAME_INS_DECIDE, TRUE);
    SendMessage(gstFrameInsert.hToolbarWnd, TB_ENABLEBUTTON, IDM_FRMINSBOX_PADDING, TRUE);

    dSelection = gstFrameInsert.dSelected;
    if (gstFrameStorage.dFrameCount <= dSelection)
        dSelection = gstFrameStorage.dFrameCount - 1;

    ComboBox_SetCurSel(gstFrameInsert.hComboWnd, dSelection);
    FrameSelectionApply(gstFrameInsert.hWnd, dSelection);
}

static VOID FrameInsBoxUpdateControls(VOID)
{
    SendMessage(gstFrameInsert.hToolbarWnd, TB_CHECKBUTTON, IDM_FRMINSBOX_QCLOSE, gstFrameInsert.bQuickClose);

    if (0 == gstFrameStorage.dFrameCount)
    {
        EnableWindow(gstFrameInsert.hComboWnd, FALSE);
        SendMessage(gstFrameInsert.hToolbarWnd, TB_ENABLEBUTTON, IDM_FRAME_INS_DECIDE, FALSE);
        SendMessage(gstFrameInsert.hToolbarWnd, TB_ENABLEBUTTON, IDM_FRMINSBOX_PADDING, FALSE);
        return;
    }

    ComboBox_SetCurSel(gstFrameInsert.hComboWnd, 0);
    FrameSelectionApply(gstFrameInsert.hWnd, 0);
}

static VOID FramePartsResetLoopState(LPFRAMEITEM *pptItems, UINT dItemCount)
{
    UINT i;

    for (i = 0; dItemCount > i; i++)
    {
        if (pptItems[i])
            pptItems[i]->iNowLn = 0;
    }
}

static BOOLEAN FrameBandPartEnabled(INT iTargetLine, INT iBandLines, LPFRAMEITEM pstItem, BOOLEAN bAlignBottom)
{
    if (!(pstItem))
        return FALSE;

    if (bAlignBottom)
        return (0 >= (iBandLines - iTargetLine) - pstItem->iLine) ? TRUE : FALSE;

    return (iTargetLine < pstItem->iLine) ? TRUE : FALSE;
}

static VOID FrameAppendHorizontalBandLine(wstring &wsLine, INT iTargetLine, const FRAME_HORIZONTAL_BAND_SPEC &stSpec)
{
    TCHAR atSubStr[MAX_PATH];
    BOOLEAN bEnable;
    INT i;

    bEnable = FrameBandPartEnabled(iTargetLine, stSpec.iBandLines, stSpec.pstLeft, stSpec.bAlignBottom);
    FrameMakeMultiSubLine(bEnable, stSpec.pstLeft, atSubStr, MAX_PATH);
    if (1 <= stSpec.iLeftOffset)
    {
        StringWidthAdjust(stSpec.iLeftOffset, atSubStr, MAX_PATH, 0);
    }
    wsLine.append(atSubStr);

    bEnable = FrameBandPartEnabled(iTargetLine, stSpec.iBandLines, stSpec.pstCenter, stSpec.bAlignBottom);
    FrameMakeMultiSubLine(bEnable, stSpec.pstCenter, atSubStr, MAX_PATH);
    for (i = 0; stSpec.iRepeatCount > i; i++)
    {
        wsLine.append(atSubStr);
    }
    if ((1 <= stSpec.iRestDot) && stSpec.bMultiPadd)
    {
        StringWidthAdjust(0, atSubStr, MAX_PATH, stSpec.iRestDot);
        wsLine.append(atSubStr);
    }

    bEnable = FrameBandPartEnabled(iTargetLine, stSpec.iBandLines, stSpec.pstRight, stSpec.bAlignBottom);
    if (stSpec.bPadMissingRight)
    {
        FrameMakeMultiSubLine(bEnable, stSpec.pstRight, atSubStr, MAX_PATH);
        wsLine.append(atSubStr);
    }
    else if (bEnable)
    {
        FrameMultiSubstring(stSpec.pstRight->atParts, stSpec.pstRight->iNowLn, atSubStr, MAX_PATH, 0);
        stSpec.pstRight->iNowLn++;
        wsLine.append(atSubStr);
    }
}

static VOID FrameBuildHorizontalBand(vector<wstring> &vcLines, INT iLineOffset, const FRAME_HORIZONTAL_BAND_SPEC &stSpec)
{
    INT i;

    for (i = 0; stSpec.iBandLines > i; i++)
    {
        FrameAppendHorizontalBandLine(vcLines.at(iLineOffset + i), i, stSpec);
    }
}

static VOID FrameBuildVerticalBand(vector<wstring> &vcLines, INT iLineOffset, const FRAME_VERTICAL_BAND_SPEC &stSpec)
{
    INT i;
    TCHAR atSubStr[MAX_PATH];

    for (i = 0; stSpec.iBandLines > i; i++)
    {
        FrameMultiSubstring(stSpec.pstLeft->atParts, stSpec.pstLeft->iNowLn, atSubStr, MAX_PATH, stSpec.iRightDot);
        if (0 < stSpec.iNegativeLeftOffset)
        {
            StringWidthAdjust(stSpec.iNegativeLeftOffset, atSubStr, MAX_PATH, 0);
        }
        stSpec.pstLeft->iNowLn++;
        if (stSpec.pstLeft->iLine <= stSpec.pstLeft->iNowLn)
        {
            stSpec.pstLeft->iNowLn = 0;
        }
        vcLines.at(iLineOffset + i).append(atSubStr);

        FrameMultiSubstring(stSpec.pstRight->atParts, stSpec.pstRight->iNowLn, atSubStr, MAX_PATH, 0);
        stSpec.pstRight->iNowLn++;
        if (stSpec.pstRight->iLine <= stSpec.pstRight->iNowLn)
        {
            stSpec.pstRight->iNowLn = 0;
        }
        vcLines.at(iLineOffset + i).append(atSubStr);
    }
}

static LPTSTR FrameJoinLines(const vector<wstring> &vcLines, BOOLEAN bTrailingCrLf)
{
    UINT_PTR cchTotal;
    UINT_PTR i;
    UINT_PTR cchSep;
    LPTSTR ptBuffer;

    cchTotal = 1;
    cchSep = CH_CRLF_CCH;
    for (i = 0; vcLines.size() > i; i++)
    {
        cchTotal += vcLines[i].size();
        if (0 < i)
            cchTotal += cchSep;
        if (bTrailingCrLf)
            cchTotal += cchSep;
    }

    ptBuffer = (LPTSTR)malloc(cchTotal * sizeof(TCHAR));
    if (!(ptBuffer))
        return nullptr;

    ZeroMemory(ptBuffer, cchTotal * sizeof(TCHAR));
    for (i = 0; vcLines.size() > i; i++)
    {
        if (0 < i)
            StringCchCat(ptBuffer, cchTotal, TEXT("\r\n"));
        StringCchCat(ptBuffer, cchTotal, vcLines[i].c_str());
        if (bTrailingCrLf)
            StringCchCat(ptBuffer, cchTotal, TEXT("\r\n"));
    }

    return ptBuffer;
}

static UINT FrameCountRefresh(VOID)
{
    gstFrameStorage.dFrameCount = (UINT)gstFrameStorage.vcFrameRecords.size();
    return gstFrameStorage.dFrameCount;
}

UINT FrameCountGet(VOID)
{
    return gstFrameStorage.dFrameCount;
}

static HRESULT FrameSelectionApply(HWND hWnd, UINT dSelection)
{
    if (gstFrameStorage.dFrameCount <= dSelection)
        return E_INVALIDARG;

    gstFrameInsert.dSelected = dSelection;
    FrameDataGet(gstFrameInsert.dSelected, &gstFrameInsert.stNowFrameInfo);

    SendMessage(gstFrameInsert.hToolbarWnd, TB_CHECKBUTTON, IDM_FRMINSBOX_PADDING, gstFrameInsert.stNowFrameInfo.dRestPadd);
    gstFrameInsert.bMultiPaddingTemp = gstFrameInsert.stNowFrameInfo.dRestPadd;
    FrameInsBoxPreviewRefresh(hWnd);

    return S_OK;
}

// 프레임 설정 JSON 경로를 확보하고 초기화한다.
HRESULT FrameInitialise(LPTSTR ptCurrent, HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    HBITMAP hImg, hMsq;

    if (!(ptCurrent) || !(hInstance))
    {
        if (gstFrameInsert.hWnd)
        {
            DestroyWindow(gstFrameInsert.hWnd);
        }
        if (gstFrameInsert.hBgBrush)
        {
            DeleteBrush(gstFrameInsert.hBgBrush);
        }

        if (gstFrameInsert.hImageList)
        {
            ImageList_Destroy(gstFrameInsert.hImageList);
        }

        return S_OK;
    }

    StringCchCopy(gstFrameStorage.atJsonPath, MAX_PATH, ptCurrent);
    PathAppend(gstFrameStorage.atJsonPath, TEMPLATE_DIR);
    PathAppend(gstFrameStorage.atJsonPath, SETTINGS_FRAME_JSON_FILE);
    FrameRecordsEnsureDefault();
    FrameCountRefresh();
    gstFrameEdit.vcFrameInfo.clear();
    gstFrameEdit.vcFrameInfo.resize(gstFrameStorage.dFrameCount);

    // 枠挿入窓
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = FrameInsProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = FRAMEINSERTBOX_CLASS;
    wcex.hIconSm = nullptr;

    RegisterClassEx(&wcex);

    gstFrameInsert = FRAME_INSERT_BOX_STATE{};
    gstFrameInsert.hBgBrush = CreateSolidBrush(TRANCE_COLOUR);

    //    アイコン
    gstFrameInsert.hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 3, 1);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBMP_FRMINS_INSERT));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBMQ_FRMINS_INSERT));
    ImageList_Add(gstFrameInsert.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBMP_REFRESH));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBMQ_REFRESH));
    ImageList_Add(gstFrameInsert.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBMP_FRMINS_PADD));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE(IDBMQ_FRMINS_PADD));
    ImageList_Add(gstFrameInsert.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 枠名称の内容でメニューを変更
HRESULT FrameNameModifyMenu(HWND hWnd)
{
    HMENU hMenu;
    HMENU hInsertMenu;

    hMenu = GetMenu(hWnd);
    if (!(hMenu))
        return E_FAIL;

    hInsertMenu = GetSubMenu(hMenu, 2);
    if (!(hInsertMenu))
        return E_FAIL;

    if (gstFrameInsert.hMainMenu)
    {
        DestroyMenu(gstFrameInsert.hMainMenu);
        gstFrameInsert.hMainMenu = nullptr;
    }

    if (FAILED(MenuPickerCreatePagedMenu(&gstFrameInsert.hMainMenu, MENU_PICKER_FRAME,
                                         MENU_PICKER_MENU_GROUP_MAX)))
        return E_FAIL;

    ModifyMenu(hInsertMenu, 4,
               MF_BYPOSITION | MF_POPUP,
               (UINT_PTR)gstFrameInsert.hMainMenu,
               UiTextGetMenuText(IDM_MN_INSFRAME_SEL));

    DrawMenuBar(hWnd);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ポッパップメニュー用に名前をずっこんばっこん
HRESULT FrameNameModifyPopUp(HMENU hPopMenu, UINT bMode)
{
    UINT i, j, k;
    TCHAR atBuffer[MAX_PATH], atName[MAX_STRING];
    const UINT dMenuFrameLimit = (IDM_INSFRAME_TANGO - IDM_INSFRAME_ALPHA + 1);

    for (i = 0, j = 1; gstFrameStorage.dFrameCount > i && dMenuFrameLimit > i; i++, j++)
    {
        FrameNameLoad(i, atName, MAX_STRING);

        if (bMode)
        {
            if (9 >= j)
            {
                k = j + '0';
            }
            else if (10 == j)
            {
                k = '0';
            }
            else
            {
                k = 'A' + j - 11;
            }
            StringCchPrintf(atBuffer, MAX_PATH, TEXT("%s(&%c)"), atName, k);
        }
        else
        {
            StringCchPrintf(atBuffer, MAX_PATH, TEXT("%s"), atName);
        }
        ModifyMenu(hPopMenu, IDM_INSFRAME_ALPHA + i, MF_BYCOMMAND | MFT_STRING, IDM_INSFRAME_ALPHA + i, atBuffer);
        //    メニューリソース番号の連番に注意
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 枠の名前を引っ張ってくる
HRESULT FrameNameLoad(UINT dNumber, LPTSTR ptNamed, UINT_PTR cchSize)
{
    if (!(ptNamed) || 0 >= cchSize)
        return E_INVALIDARG;

    if (gstFrameStorage.dFrameCount <= dNumber)
        return E_OUTOFMEMORY;

    FrameTextCopyToBuffer(ptNamed, cchSize, gstFrameStorage.vcFrameRecords[dNumber].wsName);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// JSON 레코드와 편집용 FRAMEINFO 사이를 변환한다.
HRESULT FrameRecordAccess(UINT dMode, UINT dNumber, LPFRAMEINFO pstInfo)
{
    if (dMode) //    ロード
    {
        if (!(pstInfo) || gstFrameStorage.vcFrameRecords.size() <= dNumber)
            return E_INVALIDARG;
        FrameRecordToFrameInfo(gstFrameStorage.vcFrameRecords[dNumber], pstInfo);
    }
    else //    セーブ
    {
        if (!(pstInfo))
            return E_INVALIDARG;
        if (gstFrameStorage.vcFrameRecords.size() <= dNumber)
            gstFrameStorage.vcFrameRecords.resize(dNumber + 1);
        gstFrameStorage.vcFrameRecords[dNumber] = FrameInfoToFrameRecord(*pstInfo);
        FrameCountRefresh();
        return FrameRecordsWrite();
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 割り算する。０除算なら０を返す
INT Divinus(INT iLeft, INT iRight)
{
    INT iAnswer;

    if (0 == iRight)
        return 0;

    iAnswer = iLeft / iRight;

    return iAnswer;
}
//-------------------------------------------------------------------------------------------------

// 枠設定のダイヤログを開く
INT_PTR FrameEditDialogue(HINSTANCE hInst, HWND hWnd, UINT dRsv)
{
    INT_PTR iRslt;

    gstFrameEdit.dNowSel = 0;
    gstFrameEdit.bFrameListChanged = FALSE;

    iRslt = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_FRAME_EDIT_DLG_2), hWnd, FrameEditDlgProc, 0);

    //    処理結果によっては、ここでメニューの内容書換
    if (IDYES == iRslt || gstFrameEdit.bFrameListChanged)
    {
        FrameNameModifyMenu(hWnd);
        FrameInsBoxSyncFrames();
    }

    return iRslt;
}
//-------------------------------------------------------------------------------------------------

// 枠設定ダイヤログプロシージャ
INT_PTR CALLBACK FrameEditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    default:
        break;

    case WM_INITDIALOG:
        return Frm_OnInitDialog(hDlg, (HWND)(wParam), lParam);
    case WM_COMMAND:
        return Frm_OnCommand(hDlg, (INT)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam));
    case WM_DRAWITEM:
        return Frm_OnDrawItem(hDlg, ((CONST LPDRAWITEMSTRUCT)(lParam)));
    case WM_NOTIFY:
        return Frm_OnNotify(hDlg, (INT)(wParam), (LPNMHDR)(lParam));
    case WM_SIZE:
        return Frm_OnSize(hDlg, (UINT)(wParam), (INT)(SHORT)LOWORD(lParam), (INT)(SHORT)HIWORD(lParam));

    case WM_WINDOWPOSCHANGING:
        return Frm_OnWindowPosChanging(hDlg, (LPWINDOWPOS)(lParam));
    }
    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

// ダイヤログ起動時の初期化
INT_PTR Frm_OnInitDialog(HWND hDlg, HWND hWndFocus, LPARAM lParam)
{
    HWND hWorkWnd;
    UINT i;

    GetWindowRect(hDlg, &gstFrameEdit.stOrigRect);
    gstFrameEdit.stOrigRect.bottom -= gstFrameEdit.stOrigRect.top;
    gstFrameEdit.stOrigRect.right -= gstFrameEdit.stOrigRect.left;
    gstFrameEdit.stOrigRect.top = 0;
    gstFrameEdit.stOrigRect.left = 0;

    //    コンボックスに名前いれとく
    hWorkWnd = GetDlgItem(hDlg, IDCB_BOX_NAME_SEL);

    FrameCountRefresh();
    FrameEditFramesReload();
    FrameEditSampleLayoutInit(hDlg);

    for (i = 0; gstFrameStorage.dFrameCount > i; i++)
    {
        //    INIファイルから引っ張って、コンボックスに名前をいれちゃう
        ComboBox_AddString(hWorkWnd, gstFrameEdit.vcFrameInfo[i].atFrameName);
    }

    if (0 == gstFrameStorage.dFrameCount)
    {
        EnableWindow(GetDlgItem(hDlg, IDB_APPLY), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDB_OK), FALSE);
        EnableWindow(hWorkWnd, FALSE);
        return (INT_PTR)TRUE;
    }

    ComboBox_SetCurSel(hWorkWnd, gstFrameEdit.dNowSel);

    //    パーツ情報をいれる
    FrameInfoDisp(hDlg);

    //    初期状態で確保
    FrameEditSampleRefresh(hDlg);

    return (INT_PTR)TRUE;
}
//-------------------------------------------------------------------------------------------------

// ダイヤログのCOMMANDメッセージの受け取り
INT_PTR Frm_OnCommand(HWND hDlg, INT id, HWND hWndCtl, UINT codeNotify)
{
    static BOOLEAN cbNameMod = FALSE; //    ダイヤログ終わり用の恒久的なもの
    static BOOLEAN cbNameChg = FALSE; //    APPLY用
    UINT i;
    INT iRslt;
    HWND hCmboxWnd;
    LPFRAMEINFO pstInfo;

    pstInfo = FrameEditCurrentInfo();

    switch (id)
    {
    default:
        break;

    case IDCANCEL:
    case IDB_CANCEL:
        FrameEditSampleFree();
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;

    case IDB_APPLY:
    case IDB_OK:
        hCmboxWnd = GetDlgItem(hDlg, IDCB_BOX_NAME_SEL);
        for (i = 0; gstFrameStorage.dFrameCount > i; i++)
        {
            FrameRecordAccess(INIT_SAVE, i, &(gstFrameEdit.vcFrameInfo[i]));
            if (cbNameChg)
            {
                ComboBox_DeleteString(hCmboxWnd, 0);                        // 先頭消して
                ComboBox_AddString(hCmboxWnd, gstFrameEdit.vcFrameInfo[i].atFrameName); // 末尾に付け足す
            }
        }
        ComboBox_SetCurSel(hCmboxWnd, gstFrameEdit.dNowSel);
        cbNameChg = FALSE;
        if (IDB_OK == id)
        {
            FrameEditSampleFree();
            EndDialog(hDlg, cbNameMod ? IDYES : IDOK);
        }
        return (INT_PTR)TRUE;

    case IDE_BOXP_MORNING:
    case IDE_BOXP_NOON:
    case IDE_BOXP_AFTERNOON:
    case IDE_BOXP_DAYBREAK:
    case IDE_BOXP_NIGHTFALL:
    case IDE_BOXP_TWILIGHT:
    case IDE_BOXP_MIDNIGHT:
    case IDE_BOXP_DAWN:
        if (FrameEditTryHandlePartUpdate(hDlg, id, hWndCtl, codeNotify, pstInfo))
        {
            return (INT_PTR)TRUE;
        }
        return (INT_PTR)TRUE;

    case IDS_FRAME_IMAGE:
        if (STN_DBLCLK == codeNotify) //    ダボークルックされた
        {
            InvalidateRect(hWndCtl, nullptr, TRUE);
        }
        return (INT_PTR)TRUE;

    case IDB_FRM_PADDING:
        if (!(pstInfo))
            return (INT_PTR)TRUE;
        iRslt = Button_GetCheck(hWndCtl);
        pstInfo->dRestPadd = (BST_CHECKED == iRslt) ? TRUE : FALSE;

        FrameEditSampleRefresh(hDlg);
        InvalidateRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), nullptr, TRUE);
        return (INT_PTR)TRUE;

    case IDB_ADD_FRAME:
        FrameEditAppendNewRecord(hDlg);
        return (INT_PTR)TRUE;

    case IDB_BOXP_NAME_APPLY: //    名称を変更した
        if (!(pstInfo))
            return (INT_PTR)TRUE;
        Edit_GetText(GetDlgItem(hDlg, IDE_BOXP_NAME_EDIT), pstInfo->atFrameName, MAX_STRING);
        cbNameMod = TRUE;
        cbNameChg = TRUE;
        return (INT_PTR)TRUE;

    case IDCB_BOX_NAME_SEL:
        if (CBN_SELCHANGE == codeNotify) //    選択が変更された
        {
            gstFrameEdit.dNowSel = ComboBox_GetCurSel(hWndCtl);
            FrameInfoDisp(hDlg);

            FrameEditSampleRefresh(hDlg);

            InvalidateRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), nullptr, TRUE);
        }
        return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

// 各パーツが更新されたら
HRESULT FramePartsUpdate(HWND hDlg, HWND hWndCtl, LPFRAMEITEM pstItem)
{
    TCHAR atBuffer[PARTS_CCH];

    if (Edit_GetTextLength(hWndCtl))
    {
        Edit_GetText(hWndCtl, atBuffer, PARTS_CCH);
        atBuffer[PARTS_CCH - 1] = 0;
        StringCchCopy(pstItem->atParts, PARTS_CCH, atBuffer);
    }
    else //    文字がなかったら、全角空白にしちゃう
    {
        StringCchCopy(pstItem->atParts, PARTS_CCH, TEXT("　"));
    }

    pstItem->iLine = DocStringInfoCount(pstItem->atParts, 0, &(pstItem->dDot), nullptr);

    //    ついでに再描画
    FrameEditSampleRefresh(hDlg);
    InvalidateRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), nullptr, TRUE);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ダイヤログのサイズ変更が完了する前に送られてくる
INT_PTR Frm_OnWindowPosChanging(HWND hDlg, LPWINDOWPOS pstWpos)
{
    //    移動がなかったときは何もしないでおｋ
    if (SWP_NOSIZE & pstWpos->flags)
        return FALSE;

    if (gstFrameEdit.stOrigRect.right > pstWpos->cx)
        pstWpos->cx = gstFrameEdit.stOrigRect.right;
    if (gstFrameEdit.stOrigRect.bottom > pstWpos->cy)
        pstWpos->cy = gstFrameEdit.stOrigRect.bottom;

    return (INT_PTR)TRUE;
}
//-------------------------------------------------------------------------------------------------

// ダイヤログがサイズ変更されたとき
INT_PTR Frm_OnSize(HWND hDlg, UINT state, INT cx, INT cy)
{
    HWND hSmpWnd;
    INT xx, yy;

    hSmpWnd = GetDlgItem(hDlg, IDS_FRAME_IMAGE);

    //    下半分に常に全開
    xx = cx - gstFrameEdit.stSamplePos.right;
    yy = cy - gstFrameEdit.stSamplePos.bottom;

    SetWindowPos(hSmpWnd, HWND_TOP, 0, 0, xx, yy, SWP_NOMOVE | SWP_NOZORDER);

    FrameEditSampleRefresh(hDlg);

    InvalidateRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), nullptr, TRUE);

    return (INT_PTR)TRUE;
}
//-------------------------------------------------------------------------------------------------

// オーナードローの処理・スタティックのアレ
INT_PTR Frm_OnDrawItem(HWND hDlg, CONST LPDRAWITEMSTRUCT pstDrawItem)
{
    if (IDS_FRAME_IMAGE != pstDrawItem->CtlID)
    {
        return (INT_PTR)FALSE;
    }

    FillRect(pstDrawItem->hDC, &(pstDrawItem->rcItem), GetSysColorBrush(COLOR_WINDOW));
    SetBkMode(pstDrawItem->hDC, TRANSPARENT);
    // ここから複数行処理すればいいか

    DrawText(pstDrawItem->hDC, gstFrameEdit.ptSample, -1, &(pstDrawItem->rcItem), DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_WORDBREAK);

    return (INT_PTR)TRUE;
}
//-------------------------------------------------------------------------------------------------

/*
描画幅は決まっている。枠指定なら外から、範囲指定なら、文字列＋左右の幅がＭＡＸ幅

左上パーツと右上パーツの幅を確認して、残りが上パーツ使う。使う個数は幅から算出
占有行数が異なるなら、下側を合わせる。右側は途中で切る事も考慮


床部分も処理は同じ。占有行数異なるなら、上側を合わせる。

柱は、必要行数確認する。複数行になるなら、途中で切る。
左柱は０基点、右柱は、右上右下パーツの左に合わせる＋オフセット

*/

// 渡されたパーツから、必要なところを抜き出して文字列作る
UINT FrameMakeMultiSubLine(CONST BOOLEAN bEnable, LPFRAMEITEM pstItem, LPTSTR ptDest, CONST UINT_PTR cchSz)
{
    LPTSTR ptBufStr;

    if (bEnable) //    有効であるか
    {
        //    マルチ行の一部をブッコ抜く
        FrameMultiSubstring(pstItem->atParts, pstItem->iNowLn, ptDest, cchSz, pstItem->dDot);
        // 最大幅に満たない行は、Paddingする
        pstItem->iNowLn++;
    }
    else //    空白である
    {
        ptBufStr = DocPaddingSpaceWithPeriod(pstItem->dDot, nullptr, nullptr, nullptr, TRUE); // DocPaddingSpaceMake( pstItem->dDot );
        StringCchCopy(ptDest, cchSz, ptBufStr);
        FREE(ptBufStr);
    }

    return 1;
}
//-------------------------------------------------------------------------------------------------

// 枠パーツの、天井と床の占有行数と、左柱のドット数を確保する
INT FrameMultiSizeGet(LPFRAMEINFO pstInfo, PINT piUpLine, PINT piDnLine)
{
    INT iUpLine, iDnLine;

    //    行数の確認
    iUpLine = pstInfo->stMorning.iLine; //    左上
    if (iUpLine < pstInfo->stNoon.iLine)
        iUpLine = pstInfo->stNoon.iLine; //    上
    if (iUpLine < pstInfo->stAfternoon.iLine)
        iUpLine = pstInfo->stAfternoon.iLine; //    右上

    iDnLine = pstInfo->stDawn.iLine; //    左下
    if (iDnLine < pstInfo->stMidnight.iLine)
        iDnLine = pstInfo->stMidnight.iLine; //    下
    if (iDnLine < pstInfo->stTwilight.iLine)
        iDnLine = pstInfo->stTwilight.iLine; //    右下

    if (piUpLine)
        *piUpLine = iUpLine;
    if (piDnLine)
        *piDnLine = iDnLine;

    return pstInfo->stDaybreak.dDot;
}
//-------------------------------------------------------------------------------------------------

// 文字列を受けて、前パディングして、幅ぴったりになるようにパディングしたり切ったりする
UINT StringWidthAdjust(CONST UINT iFwOffs, LPTSTR ptStr, CONST UINT_PTR cchSz, CONST INT iMaxDot)
{
    INT iDot, iPadd;
    INT iDotCnt, iBuf;
    UINT_PTR dm, dMozi;
    TCHAR atWork[MAX_PATH];
    LPTSTR ptBufStr;

    ZeroMemory(atWork, sizeof(atWork));

    if (1 <= iFwOffs)
    {
        ptBufStr = DocPaddingSpaceWithPeriod(iFwOffs, nullptr, nullptr, nullptr, TRUE); // DocPaddingSpaceMake( iFwOffs );
        if (ptBufStr)
        {
            StringCchCopy(atWork, MAX_PATH, ptBufStr);
            FREE(ptBufStr);
        }
    }
    StringCchCat(atWork, MAX_PATH, ptStr);

    iDot = ViewStringWidthGet(atWork);

    if ((0 != iMaxDot) && (iDot != iMaxDot)) //    ０ではなく、丁度でも無い場合
    {
        if (iDot < iMaxDot) //    指定幅のほうが広いならパディングー
        {
            iPadd = iMaxDot - iDot;
            ptBufStr = DocPaddingSpaceWithPeriod(iPadd, nullptr, nullptr, nullptr, TRUE); // DocPaddingSpaceMake( iPadd );
            if (ptBufStr)
            {
                StringCchCat(atWork, MAX_PATH, ptBufStr);
                FREE(ptBufStr);
            }
        }
        else //    そうでないならぶった切る
        {
            StringCchLength(atWork, MAX_PATH, &dMozi);
            iDotCnt = 0; //    長さ確認していく
            for (dm = 0; dMozi > dm; dm++)
            {
                iBuf = ViewLetterWidthGet(atWork[dm]);
                if (iMaxDot < (iDotCnt + iBuf))
                {
                    atWork[dm] = 0; //    一旦文字列閉じる
                    iBuf = iMaxDot - iDotCnt;
                    if (0 < iBuf)
                    {
                        ptBufStr = DocPaddingSpaceWithPeriod(iBuf, nullptr, nullptr, nullptr, TRUE); // DocPaddingSpaceMake( iBuf );
                        if (ptBufStr)
                        {
                            StringCchCat(atWork, MAX_PATH, ptBufStr);
                            FREE(ptBufStr);
                        }
                    }
                    break;
                }
                iDotCnt += iBuf;
            }
        }

        iDot = ViewStringWidthGet(atWork);
    }

    StringCchCopy(ptStr, cchSz, atWork);

    return iDot;
}
//-------------------------------------------------------------------------------------------------

// 外枠に合わせて、複数行枠を作る
LPTSTR FrameMakeOutsideBoundary(CONST INT iWidth, CONST INT iHeight, LPFRAMEINFO pstInfo)
{
    LPTSTR ptBufStr;
    INT iLines;                    //    全体行数
    INT iUpLine, iMdLine, iDnLine; //    天井、柱、床の占有行
    INT iRitOccup;
    INT iRight;
    INT iRoofDot, iFloorDot;
    INT iRoofCnt, iFloorCnt; //    上と下のパーツの個数
    INT iRfRstDot, iFlRstDot;
    INT iRitOff;
    INT iRitBuf;
    UINT bMultiPadd;
    vector<wstring> vcString; //    作業用
    LPFRAMEITEM apstResetItems[] = {
        &(pstInfo->stDaybreak),
        &(pstInfo->stMorning),
        &(pstInfo->stNoon),
        &(pstInfo->stAfternoon),
        &(pstInfo->stNightfall),
        &(pstInfo->stTwilight),
        &(pstInfo->stMidnight),
        &(pstInfo->stDawn)};
    FRAME_HORIZONTAL_BAND_SPEC stRoofSpec{};
    FRAME_VERTICAL_BAND_SPEC stPillarSpec{};
    FRAME_HORIZONTAL_BAND_SPEC stFloorSpec{};

    try
    {

        iLines = iHeight / LINE_HEIGHT; //    切り捨てでおｋ

        bMultiPadd = pstInfo->dRestPadd; //    パディングするかどうか

        //    行数の確認
        FrameMultiSizeGet(pstInfo, &iUpLine, &iDnLine);

        iMdLine = iLines - (iUpLine + iDnLine); //    柱
        //    もし iMdLine がマイナスになったら？　中が作られなくなるだけ
        if (0 > iMdLine)
        {
            iLines -= iMdLine;
            iMdLine = 0;
        } //    符号に注意セヨ
        //    はみ出すとバグるので、はみ出した分は無かったことにする

        vcString.resize(iLines);

        //    右パーツの占有幅、一番長いのを確認
        iRitOccup = pstInfo->stAfternoon.dDot; //    右上
        if (iRitOccup < pstInfo->stTwilight.dDot)
        {
            iRitOccup = pstInfo->stTwilight.dDot;
        }                                                            //    右下
        iRitBuf = pstInfo->dRightOffset + pstInfo->stNightfall.dDot; //    右とオフセット
        if (iRitOccup < iRitBuf)
        {
            iRitOccup = iRitBuf;
        } //    右
        //    iRitOccupは、右パーツの最大占有幅である
        iRitOff = iWidth - iRitOccup; //    右パーツ開始位置を確定

        //    天井に使えるドット数をゲット
        iRoofDot = iRitOff - pstInfo->stMorning.dDot; //    天井に使えるドット幅
        if (1 <= pstInfo->dLeftOffset)
        {
            iRoofDot -= pstInfo->dLeftOffset;
        }
        iRoofCnt = Divinus(iRoofDot, pstInfo->stNoon.dDot); //    占有ドットを確認して、個数出す。切り捨てでおｋ
        iRfRstDot = iRoofDot - (iRoofCnt * pstInfo->stNoon.dDot);

        // 零除算発生

        //    左下と右下と床
        iFloorDot = iRitOff - pstInfo->stDawn.dDot; //    床に使えるドット幅
        if (1 <= pstInfo->dLeftOffset)
        {
            iFloorDot -= pstInfo->dLeftOffset;
        }
        iFloorCnt = Divinus(iFloorDot, pstInfo->stMidnight.dDot); //    占有ドットを確認して、個数出す。
        iFlRstDot = iFloorDot - (iFloorCnt * pstInfo->stMidnight.dDot);

        //    右柱開始位置・パディングを考慮セヨ
        if (bMultiPadd)
        {
            iRight = iRitOff + pstInfo->dRightOffset;
            //    負のオフセットなら、内側にめり込むので、その分端までの長さを縮める
            if (-1 >= pstInfo->dLeftOffset)
            {
                iRight += pstInfo->dLeftOffset;
            }
            //    負数の扱いに注意
        }
        else
        {
            iRight = (iRoofCnt * pstInfo->stNoon.dDot) + pstInfo->stMorning.dDot + pstInfo->dRightOffset;
            iRitBuf = (iFloorCnt * pstInfo->stMidnight.dDot) + pstInfo->stDawn.dDot + pstInfo->dRightOffset;
            if (iRight < iRitBuf)
            {
                iRight = iRitBuf;
            }; //    長い方に合わせる

            iRight += pstInfo->dLeftOffset;
        }

        //    枠合わせ拡大モードのときはオフセットは考慮しない

        //    作業に向けてクルヤー
        FramePartsResetLoopState(apstResetItems, (UINT)std::size(apstResetItems));

        stRoofSpec = {&(pstInfo->stMorning), &(pstInfo->stNoon), &(pstInfo->stAfternoon),
                      iUpLine, iRoofCnt, iRfRstDot, pstInfo->dLeftOffset, bMultiPadd, TRUE, TRUE};
        stPillarSpec = {&(pstInfo->stDaybreak), &(pstInfo->stNightfall), iMdLine, iRight,
                        (-1 >= pstInfo->dLeftOffset) ? -(pstInfo->dLeftOffset) : 0};
        stFloorSpec = {&(pstInfo->stDawn), &(pstInfo->stMidnight), &(pstInfo->stTwilight),
                       iDnLine, iFloorCnt, iFlRstDot, pstInfo->dLeftOffset, bMultiPadd, FALSE, FALSE};

        FrameBuildHorizontalBand(vcString, 0, stRoofSpec);
        FrameBuildVerticalBand(vcString, iUpLine, stPillarSpec);
        FrameBuildHorizontalBand(vcString, iUpLine + iMdLine, stFloorSpec);

        ptBufStr = FrameJoinLines(vcString, FALSE);

    }
    catch (exception &err)
    {
        return (LPTSTR)ETC_MSG(err.what(), 0);
    }
    catch (...)
    {
        return (LPTSTR)ETC_MSG(("etc error"), 0);
    }

    return ptBufStr;
}
//-------------------------------------------------------------------------------------------------

// 内枠に合わせて、複数行枠を作る
LPTSTR FrameMakeInsideBoundary(UINT dType, PINT piValue, LPFRAMEINFO pstInfo)
{
    static INT iRoofCnt, iFloorCnt; //    上と下のパーツの個数
    static INT iRfRstDot, iFlRstDot;

    LPTSTR ptBufStr;

    UINT bMultiPadd;
    INT iUpLine, iDnLine;
    INT iRitOff;
    INT iRitBuf, iRitVle;
    INT iRoofDot, iFloorDot;
    vector<wstring> vcString; //    作業用
    LPFRAMEITEM apstRoofItems[] = {&(pstInfo->stMorning), &(pstInfo->stNoon), &(pstInfo->stAfternoon)};
    LPFRAMEITEM apstFloorItems[] = {&(pstInfo->stTwilight), &(pstInfo->stMidnight), &(pstInfo->stDawn)};
    FRAME_HORIZONTAL_BAND_SPEC stBandSpec{};

    try
    {

        bMultiPadd = pstInfo->dRestPadd; //    パディングするかどうか

        if (0 == dType) //    初期化してstaticで持っておくか
        {
            //    piValue[in]内部のドット数　[out]右柱開始位置

            //    右柱開始位置を確定
            iRitOff = pstInfo->stDaybreak.dDot + *piValue;
            //    左オフセットマイナスなら、左柱が内側にめり込む・マイナス計算に注意
            if (-1 >= pstInfo->dLeftOffset)
            {
                iRitOff += -(pstInfo->dLeftOffset);
            }
            //    右オフセットマイナスなら、右柱が内側にめり込む
            if (-1 >= pstInfo->dRightOffset)
            {
                iRitOff += -(pstInfo->dRightOffset);
            }
            //    すなわち、右開始位置がより右に移動する

            //    天井に使えるドット数をゲット
            iRoofDot = iRitOff - pstInfo->stMorning.dDot; //    天井に使えるドット幅
            if (1 <= pstInfo->dLeftOffset)
            {
                iRoofDot -= pstInfo->dLeftOffset;
            }
            iRoofCnt = Divinus(iRoofDot, pstInfo->stNoon.dDot);       //    占有ドットを確認して、個数出す。切り捨てでおｋ
            iRfRstDot = iRoofDot - (iRoofCnt * pstInfo->stNoon.dDot); //    パーツを入れていったら余るドット数

            //    左下と右下と床
            iFloorDot = iRitOff - pstInfo->stDawn.dDot; //    床に使えるドット幅
            if (1 <= pstInfo->dLeftOffset)
            {
                iFloorDot -= pstInfo->dLeftOffset;
            }
            iFloorCnt = Divinus(iFloorDot, pstInfo->stMidnight.dDot);       //    占有ドットを確認して、個数出す。
            iFlRstDot = iFloorDot - (iFloorCnt * pstInfo->stMidnight.dDot); //    パーツを入れていったら余るドット数

            if (!(bMultiPadd))
            {
                //    パディングしないのなら、余り分はフルに使い、右柱開始位置は、より長い方に合わせる
                if (0 != iRfRstDot)
                {
                    iRoofCnt++;
                }
                if (0 != iFlRstDot)
                {
                    iFloorCnt++;
                }

                //    はみ出し分確保
                iRitVle = pstInfo->stNoon.dDot - iFlRstDot;
                iRitBuf = pstInfo->stMidnight.dDot - iFlRstDot;
                if (iRitVle < iRitBuf)
                {
                    iRitVle = iRitBuf;
                }

                iRitOff += iRitVle;
            }

            //    右オフセットブラスなら、右柱開始位置はより右に移動
            if (1 <= pstInfo->dRightOffset)
            {
                iRitOff += pstInfo->dRightOffset;
            }

            *piValue = iRitOff; //    右開始位置を戻す

            return nullptr;
        }
        else if (1 == dType) //    天井全体
        {
            FrameMultiSizeGet(pstInfo, &iUpLine, nullptr); //    天井の行数

            FramePartsResetLoopState(apstRoofItems, (UINT)std::size(apstRoofItems));
            vcString.resize(iUpLine);
            stBandSpec = {&(pstInfo->stMorning), &(pstInfo->stNoon), &(pstInfo->stAfternoon),
                          iUpLine, iRoofCnt, iRfRstDot, pstInfo->dLeftOffset, bMultiPadd, TRUE, TRUE};
            FrameBuildHorizontalBand(vcString, 0, stBandSpec);
        }
        else if (2 == dType) //    床全体
        {
            FrameMultiSizeGet(pstInfo, nullptr, &iDnLine); //    床の行数

            FramePartsResetLoopState(apstFloorItems, (UINT)std::size(apstFloorItems));
            vcString.resize(iDnLine);
            stBandSpec = {&(pstInfo->stDawn), &(pstInfo->stMidnight), &(pstInfo->stTwilight),
                          iDnLine, iFloorCnt, iFlRstDot, pstInfo->dLeftOffset, bMultiPadd, FALSE, FALSE};
            FrameBuildHorizontalBand(vcString, 0, stBandSpec);
        }
        else
        {
            return nullptr;
        }

        ptBufStr = FrameJoinLines(vcString, TRUE);

    }
    catch (exception &err)
    {
        return (LPTSTR)ETC_MSG(err.what(), 0);
    }
    catch (...)
    {
        return (LPTSTR)ETC_MSG(("etc error"), 0);
    }

    return ptBufStr;
}
//-------------------------------------------------------------------------------------------------

// ノーティファイメッセージの処理
INT_PTR Frm_OnNotify(HWND hDlg, INT idFrom, LPNMHDR pstNmhdr)
{
    INT nmCode;
    TCHAR atBuff[MIN_STRING];
    LPNMUPDOWN pstUpDown;
    LPFRAMEINFO pstInfo;

    nmCode = pstNmhdr->code;
    pstInfo = FrameEditCurrentInfo();

    //    左押したらデルタが正、右で負

    if (IDSP_LEFT_OFFSET == idFrom)
    {
        pstUpDown = (LPNMUPDOWN)pstNmhdr;
        if (UDN_DELTAPOS == nmCode)
        {
            if (!(pstInfo))
                return (INT_PTR)TRUE;
            if (0 < pstUpDown->iDelta)
            {
                pstInfo->dLeftOffset += 1;
            }
            else if (0 > pstUpDown->iDelta)
            {
                pstInfo->dLeftOffset -= 1;
            }

            StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dLeftOffset);
            Edit_SetText(GetDlgItem(hDlg, IDE_LEFT_OFFSET), atBuff);

            FrameEditSampleRefresh(hDlg);
            InvalidateRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), nullptr, TRUE);
        }
        return (INT_PTR)TRUE;
    }

    if (IDSP_RIGHT_OFFSET == idFrom)
    {
        pstUpDown = (LPNMUPDOWN)pstNmhdr;
        if (UDN_DELTAPOS == nmCode)
        {
            if (!(pstInfo))
                return (INT_PTR)TRUE;
            if (0 < pstUpDown->iDelta)
            {
                pstInfo->dRightOffset -= 1;
            }
            else if (0 > pstUpDown->iDelta)
            {
                pstInfo->dRightOffset += 1;
            }

            StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dRightOffset);
            Edit_SetText(GetDlgItem(hDlg, IDE_RIGHT_OFFSET), atBuff);

            FrameEditSampleRefresh(hDlg);
            InvalidateRect(GetDlgItem(hDlg, IDS_FRAME_IMAGE), nullptr, TRUE);
        }
        return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

// 지정한 프레임 JSON 레코드를 편집용 구조체로 로드하고 도트 정보를 계산한다.
HRESULT FrameDataGet(UINT dNumber, LPFRAMEINFO pstFrame)
{
    FrameRecordAccess(INIT_LOAD, dNumber, pstFrame);
    FrameDataRefreshMetrics(pstFrame);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// エディットボックスに設定内容を入れる
HRESULT FrameInfoDisp(HWND hDlg)
{
    TCHAR atBuff[MIN_STRING];
    LPFRAMEINFO pstInfo;
    UINT i;
    LPFRAMEITEM pstItem;

    pstInfo = FrameEditCurrentInfo();
    if (!(pstInfo))
        return E_FAIL;

    //    名前表示
    Edit_SetText(GetDlgItem(hDlg, IDE_BOXP_NAME_EDIT), pstInfo->atFrameName);

    //    パーツ情報をいれる
    for (i = 0; std::size(gstFrameEditPartBindings) > i; i++)
    {
        pstItem = &(pstInfo->*(gstFrameEditPartBindings[i].pfItem));
        Edit_SetText(GetDlgItem(hDlg, gstFrameEditPartBindings[i].dControlId), pstItem->atParts);
    }

    StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dLeftOffset);
    Edit_SetText(GetDlgItem(hDlg, IDE_LEFT_OFFSET), atBuff);

    StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dRightOffset);
    Edit_SetText(GetDlgItem(hDlg, IDE_RIGHT_OFFSET), atBuff);

    //    天井と床の余り部分埋めるかどうか
    Button_SetCheck(GetDlgItem(hDlg, IDB_FRM_PADDING), pstInfo->dRestPadd ? BST_CHECKED : BST_UNCHECKED);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 枠を入れる
HRESULT DocFrameInsert(INT dMode, INT dStyle)
{
    UINT_PTR iLines;
    INT iLns, iLast;
    INT iTop, iBtm, iInX, iEndot, iPadding, i, baseDot;
    INT xMidLen;
    LPTSTR ptPadding;
    LPTSTR ptString;

    FRAMEINFO stInfo;

    INT iMidLine, iUpLine, iDnLine;
    LPFRAMEITEM pstItem;
    TCHAR atSubStr[MAX_PATH]; //    足りるか？

    try
    {

        if (0 > dMode)
            return DocInsertSpoTag(dStyle);

        FrameDataGet(dMode, &stInfo);

        iLines = DocNowFilePageLineCount(); //    ページ全体の行数

        //    開始地点から開始    //    D_SQUARE
        iTop = gitFileIt->vcCont.at(gixFocusPage).dSelLineTop;
        iBtm = gitFileIt->vcCont.at(gixFocusPage).dSelLineBottom;
        if (0 > iTop)
        {
            iTop = 0;
        }
        if (0 > iBtm)
        {
            iBtm = iLines - 1;
        }

        //    末端を確認・内容がないなら、使用行戻す
        iInX = DocLineParamGet(iBtm, nullptr, nullptr);
        if (0 == iInX)
        {
            iBtm--;
        }

        //    矩形選択無しとみなす

        DocViewClearSelection();

        //    選択範囲内でもっとも長いドット数を確認
        baseDot = DocPageMaxDotGet(iTop, iBtm);

        // 天井の行数、左の幅、床の行数を確保

        iMidLine = (iBtm - iTop) + 1; //    間の行数確保

        xMidLen = baseDot;
        FrameMakeInsideBoundary(0, &xMidLen, &stInfo);
        //    初期状態を確定する

        //    天井パーツ作成
        ptString = FrameMakeInsideBoundary(1, &xMidLen, &stInfo);
        FrameMultiSizeGet(&stInfo, &iUpLine, nullptr); //    天井の行数
        iLns = iTop;                                   //    注目行
        iInX = 0;                                      //    天井追加
        DocInsertString(&iInX, &iLns, nullptr, ptString, 0, TRUE);
        FREE(ptString);

        //    左と右つくる
        stInfo.stDaybreak.iNowLn = 0;
        stInfo.stNightfall.iNowLn = 0;
        for (i = 0; iMidLine > i; i++, iLns++)
        {
            //    左
            pstItem = &(stInfo.stDaybreak);
            //    埋めるのはパーツ最大位置まで
            FrameMultiSubstring(pstItem->atParts, pstItem->iNowLn, atSubStr, MAX_PATH, pstItem->dDot);
            //    負のパディングをセット
            if (-1 >= stInfo.dLeftOffset)
            {
                StringWidthAdjust(-(stInfo.dLeftOffset), atSubStr, MAX_PATH, 0);
            }
            pstItem->iNowLn++; //    壱行ずつループさせていく
            if (pstItem->iLine <= pstItem->iNowLn)
            {
                pstItem->iNowLn = 0;
            }
            iInX = 0; //    左端からいれる
            DocInsertString(&iInX, &iLns, nullptr, atSubStr, 0, FALSE);

            //    右
            iEndot = DocLineParamGet(iLns, nullptr, nullptr);                                 //    この行の末端
            iPadding = xMidLen - iEndot;                                                      //    埋め量確認
            ptPadding = DocPaddingSpaceWithPeriod(iPadding, nullptr, nullptr, nullptr, TRUE); // DocPaddingSpaceMake( iPadding );
            if (ptPadding)
            {
                DocInsertString(&iEndot, &iLns, nullptr, ptPadding, 0, FALSE);
                FREE(ptPadding);
            }
            pstItem = &(stInfo.stNightfall);
            FrameMultiSubstring(pstItem->atParts, pstItem->iNowLn, atSubStr, MAX_PATH, 0);
            pstItem->iNowLn++; //    壱行ずつループさせていく
            if (pstItem->iLine <= pstItem->iNowLn)
            {
                pstItem->iNowLn = 0;
            }
            DocInsertString(&iEndot, &iLns, nullptr, atSubStr, 0, FALSE);
        }

        //    行末がＥＯＦならここでおかしな事になる
        iLast = DocPageParamGet(nullptr, nullptr);
        if (iLast <= iLns)
        {
            iLns = iLast - 1;
            iInX = DocLineParamGet(iLns, nullptr, nullptr);
            DocCrLfAdd(iInX, iLns, FALSE); //    床を入れるために改行
            iLns++;
        }

        //    床作る
        ptString = FrameMakeInsideBoundary(2, &xMidLen, &stInfo);
        FrameMultiSizeGet(&stInfo, nullptr, &iDnLine); //    床の行数
        iInX = 0;                                      //    天井追加
        DocInsertString(&iInX, &iLns, nullptr, ptString, 0, FALSE);
        FREE(ptString);

    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
    try
    {

        //    最終的なキャレットの位置をリセット
        DocViewResetCaret(iInX, iLns);

        DocViewRefreshLine(iTop);
        DocBadSpaceCheck(iTop);

        iLns = DocNowFilePageLineCount();
        for (i = iTop; iLns > i; i++)
        {
            DocBadSpaceCheck(i);
            DocViewRefreshLine(i);
        }

        DocPageInfoRenew(-1, 1);

    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 複数行Frameの、￥￥・￥ｎ・￥ｓ＜＝＞￥・0x0D0A・半角空白の相互変換
VOID FrameDataTranslate(LPTSTR ptData, UINT bMode)
{
    TCHAR atBuffer[SUB_STRING];
    UINT_PTR i, j, cchLen;

    ZeroMemory(atBuffer, sizeof(atBuffer));

    StringCchLength(ptData, PARTS_CCH, &cchLen); //    長さ確認

    for (i = 0, j = 0; cchLen > i; i++, j++)
    {
        if (0x0000 == ptData[i])
            break; //    多分意味はないけど安全対策

        if (bMode) //    ￥ｎを改行にする
        {
            if (0x005C == ptData[i]) //    エスケープシーケンス
            {
                i++; //    次の文字が重要
                if ('n' == ptData[i])
                {
                    atBuffer[j++] = 0x000D;
                    atBuffer[j] = 0x000A;
                }
                else if ('s' == ptData[i])
                {
                    atBuffer[j] = 0x0020;
                }
                else //    ￥￥であった
                {
                    atBuffer[j] = ptData[i];
                }
            }
            else //    関係ないならそのままコピーしていく
            {
                atBuffer[j] = ptData[i];
            }
        }
        else //    改行を￥ｎにする
        {
            if (0x005C == ptData[i]) //    ￥記号
            {
                atBuffer[j++] = 0x005C;
                atBuffer[j] = 0x005C; //    重ねる
            }
            else if (0x000D == ptData[i]) //    改行はいった
            {
                atBuffer[j++] = 0x005C;
                atBuffer[j] = TEXT('n'); //    エスケープシーケンス
                i++;                     //    次に進める
            }
            else if (0x0020 == ptData[i]) //    半角空白はいった
            {
                atBuffer[j++] = 0x005C;
                atBuffer[j] = TEXT('s'); //    エスケープシーケンス
            }
            else //    関係ないならそのままコピーしていく
            {
                atBuffer[j] = ptData[i];
            }
        }
    }

    StringCchCopy(ptData, PARTS_CCH, atBuffer); //    書き戻す

    return;
}
//-------------------------------------------------------------------------------------------------

// 改行を含む文字列を受け取って、指定行の内容をバッファに入れる
UINT FrameMultiSubstring(LPCTSTR ptSrc, CONST UINT dLine, LPTSTR ptDest, CONST UINT_PTR cchSz, CONST INT iUseDot)
{
    LPTSTR ptPadding;
    INT iPaDot, iStrDot;
    UINT_PTR cchSrc, c, d;
    UINT iLnCnt;

    StringCchLength(ptSrc, STRSAFE_MAX_CCH, &cchSrc); //    元文字列の長さ確認

    ZeroMemory(ptDest, cchSz * sizeof(TCHAR)); //    とりあえずウケを浄化

    iLnCnt = 0;
    d = 0;
    for (c = 0; cchSrc > c; c++)
    {
        if (0x000D == ptSrc[c]) //    かいぎょうはっけん
        {
            c++;      //    0x0Aを飛ばす
            iLnCnt++; //    フォーカス行数
        }
        else //    普通の文字
        {
            if (dLine == iLnCnt) //    行が一致したら
            {
                if (cchSz > d)
                {
                    ptDest[d] = ptSrc[c];
                    d++;
                }
            }
        }
    }
    ptDest[(cchSz - 1)] = 0; //    ヌルターミネータ

    iStrDot = ViewStringWidthGet(ptDest); //    ブッコ抜いた文字列のドット長
    //    パディングもしちゃう
    iPaDot = iUseDot - iStrDot;
    if (1 <= iPaDot)
    {
        ptPadding = DocPaddingSpaceWithPeriod(iPaDot, nullptr, nullptr, nullptr, TRUE); // DocPaddingSpaceMake( iPaDot );
        StringCchCat(ptDest, cchSz, ptPadding);
        FREE(ptPadding);
    }

    iLnCnt++; //    ０インデックスなので１増やすのが正解
    return iLnCnt;
}
//-------------------------------------------------------------------------------------------------

// 挿入ウインドウについて

// 挿入用ウインドウ作る
HWND FrameInsBoxCreate(HINSTANCE hInst, HWND hPrWnd)
{
    INT x, y;
    RECT vwRect;

    if (gstFrameInsert.hWnd)
    {
        SetForegroundWindow(gstFrameInsert.hWnd);
        return gstFrameInsert.hWnd;
    }

    gstFrameInsert.dFrameSelHeight = 0;
    FrameInsBoxCreateWindow(hInst);
    FrameInsBoxCreateToolbar(hInst);
    FrameInsBoxCreateCombo(hInst);
    FrameInsBoxLayoutCombo();
    FrameInsBoxPopulateCombo();
    FrameInsBoxUpdateControls();
    RedrawWindow(gstFrameInsert.hComboWnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

    //    ウインドウ位置を確定させる
    GetWindowRect(ghViewWnd, &vwRect);
    x = LINENUM_WID;
    y = RULER_AREA;

    InsertUiPlaceContentAtViewPoint(ghViewWnd, gstFrameInsert.hWnd, &(gstFrameInsert.stGeometry), gstFrameInsert.dToolBarHeight, x, y);

    return gstFrameInsert.hWnd;
}
//-------------------------------------------------------------------------------------------------

LRESULT CALLBACK gpfFrameTBProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (SendMessage(hWnd, TB_GETHOTITEM, 0, 0) >= 0)
        {
            ReleaseCapture();
        }
        return 0;

    default:
        break;
    }

    return CallWindowProc(gstFrameInsert.pfOrigToolbarProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// フレームサイズを確保
INT FrameInsBoxSizeGet(LPRECT pstRect)
{
    RECT rect;
    INT iTopOffset;

    GetClientRect(gstFrameInsert.hWnd, &rect);

    iTopOffset = gstFrameInsert.dToolBarHeight;
    rect.top = 0;
    rect.bottom -= iTopOffset;

    *pstRect = rect;

    return iTopOffset;
}
//-------------------------------------------------------------------------------------------------

// 挿入実行
HRESULT FrameInsBoxDoInsert(HWND hWnd)
{
    INT iX, iY;
    HWND hLyrWnd;
    RECT rect;

    if (nullptr == gstFrameInsert.ptFrameBox)
        return S_FALSE;

    //    挿入処理には、レイヤボックスを非表示処理で使う
    hLyrWnd = LayerBoxVisibalise(GetModuleHandle(nullptr), gstFrameInsert.ptFrameBox, 0x10);

    //    レイヤの位置を変更
    GetWindowRect(hWnd, &rect);
    LayerBoxPositionChange(hLyrWnd, (rect.left + gstFrameInsert.stGeometry.stContentOffset.x), (rect.top + gstFrameInsert.stGeometry.stContentOffset.y));
    //    空白を全部透過指定にする
    LayerTransparentToggle(hLyrWnd, 1);
    //    上書きする
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    LayerContentsImportable(hLyrWnd, IDM_LYB_OVERRIDE, &iX, &iY, D_INVISI);
    DocViewResetCaret(iX, iY);
    DocPageInfoRenew(-1, 1);
    EditChangeSetApply(stChangeSet);
    //    終わったら閉じる
    DestroyWindow(hLyrWnd);

    if (gstFrameInsert.bQuickClose)
        DestroyWindow(hWnd);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ウインドウプロシージャ
LRESULT CALLBACK FrameInsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_PAINT, Fib_OnPaint); //    画面の更新とか
        HANDLE_MSG(hWnd, WM_KEYDOWN, Fib_OnKey);
        HANDLE_MSG(hWnd, WM_COMMAND, Fib_OnCommand);
        HANDLE_MSG(hWnd, WM_DESTROY, Fib_OnDestroy); //    終了時の処理
        HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGING, Fib_OnWindowPosChanging);
        HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGED, Fib_OnWindowPosChanged);

    case WM_MOVING:
        Fib_OnMoving(hWnd, (LPRECT)lParam);
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// COMMANDメッセージの受け取り。ボタン押されたとかで発生
VOID Fib_OnCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    INT iRslt = -1;
    INT iSel;

    switch (id)
    {
    case IDM_FRAME_INS_DECIDE:
        FrameInsBoxDoInsert(hWnd);
        return;

    case IDCB_FRMINSBOX_FRAMESEL:
        if (CBN_SELCHANGE != codeNotify)
            return;
        iSel = ComboBox_GetCurSel(hWndCtl);
        if (0 > iSel)
            return;
        FrameSelectionApply(hWnd, (UINT)iSel);
        SetFocus(hWnd);
        return;

    case IDM_FRMINSBOX_QCLOSE:
        gstFrameInsert.bQuickClose = SendMessage(gstFrameInsert.hToolbarWnd, TB_ISBUTTONCHECKED, IDM_FRMINSBOX_QCLOSE, 0);
        return;

    case IDM_FRMINSBOX_PADDING:
        iRslt = SendMessage(gstFrameInsert.hToolbarWnd, TB_ISBUTTONCHECKED, IDM_FRMINSBOX_PADDING, 0);
        break;

    default:
        return;
    }

    if (0 <= iRslt)
    {
        gstFrameInsert.stNowFrameInfo.dRestPadd = iRslt;
        gstFrameInsert.bMultiPaddingTemp = iRslt;
    }
    else
    {
        gstFrameInsert.bMultiPaddingTemp = gstFrameInsert.stNowFrameInfo.dRestPadd;
        SendMessage(gstFrameInsert.hToolbarWnd, TB_CHECKBUTTON, IDM_FRMINSBOX_PADDING, gstFrameInsert.stNowFrameInfo.dRestPadd);
    }

    FrameInsBoxPreviewRefresh(hWnd);

    return;
}
//-------------------------------------------------------------------------------------------------

// キーダウンが発生・キーボードで移動用
VOID Fib_OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    RECT rect;

    GetWindowRect(hWnd, &rect);

    if (fDown)
    {
        switch (vk)
        {
        case VK_RIGHT:
            rect.left++;
            break;
        case VK_LEFT:
            rect.left--;
            break;
        case VK_DOWN:
            rect.top += LINE_HEIGHT;
            break;
        case VK_UP:
            rect.top -= LINE_HEIGHT;
            break;
        default:
            return;
        }
    }

    SetWindowPos(hWnd, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    Fib_OnMoving(hWnd, &rect);

    return;
}
//-------------------------------------------------------------------------------------------------

// PAINT。無効領域が出来たときに発生。背景の扱いに注意。背景を塗りつぶしてから、オブジェクトを描画
VOID Fib_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;

    RECT rect;

    GetClientRect(hWnd, &rect);

    hdc = BeginPaint(hWnd, &ps);

    FillRect(hdc, &rect, gstFrameInsert.hBgBrush);

    //    文字列再描画
    FrameInsBoxFrmDraw(hdc);

    EndPaint(hWnd, &ps);

    return;
}
//-------------------------------------------------------------------------------------------------

// 描画
VOID FrameInsBoxFrmDraw(HDC hDC)
{
    HFONT hOldFnt;
    INT topOst, iYpos;
    RECT stFrmRct;

    UINT dLines, d;
    TCHAR atBuffer[MAX_PATH];

    SetBkColor(hDC, ViewBackColourGet(nullptr)); //

    hOldFnt = SelectFont(hDC, ghAaFont); //    フォントくっつける

    topOst = FrameInsBoxSizeGet(&stFrmRct); //    FRAME当てはめ枠のサイズ

    if (nullptr == gstFrameInsert.ptFrameBox)
    {
        SelectFont(hDC, hOldFnt);
        return;
    }

    dLines = DocStringInfoCount(gstFrameInsert.ptFrameBox, 0, nullptr, nullptr); //    行数確保
                                                                 //    ptMultiStr

    iYpos = topOst; //
    for (d = 0; dLines > d; d++)
    { //    ptMultiStr
        FrameMultiSubstring(gstFrameInsert.ptFrameBox, d, atBuffer, MAX_PATH, 0);
        FrameDrawItem(hDC, iYpos, atBuffer);
        iYpos += LINE_HEIGHT;
    }

    SelectFont(hDC, hOldFnt); //    フォント戻す

    return;
}

// 壱行分の描画
VOID FrameDrawItem(HDC hDC, INT iY, LPTSTR ptLine)
{
    UINT_PTR cchSize, cl;
    UINT iX, len;
    INT mRslt, mBase;
    LPTSTR ptBgn;
    SIZE stSize;

    StringCchLength(ptLine, STRSAFE_MAX_CCH, &cchSize);

    iX = 0;
    for (cl = 0; cchSize > cl;)
    {
        mRslt = iswspace(ptLine[cl]); //    開始位置の文字タイプ確認
        ptBgn = &(ptLine[cl]);

        for (len = 0; cchSize > cl; len++, cl++)
        {
            mBase = iswspace(ptBgn[len]); //    文字タイプを確認していく
            if (mRslt != mBase)
            {
                break;
            } //    タイプが変わったら
        }
        GetTextExtentPoint32(hDC, ptBgn, len, &stSize); //    ドット数確認

        if (mRslt)
        {
            SetBkMode(hDC, TRANSPARENT);
        }
        else
        {
            SetBkMode(hDC, OPAQUE);
        }

        TextOut(hDC, iX, iY, ptBgn, len);

        iX += stSize.cx;
    }

    return;
}

//-------------------------------------------------------------------------------------------------

// ウインドウを閉じるときに発生。デバイスコンテキストとか確保した画面構造のメモリとかも終了。
VOID Fib_OnDestroy(HWND hWnd)
{
    FrameInsBoxPreviewFree();

    MainStatusBarSetText(SB_LAYER, TEXT(""));

    gstFrameInsert.hWnd = nullptr;

    return;
}
//-------------------------------------------------------------------------------------------------

// 動かされているときに発生・マウスでウインドウドラッグ中とか
VOID Fib_OnMoving(HWND hWnd, LPRECT pstPos)
{
    TCHAR atBuffer[SUB_STRING];

    InsertUiFormatStatusText(pstPos, &(gstFrameInsert.stGeometry), gdHideXdot, gdViewTopLine, TEXT("말풍선"), atBuffer, SUB_STRING);
    MainStatusBarSetText(SB_LAYER, atBuffer);

    return;
}
//-------------------------------------------------------------------------------------------------

// ウィンドウのサイズ変更が完了する前に送られてくる
BOOL Fib_OnWindowPosChanging(HWND hWnd, LPWINDOWPOS pstWpos)
{
    return InsertUiSnapWindowYToViewLine(ghViewWnd, &(gstFrameInsert.stGeometry), pstWpos);
}
//-------------------------------------------------------------------------------------------------

// ウィンドウのサイズ変更が完了したら送られてくる
VOID Fib_OnWindowPosChanged(HWND hWnd, const LPWINDOWPOS pstWpos)
{
    RECT rect;

    SendMessage(gstFrameInsert.hToolbarWnd, TB_AUTOSIZE, 0, 0);
    SendMessage(gstFrameInsert.hToolbarWnd, TB_GETITEMRECT, FIB_INDEX_FRAMESEL_SPACE, (LPARAM)&rect);
    MoveWindow(gstFrameInsert.hComboWnd, rect.left + 1, 2, (rect.right - rect.left) - 2, 200, TRUE);

    GetWindowRect(gstFrameInsert.hToolbarWnd, &rect);
    gstFrameInsert.dToolBarHeight = rect.bottom - rect.top;
    InsertUiCaptureContentOffset(hWnd, gstFrameInsert.dToolBarHeight, &(gstFrameInsert.stGeometry.stContentOffset));

    if (0 < gstFrameStorage.dFrameCount)
    {
        gstFrameInsert.stNowFrameInfo.dRestPadd = gstFrameInsert.bMultiPaddingTemp; //    一時設定
        FrameInsBoxPreviewRefresh(hWnd);
    }

    //    移動がなかったときは何もしないでおｋ
    if (SWP_NOMOVE & pstWpos->flags)
        return;

    InsertUiUpdateOffsetFromWindowPos(ghViewWnd, &(gstFrameInsert.stGeometry), pstWpos);

    return;
}
//-------------------------------------------------------------------------------------------------

// ビューが移動した
HRESULT FrameMoveFromView(HWND hWnd, UINT state)
{
    if (!(gstFrameInsert.hWnd))
        return S_FALSE;

    return InsertUiMoveFromView(ghViewWnd, gstFrameInsert.hWnd, state, &(gstFrameInsert.stGeometry));
}
//-------------------------------------------------------------------------------------------------
