#include "AppModuleInternal.h"
#include "UiText.h"

static HWND ghRebarWnd;

static HWND ghMainTBWnd;
static HWND ghEditTBWnd;
static HWND ghInsertTBWnd;
static HWND ghLayoutTBWnd;
static HWND ghViewTBWnd;

static HIMAGELIST ghMainImgLst;
static HIMAGELIST ghEditImgLst;
static HIMAGELIST ghInsertImgLst;
static HIMAGELIST ghLayoutImgLst;
static HIMAGELIST ghViewImgLst;

static WNDPROC gpfOrigTBProc;
static const UINT gadEditToolbarImageIds[] = {
    IDBMP_UNDO,
    IDBMP_REDO,
    IDBMP_CUT,
    IDBMP_COPY,
    IDBMP_PASTE,
    IDBMP_DELETE,
    IDBMP_PAGESJISCOPY,
    IDBMP_ALLSELECT,
    IDBMP_BOXSELECT,
    IDBMP_LAYERBOX,
    IDBMP_EXTRACT,
    IDBMP_UNICODE_MODE};
static const UINT gadEditToolbarMaskIds[] = {
    IDBMQ_UNDO,
    IDBMQ_REDO,
    IDBMQ_CUT,
    IDBMQ_COPY,
    IDBMQ_PASTE,
    IDBMQ_DELETE,
    IDBMQ_PAGESJISCOPY,
    IDBMQ_ALLSELECT,
    IDBMQ_BOXSELECT,
    IDBMQ_LAYERBOX,
    IDBMQ_EXTRACT,
    IDBMQ_UNICODE_MODE};
static const UINT gadViewToolbarImageIds[] = {
    IDBMP_TRACEMODE,
    IDBMP_PREVIEW};
static const UINT gadViewToolbarMaskIds[] = {
    IDBMQ_TRACEMODE,
    IDBMQ_PREVIEW};

static LRESULT CALLBACK gpfToolbarProc(HWND, UINT, WPARAM, LPARAM);
static VOID ToolBarAssignButtonStrings(HWND, TBBUTTON *, UINT);
static VOID ToolBarRewriteButtonInfo(HWND, TBBUTTON *, UINT, LPACCEL, INT);

#define TB_MAIN_ITEMS 4
static TBBUTTON gstMainTBInfo[] = {
    {0, IDM_NEWFILE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {1, IDM_OPEN, TBSTATE_ENABLED, TBSTYLE_DROPDOWN | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {2, IDM_OVERWRITESAVE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {3, IDM_GENERAL_OPTION, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0} //
};

#define TB_EDIT_ITEMS 14
static TBBUTTON gstEditTBInfo[] = {
    {0, IDM_UNDO, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {1, IDM_REDO, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {7, IDM_ALLSEL, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {3, IDM_COPY, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {6, IDM_SJISCOPY_ALL, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {2, IDM_CUT, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {4, IDM_PASTE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {5, IDM_DELETE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {9, IDM_LAYERBOX, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {8, IDM_SQSELECT, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {10, IDM_EXTRACTION_MODE, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {11, IDM_UNICODE_TOGGLE, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
};

#define TB_INSERT_ITEMS 4
static TBBUTTON gstInsertTBInfo[] = {
    {0, IDM_IN_UNI_SPACE, TBSTATE_ENABLED, TBSTYLE_DROPDOWN | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {1, IDM_INSTAG_COLOUR, TBSTATE_ENABLED, TBSTYLE_DROPDOWN | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {2, IDM_FRMINSBOX_OPEN, TBSTATE_ENABLED, TBSTYLE_DROPDOWN | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {3, IDM_USERINS_NA, TBSTATE_ENABLED, TBSTYLE_DROPDOWN | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0}};

#define TB_LAYOUT_ITEMS 12
static TBBUTTON gstLayoutTBInfo[] = {
    {1, IDM_INS_TOPSPACE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {2, IDM_DEL_TOPSPACE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {8, IDM_INCR_DOT_LINES, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {9, IDM_DECR_DOT_LINES, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {6, IDM_INCREMENT_DOT, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {7, IDM_DECREMENT_DOT, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {10, IDM_MIRROR_INVERSE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {11, IDM_UPSET_INVERSE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {0, IDM_RIGHT_GUIDE_SET, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {3, IDM_DEL_LASTSPACE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {4, IDM_DEL_LASTLETTER, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0} //    行末文字削除
};

#define TB_VIEW_ITEMS 2
static TBBUTTON gstViewTBInfo[] = {
    {0, IDM_TRACE_MODE_ON, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},
    {1, IDM_ON_PREVIEW, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0}};

#define TB_BAND_COUNT 5
static CONST REBARLAYOUTINFO gcstReBarDef[] = {
    {IDTB_MAIN_TOOLBAR, 180, RBBS_GRIPPERALWAYS | RBBS_CHILDEDGE},
    {IDTB_EDIT_TOOLBAR, 450, RBBS_GRIPPERALWAYS | RBBS_CHILDEDGE},
    {IDTB_INSERT_TOOLBAR, 280, RBBS_GRIPPERALWAYS | RBBS_CHILDEDGE | RBBS_BREAK},
    {IDTB_LAYOUT_TOOLBAR, 310, RBBS_GRIPPERALWAYS | RBBS_CHILDEDGE},
    {IDTB_VIEW_TOOLBAR, 140, RBBS_GRIPPERALWAYS | RBBS_CHILDEDGE}
};

LRESULT CALLBACK gpfToolbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
    }

    return CallWindowProc(gpfOrigTBProc, hWnd, msg, wParam, lParam);
}

static VOID ToolBarAssignButtonStrings(HWND hWnd, TBBUTTON *pstButtons, UINT dItemCount)
{
    UINT i;
    TCHAR atBuff[MAX_STRING];
    LPCTSTR ptText;

    for (i = 0; dItemCount > i; i++)
    {
        if (0 == pstButtons[i].idCommand || TBSTYLE_SEP == pstButtons[i].fsStyle)
            continue;

        ptText = UiTextGetLabel(pstButtons[i].idCommand);
        if (nullptr == ptText)
            continue;

        StringCchCopy(atBuff, MAX_STRING, ptText);
        pstButtons[i].iString = SendMessage(hWnd, TB_ADDSTRING, 0, (LPARAM)atBuff);
    }
}

static VOID ToolBarRewriteButtonInfo(HWND hWnd, TBBUTTON *pstButtons, UINT dItemCount, LPACCEL pstAccel, INT iEntry)
{
    UINT i;
    LPCTSTR ptText;
    TCHAR atText[MAX_STRING];
    TBBUTTONINFO stButtonInfo;

    ZeroMemory(&stButtonInfo, sizeof(TBBUTTONINFO));
    stButtonInfo.cbSize = sizeof(TBBUTTONINFO);
    stButtonInfo.dwMask = TBIF_TEXT;
    stButtonInfo.pszText = atText;
    stButtonInfo.cchText = MAX_STRING;

    for (i = 0; dItemCount > i; i++)
    {
        if (0 == pstButtons[i].idCommand)
            continue;

        ptText = UiTextGetLabel(pstButtons[i].idCommand);
        if (nullptr == ptText)
            continue;

        StringCchCopy(atText, MAX_STRING, ptText);
        AccelKeyTextBuild(atText, MAX_STRING, pstButtons[i].idCommand, pstAccel, iEntry);
        SendMessage(hWnd, TB_SETBUTTONINFO, (WPARAM)(pstButtons[i].idCommand), (LPARAM)&stButtonInfo);
    }
}

VOID ToolBarCreate(HWND hWnd, HINSTANCE lcInst)
{
    UINT ici, resnum, d;
    REBARINFO stRbrInfo;
    REBARBANDINFO stRbandInfo;
    REBARLAYOUTINFO stInfo[TB_BAND_COUNT];

    HBITMAP hImg, hMsq;

    ghRebarWnd = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, nullptr,
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE | CCS_NODIVIDER | CCS_TOP,
                                0, 0, 0, 0, hWnd, (HMENU)IDRB_REBAR, lcInst, nullptr);
    AppUiFontApply(ghRebarWnd, FALSE);

    ZeroMemory(&stRbrInfo, sizeof(REBARINFO));
    stRbrInfo.cbSize = sizeof(REBARINFO);
    SendMessage(ghRebarWnd, RB_SETBARINFO, 0, (LPARAM)&stRbrInfo);

    ZeroMemory(stInfo, sizeof(stInfo));
    for (d = 0; TB_BAND_COUNT > d; d++)
    {
        stInfo[d].wID = gcstReBarDef[d].wID;
        stInfo[d].cx = gcstReBarDef[d].cx;
        stInfo[d].fStyle = gcstReBarDef[d].fStyle;
    }
    InitToolBarLayout(INIT_LOAD, TB_BAND_COUNT, stInfo);

    ghMainTBWnd = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("maintb"),
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER, 0, 0, 0, 0, ghRebarWnd, (HMENU)IDTB_MAIN_TOOLBAR, lcInst, nullptr);
    AppUiFontApply(ghMainTBWnd, FALSE);
    SendMessage(ghMainTBWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);

    ghMainImgLst = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 1);
    resnum = IDBMPQ_MAIN_TB_FIRST;
    for (ici = 0; 4 > ici; ici++)
    {
        hImg = LoadBitmap(lcInst, MAKEINTRESOURCE((resnum++)));
        hMsq = LoadBitmap(lcInst, MAKEINTRESOURCE((resnum++)));
        ImageList_Add(ghMainImgLst, hImg, hMsq);
        DeleteBitmap(hImg);
        DeleteBitmap(hMsq);
    }
    SendMessage(ghMainTBWnd, TB_SETIMAGELIST, 0, (LPARAM)ghMainImgLst);
    SendMessage(ghMainTBWnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(16, 16));

    SendMessage(ghMainTBWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    ToolBarAssignButtonStrings(ghMainTBWnd, gstMainTBInfo, TB_MAIN_ITEMS);

    SendMessage(ghMainTBWnd, TB_ADDBUTTONS, (WPARAM)TB_MAIN_ITEMS, (LPARAM)&gstMainTBInfo);

    SendMessage(ghMainTBWnd, TB_AUTOSIZE, 0, 0);

    gpfOrigTBProc = SubclassWindow(ghMainTBWnd, gpfToolbarProc);

    ghEditTBWnd = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("edittb"),
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER, 0, 0, 0, 0, ghRebarWnd, (HMENU)IDTB_EDIT_TOOLBAR, lcInst, nullptr);
    AppUiFontApply(ghEditTBWnd, FALSE);
    SendMessage(ghEditTBWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    ghEditImgLst = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, ARRAYSIZE(gadEditToolbarImageIds), 1);
    for (ici = 0; ARRAYSIZE(gadEditToolbarImageIds) > ici; ici++)
    {
        hImg = LoadBitmap(lcInst, MAKEINTRESOURCE(gadEditToolbarImageIds[ici]));
        hMsq = LoadBitmap(lcInst, MAKEINTRESOURCE(gadEditToolbarMaskIds[ici]));
        ImageList_Add(ghEditImgLst, hImg, hMsq);
        DeleteBitmap(hImg);
        DeleteBitmap(hMsq);
    }
    SendMessage(ghEditTBWnd, TB_SETIMAGELIST, 0, (LPARAM)ghEditImgLst);

    SendMessage(ghEditTBWnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(16, 16));

    SendMessage(ghEditTBWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    ToolBarAssignButtonStrings(ghEditTBWnd, gstEditTBInfo, TB_EDIT_ITEMS);
    SendMessage(ghEditTBWnd, TB_ADDBUTTONS, (WPARAM)TB_EDIT_ITEMS, (LPARAM)&gstEditTBInfo);
    SendMessage(ghEditTBWnd, TB_AUTOSIZE, 0, 0);

    gpfOrigTBProc = SubclassWindow(ghEditTBWnd, gpfToolbarProc);

    ghInsertTBWnd = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("inserttb"),
                                   WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER,
                                   0, 0, 0, 0, ghRebarWnd, (HMENU)IDTB_INSERT_TOOLBAR, lcInst, nullptr);
    AppUiFontApply(ghInsertTBWnd, FALSE);
    SendMessage(ghInsertTBWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);

    ghInsertImgLst = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 5, 1);
    resnum = IDBMPQ_INSERT_TB_FIRST;
    for (ici = 0; 4 > ici; ici++)
    {
        hImg = LoadBitmap(lcInst, MAKEINTRESOURCE((resnum++)));
        hMsq = LoadBitmap(lcInst, MAKEINTRESOURCE((resnum++)));
        ImageList_Add(ghInsertImgLst, hImg, hMsq);
        DeleteBitmap(hImg);
        DeleteBitmap(hMsq);
    }

    SendMessage(ghInsertTBWnd, TB_SETIMAGELIST, 0, (LPARAM)ghInsertImgLst);

    SendMessage(ghInsertTBWnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(16, 16));

    SendMessage(ghInsertTBWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    ToolBarAssignButtonStrings(ghInsertTBWnd, gstInsertTBInfo, TB_INSERT_ITEMS);
    SendMessage(ghInsertTBWnd, TB_ADDBUTTONS, (WPARAM)TB_INSERT_ITEMS, (LPARAM)&gstInsertTBInfo);
    SendMessage(ghInsertTBWnd, TB_AUTOSIZE, 0, 0);

    gpfOrigTBProc = SubclassWindow(ghInsertTBWnd, gpfToolbarProc);

    ghLayoutTBWnd = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("layouttb"),
                                   WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER,
                                   0, 0, 0, 0, ghRebarWnd, (HMENU)IDTB_LAYOUT_TOOLBAR, lcInst, nullptr);
    AppUiFontApply(ghLayoutTBWnd, FALSE);
    SendMessage(ghLayoutTBWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    ghLayoutImgLst = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 14, 1);
    resnum = IDBMPQ_LAYOUT_TB_FIRST;
    for (ici = 0; 12 > ici; ici++)
    {
        hImg = LoadBitmap(lcInst, MAKEINTRESOURCE((resnum++)));
        hMsq = LoadBitmap(lcInst, MAKEINTRESOURCE((resnum++)));
        ImageList_Add(ghLayoutImgLst, hImg, hMsq);
        DeleteBitmap(hImg);
        DeleteBitmap(hMsq);
    }
    hImg = LoadBitmap(lcInst, MAKEINTRESOURCE(IDBMP_SPLIT_LEFT));
    hMsq = LoadBitmap(lcInst, MAKEINTRESOURCE(IDBMQ_SPLIT_LEFT));
    ImageList_Add(ghLayoutImgLst, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    hImg = LoadBitmap(lcInst, MAKEINTRESOURCE(IDBMP_SPLIT_RIGHT));
    hMsq = LoadBitmap(lcInst, MAKEINTRESOURCE(IDBMQ_SPLIT_RIGHT));
    ImageList_Add(ghLayoutImgLst, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    SendMessage(ghLayoutTBWnd, TB_SETIMAGELIST, 0, (LPARAM)ghLayoutImgLst);

    SendMessage(ghLayoutTBWnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(16, 16));

    SendMessage(ghLayoutTBWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    ToolBarAssignButtonStrings(ghLayoutTBWnd, gstLayoutTBInfo, TB_LAYOUT_ITEMS);

    SendMessage(ghLayoutTBWnd, TB_ADDBUTTONS, (WPARAM)TB_LAYOUT_ITEMS, (LPARAM)&gstLayoutTBInfo);

    SendMessage(ghLayoutTBWnd, TB_AUTOSIZE, 0, 0);

    gpfOrigTBProc = SubclassWindow(ghLayoutTBWnd, gpfToolbarProc);

    ghViewTBWnd = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("viewtb"),
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER,
                                 0, 0, 0, 0, ghRebarWnd, (HMENU)IDTB_VIEW_TOOLBAR, lcInst, nullptr);
    AppUiFontApply(ghViewTBWnd, FALSE);
    SendMessage(ghViewTBWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    ghViewImgLst = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, ARRAYSIZE(gadViewToolbarImageIds), 1);
    for (ici = 0; ARRAYSIZE(gadViewToolbarImageIds) > ici; ici++)
    {
        hImg = LoadBitmap(lcInst, MAKEINTRESOURCE(gadViewToolbarImageIds[ici]));
        hMsq = LoadBitmap(lcInst, MAKEINTRESOURCE(gadViewToolbarMaskIds[ici]));
        ImageList_Add(ghViewImgLst, hImg, hMsq);
        DeleteBitmap(hImg);
        DeleteBitmap(hMsq);
    }
    SendMessage(ghViewTBWnd, TB_SETIMAGELIST, 0, (LPARAM)ghViewImgLst);

    SendMessage(ghViewTBWnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(16, 16));

    SendMessage(ghViewTBWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    ToolBarAssignButtonStrings(ghViewTBWnd, gstViewTBInfo, TB_VIEW_ITEMS);

    SendMessage(ghViewTBWnd, TB_ADDBUTTONS, (WPARAM)TB_VIEW_ITEMS, (LPARAM)&gstViewTBInfo);
    SendMessage(ghViewTBWnd, TB_AUTOSIZE, 0, 0);

    gpfOrigTBProc = SubclassWindow(ghViewTBWnd, gpfToolbarProc);

    ZeroMemory(&stRbandInfo, sizeof(REBARBANDINFO));
    stRbandInfo.cbSize = sizeof(REBARBANDINFO);
    stRbandInfo.fMask = RBBIM_TEXT | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_ID;
    stRbandInfo.cxMinChild = 0;
    stRbandInfo.cyMinChild = 25;

    for (d = 0; TB_BAND_COUNT > d; d++)
    {
        switch (stInfo[d].wID)
        {
        case IDTB_MAIN_TOOLBAR:
            stRbandInfo.lpText = TEXT("파일");
            stRbandInfo.hwndChild = ghMainTBWnd;
            break;

        case IDTB_EDIT_TOOLBAR:
            stRbandInfo.lpText = TEXT("편집");
            stRbandInfo.hwndChild = ghEditTBWnd;
            break;

        case IDTB_INSERT_TOOLBAR:
            stRbandInfo.lpText = TEXT("삽입");
            stRbandInfo.hwndChild = ghInsertTBWnd;
            break;

        case IDTB_LAYOUT_TOOLBAR:
            stRbandInfo.lpText = TEXT("변형");
            stRbandInfo.hwndChild = ghLayoutTBWnd;
            break;

        case IDTB_VIEW_TOOLBAR:
            stRbandInfo.lpText = TEXT("표시");
            stRbandInfo.hwndChild = ghViewTBWnd;
            break;

        default:
            continue;
            break;
        }

        stRbandInfo.wID = stInfo[d].wID;
        stRbandInfo.cx = stInfo[d].cx;
        stRbandInfo.fStyle = stInfo[d].fStyle;

        SendMessage(ghRebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&stRbandInfo);
    }

    return;
}

HRESULT ToolBarInfoChange(LPACCEL pstAccel, INT iEntry)
{
    ToolBarRewriteButtonInfo(ghMainTBWnd, gstMainTBInfo, TB_MAIN_ITEMS, pstAccel, iEntry);
    ToolBarRewriteButtonInfo(ghEditTBWnd, gstEditTBInfo, TB_EDIT_ITEMS, pstAccel, iEntry);
    ToolBarRewriteButtonInfo(ghInsertTBWnd, gstInsertTBInfo, TB_INSERT_ITEMS, pstAccel, iEntry);
    ToolBarRewriteButtonInfo(ghLayoutTBWnd, gstLayoutTBInfo, TB_LAYOUT_ITEMS, pstAccel, iEntry);
    ToolBarRewriteButtonInfo(ghViewTBWnd, gstViewTBInfo, TB_VIEW_ITEMS, pstAccel, iEntry);

    return S_OK;
}

VOID ToolBarDestroy(VOID)
{
    ImageList_Destroy(ghMainImgLst);
    ImageList_Destroy(ghEditImgLst);
    ImageList_Destroy(ghLayoutImgLst);
    ImageList_Destroy(ghInsertImgLst);
    ImageList_Destroy(ghViewImgLst);

    return;
}

HRESULT ToolBarSizeGet(LPRECT pstRect)
{
    RECT rect;

    GetWindowRect(ghRebarWnd, &rect);

    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.left = 0;
    rect.top = 0;

    SetRect(pstRect, rect.left, rect.top, rect.right, rect.bottom);

    return S_OK;
}

HRESULT ToolBarCheckOnOff(UINT itemID, UINT bCheck)
{
    HWND hTlBrWnd;

    switch (itemID)
    {
    default:
        return S_OK;

    case IDM_EXTRACTION_MODE:
        hTlBrWnd = ghEditTBWnd;
        break;
    case IDM_SQSELECT:
        hTlBrWnd = ghEditTBWnd;
        break;
    case IDM_UNICODE_TOGGLE:
        hTlBrWnd = ghEditTBWnd;
        break;

    case IDM_TRACE_MODE_ON:
        hTlBrWnd = ghViewTBWnd;
        break;
    }

    SendMessage(hTlBrWnd, TB_CHECKBUTTON, itemID, bCheck ? TRUE : FALSE);

    return S_OK;
}

HRESULT ToolBarOnSize(HWND hWnd, UINT state, INT cx, INT cy)
{
    FORWARD_WM_SIZE(ghRebarWnd, state, cx, cy, SendMessage);

    return S_OK;
}

VOID ToolBarPseudoDropDown(HWND hWnd, INT itemID)
{
    NMTOOLBAR stNmToolBar;
    INT iFrom;

    ZeroMemory(&stNmToolBar, sizeof(NMTOOLBAR));

    switch (itemID)
    {
    case IDM_IN_UNI_SPACE:
    case IDM_INSTAG_COLOUR:
    case IDM_USERINS_NA:
        stNmToolBar.hdr.hwndFrom = ghInsertTBWnd;
        iFrom = IDTB_INSERT_TOOLBAR;
        break;

    default:
        return;
    }

    stNmToolBar.hdr.idFrom = iFrom;
    stNmToolBar.hdr.code = TBN_DROPDOWN;
    stNmToolBar.iItem = itemID;

    ToolBarOnNotify(hWnd, iFrom, (LPNMHDR)(&stNmToolBar));

    return;
}

LRESULT ToolBarOnContextMenu(HWND hWnd, HWND hWndContext, LONG xPos, LONG yPos)
{
    HMENU hPopupMenu;

    if (ghRebarWnd != hWndContext)
    {
        return 0;
    }

    hPopupMenu = CreatePopupMenu();
    AppendMenu(hPopupMenu, MF_STRING, IDM_REBER_DORESET, UiTextGetLabel(IDM_REBER_DORESET));
    TrackPopupMenu(hPopupMenu, 0, xPos, yPos, 0, hWnd, nullptr);
    DestroyMenu(hPopupMenu);

    return 1;
}

LRESULT ToolBarOnNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    INT iItem;
    HWND hWndFrom;
    HMENU hPopupMenu = nullptr, hMainMenu;
    LPNMTOOLBAR pstNmToolBar;
    TPMPARAMS stTpmParam;
    RECT rect;

    if (IDRB_REBAR == idFrom)
    {
        if (RBN_HEIGHTCHANGE == pstNmhdr->code)
        {
            if (!(AppClientAreaCalc(&rect)))
                return 0;
            ViewSizeMove(hWnd, &rect);
            InvalidateRect(hWnd, nullptr, TRUE);
            InvalidateRect(ghRebarWnd, nullptr, TRUE);
        }
    }

    if (TBN_DROPDOWN == pstNmhdr->code)
    {
        pstNmToolBar = (LPNMTOOLBAR)pstNmhdr;

        iItem = pstNmToolBar->iItem;
        hWndFrom = pstNmToolBar->hdr.hwndFrom;

        hMainMenu = GetMenu(hWnd);

        SendMessage(hWndFrom, TB_GETRECT, (WPARAM)iItem, (LPARAM)(&rect));
        MapWindowPoints(hWndFrom, HWND_DESKTOP, (LPPOINT)(&rect), 2);

        ZeroMemory(&stTpmParam, sizeof(TPMPARAMS));
        stTpmParam.cbSize = sizeof(TPMPARAMS);
        stTpmParam.rcExclude = rect;

        switch (iItem)
        {
        case IDM_OPEN:
            TrackPopupMenuEx(ghHistyMenu, TPM_VERTICAL, rect.left, rect.bottom, hWnd, &stTpmParam);
            break;

        case IDM_IN_UNI_SPACE:
            hPopupMenu = GetSubMenu(GetSubMenu(hMainMenu, 2), 0);
            TrackPopupMenuEx(hPopupMenu, TPM_VERTICAL, rect.left, rect.bottom, hWnd, &stTpmParam);
            break;

        case IDM_INSTAG_COLOUR:
            hPopupMenu = GetSubMenu(GetSubMenu(hMainMenu, 2), 1);
            TrackPopupMenuEx(hPopupMenu, TPM_VERTICAL, rect.left, rect.bottom, hWnd, &stTpmParam);
            break;

        case IDM_FRMINSBOX_OPEN:
            MenuPickerShow(hWnd, MENU_PICKER_FRAME, rect.left, rect.bottom);
            break;

        case IDM_USERINS_NA:
            MenuPickerShow(hWnd, MENU_PICKER_USERITEM, rect.left, rect.bottom);
            break;

        default:
            break;
        }
    }

    return 1;
}

HRESULT ToolBarBandReset(HWND hWnd)
{
    INT index;
    UINT d;
    REBARBANDINFO stRbandInfo;

    ZeroMemory(&stRbandInfo, sizeof(REBARBANDINFO));
    stRbandInfo.cbSize = sizeof(REBARBANDINFO);
    stRbandInfo.fMask = RBBIM_STYLE | RBBIM_SIZE;

    for (d = 0; TB_BAND_COUNT > d; d++)
    {
        index = SendMessage(ghRebarWnd, RB_IDTOINDEX, gcstReBarDef[d].wID, 0);
        if (0 > index)
        {
            continue;
        }

        SendMessage(ghRebarWnd, RB_MOVEBAND, index, d);

        stRbandInfo.cx = gcstReBarDef[d].cx;
        stRbandInfo.fStyle = gcstReBarDef[d].fStyle;
        SendMessage(ghRebarWnd, RB_SETBANDINFO, (WPARAM)d, (LPARAM)&stRbandInfo);
    }

    InvalidateRect(ghRebarWnd, nullptr, TRUE);

    return S_OK;
}

UINT ToolBarBandInfoGet(LPVOID pVoid)
{
    UINT d;
    REBARBANDINFO stBandInfo;
    REBARLAYOUTINFO stInfo[TB_BAND_COUNT];

    ZeroMemory(&stBandInfo, sizeof(REBARBANDINFO));
    stBandInfo.cbSize = sizeof(REBARBANDINFO);
    stBandInfo.fMask = RBBIM_ID | RBBIM_STYLE | RBBIM_SIZE;

    ZeroMemory(stInfo, sizeof(stInfo));

    for (d = 0; TB_BAND_COUNT > d; d++)
    {
        SendMessage(ghRebarWnd, RB_GETBANDINFO, (WPARAM)d, (LPARAM)(&stBandInfo));
        stInfo[d].wID = stBandInfo.wID;
        stInfo[d].cx = stBandInfo.cx;
        stInfo[d].fStyle = stBandInfo.fStyle;
    }

    InitToolBarLayout(INIT_SAVE, TB_BAND_COUNT, stInfo);

    return TB_BAND_COUNT;
}
