// 페이지 단위 생성/삭제/전환 관리
#include "AppModuleInternal.h"
#include "DocContextInternal.h"
#include "Palette.h"
#include "PageListInternal.h"
#include "UiText.h"
#include "MenuMnemonic.h"
//-------------------------------------------------------------------------------------------------

namespace
{
constexpr LPCTSTR kPageListClass = TEXT("PAGE_LIST");
constexpr INT kPageListDefaultWidth = 320;
constexpr INT kPageListDefaultHeight = 300;
constexpr UINT_PTR kPageListTooltipMaxChars = 4096;
constexpr UINT kPageListTooltipMaxLines = 32;
}
//-------------------------------------------------------------------------------------------------

extern INT gixDropPage;  //    投下ホット番号
//-------------------------------------------------------------------------------------------------

static PAGE_LIST_CONTROLLER_STATE gstPageListControllerState;

static PAGE_LIST_CONTROLLER_STATE &PageListStateGet()
{
    return gstPageListControllerState;
}

static PAGE_LIST_CONTROLLER_STATE &PageListStateResolve(HWND hWnd)
{
    PAGE_LIST_CONTROLLER_STATE *pstState;

    pstState = PageListControllerStateFromWindow(hWnd);
    if (pstState)
        return *pstState;

    return PageListStateGet();
}

static HWND PageListWindowGet()
{
    return PageListStateGet().hPageWindow;
}

static HWND PageListToolWindowGet()
{
    return PageListStateGet().hToolWindow;
}

static HWND PageListViewWindowGet()
{
    return PageListStateGet().hPageListWindow;
}

static VOID PageListAttachState(HWND hWnd, PAGE_LIST_CONTROLLER_STATE *pstState)
{
    if (hWnd)
    {
        WndTagSet(hWnd, reinterpret_cast<LONG_PTR>(pstState));
    }
}

static VOID PageListStateReset(PAGE_LIST_CONTROLLER_STATE *pstState)
{
    if (!pstState)
        return;

    *pstState = PAGE_LIST_CONTROLLER_STATE{};
}
//-------------------------------------------------------------------------------------------------

static TBBUTTON gstPgTlBarInfo[] = {
    {0, IDM_PAGEL_ADD, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {1, IDM_PAGEL_INSERT, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {2, IDM_PAGEL_DUPLICATE, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {3, IDM_PAGEL_DELETE, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {4, IDM_PAGEL_UPFLOW, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {5, IDM_PAGEL_DOWNSINK, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {6, IDM_PAGEL_RENAME, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0}
};
static constexpr UINT kPageToolbarItemCount = static_cast<UINT>(std::size(gstPgTlBarInfo));
static const UINT gadPageToolbarImageIds[kPageToolbarItemCount] = {
    IDBMP_PAGECREATE,
    IDBMP_PAGEINSERT,
    IDBMP_PAGEDUPLICATE,
    IDBMP_PAGEDELETE,
    IDBMP_PAGEUPPERSHIFT,
    IDBMP_PAGEUNDERSHIFT,
    IDBMP_PAGENAMECHANGE};
static const UINT gadPageToolbarMaskIds[kPageToolbarItemCount] = {
    IDBMQ_PAGECREATE,
    IDBMQ_PAGEINSERT,
    IDBMQ_PAGEDUPLICATE,
    IDBMQ_PAGEDELETE,
    IDBMQ_PAGEUPPERSHIFT,
    IDBMQ_PAGEUNDERSHIFT,
    IDBMQ_PAGENAMECHANGE};

//-------------------------------------------------------------------------------------------------

typedef struct tagPAGEDELETEBOXMSG
{
    TCHAR atMsg1[MAX_PATH];
    TCHAR atMsg2[MAX_PATH];
    UINT bChecked;

} PAGEDELETEBOXMSG, *LPPAGEDELETEBOXMSG;

static INT_PTR CALLBACK PageListDeleteCheckBoxProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR PageListDeleteCheckBox(HWND, LPCTSTR, LPCTSTR, UINT);

LRESULT CALLBACK PageListProc(HWND, UINT, WPARAM, LPARAM);
static VOID Plt_OnCommand(HWND, INT, HWND, UINT);
VOID Plt_OnSize(HWND, UINT, INT, INT);
static LRESULT Plt_OnNotify(HWND, INT, LPNMHDR);
static VOID Plt_OnContextMenu(HWND, HWND, UINT, UINT);

static LRESULT PageListNotify(HWND, LPNMLISTVIEW);
static HRESULT PageListNameChange(INT);
static HRESULT PageListSpinning(INT, INT);
static HRESULT PageListDuplicate(INT);

static HRESULT PageListJump(INT);
static INT PageListSelectedIndexGet();

static LRESULT CALLBACK gpfPageViewProc(HWND, UINT, WPARAM, LPARAM);
static VOID Plv_OnMouseMove(HWND, INT, INT, UINT);

static LRESULT CALLBACK gpfPageToolProc(HWND, UINT, WPARAM, LPARAM);
static VOID PageToolBarAssignToolTips(HWND);
static VOID PageToolBarRewriteToolTips(HWND, LPACCEL, INT);

static INT_PTR CALLBACK PageNameDlgProc(HWND, UINT, WPARAM, LPARAM);

#ifdef USE_HOVERTIP
static LPTSTR CALLBACK PageListHoverTipInfo(LPVOID);
#endif
static HRESULT PageListPreviewTextGetAlloc(INT, LPTSTR *);
//-------------------------------------------------------------------------------------------------

static INT_PTR CALLBACK PageListDeleteCheckBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPAGEDELETEBOXMSG pcstMsg;
    INT id;

    pcstMsg = reinterpret_cast<LPPAGEDELETEBOXMSG>(WndTagGet(hDlg));

    switch (message)
    {
    default:
        break;

    case WM_INITDIALOG:
        pcstMsg = reinterpret_cast<LPPAGEDELETEBOXMSG>(lParam);
        WndTagSet(hDlg, lParam);
        SetDlgItemText(hDlg, IDS_MC_MSG1, pcstMsg->atMsg1);
        SetDlgItemText(hDlg, IDS_MC_MSG2, pcstMsg->atMsg2);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        id = LOWORD(wParam);
        if (pcstMsg)
        {
            pcstMsg->bChecked = IsDlgButtonChecked(hDlg, IDCB_MC_CHECKBOX);
        }
        if (IDYES == id || IDOK == id)
        {
            EndDialog(hDlg, IDYES);
        }
        if (IDNO == id || IDCANCEL == id)
        {
            EndDialog(hDlg, IDNO);
        }
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

static INT_PTR PageListDeleteCheckBox(HWND hWnd, LPCTSTR ptMsg1, LPCTSTR ptMsg2, UINT dIniKey)
{
    INT_PTR iRslt;
    PAGEDELETEBOXMSG stMsg;
    const PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();

    if (nullptr == ptMsg1 || nullptr == ptMsg2)
        return IDCANCEL;

    StringCchCopy(stMsg.atMsg1, MAX_PATH, ptMsg1);
    StringCchCopy(stMsg.atMsg2, MAX_PATH, ptMsg2);

    stMsg.bChecked = InitParamValue(INIT_LOAD, dIniKey, 0);
    if (1 == stMsg.bChecked)
    {
        iRslt = IDYES;
    }
    else
    {
        iRslt = DialogBoxParam(stState.hInstance, MAKEINTRESOURCE(IDD_MSGCHECKBOX_DLG), hWnd, PageListDeleteCheckBoxProc, (LPARAM)(&stMsg));
        InitParamValue(INIT_SAVE, dIniKey, (BST_CHECKED == stMsg.bChecked) ? 1 : 0);
    }

    return iRslt;
}
//-------------------------------------------------------------------------------------------------

static INT_PTR PageListDeleteConfirm(HWND hWnd)
{
    return PageListDeleteCheckBox(hWnd, ORR_UI_MSG_PAGEL_DELETE_LINE1, ORR_UI_MSG_PAGEL_DELETE_LINE2, VL_PDELETE_NM);
}

static HRESULT PageListSelectAndSync(INT iPage, BOOLEAN bReturnFocus, BOOLEAN bEnsureVisible)
{
    if (0 > iPage)
        return E_INVALIDARG;

    if (bEnsureVisible)
    {
        ListView_EnsureVisible(PageListViewWindowGet(), iPage, FALSE);
    }

    DocPageChange(iPage);

    if (bReturnFocus)
    {
        ViewFocusSet();
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

static VOID PageToolBarAssignToolTips(HWND hWnd)
{
    UINT i;
    LPCTSTR ptText;
    TCHAR atBuff[MAX_STRING];

    for (i = 0; kPageToolbarItemCount > i; i++)
    {
        ptText = UiTextGetLabel(gstPgTlBarInfo[i].idCommand);
        if (nullptr == ptText)
            continue;

        StringCchCopy(atBuff, MAX_STRING, ptText);
        gstPgTlBarInfo[i].iString = SendMessage(hWnd, TB_ADDSTRING, 0, (LPARAM)atBuff);
    }
}

static VOID PageToolBarRewriteToolTips(HWND hWnd, LPACCEL pstAccel, INT iEntry)
{
    UINT i;
    LPCTSTR ptText;
    TCHAR atText[MAX_STRING];
    TBBUTTONINFO stButtonInfo;

    stButtonInfo = {};
    stButtonInfo.cbSize = sizeof(TBBUTTONINFO);
    stButtonInfo.dwMask = TBIF_TEXT;
    stButtonInfo.pszText = atText;
    stButtonInfo.cchText = MAX_STRING;

    for (i = 0; kPageToolbarItemCount > i; i++)
    {
        ptText = UiTextGetLabel(gstPgTlBarInfo[i].idCommand);
        if (nullptr == ptText)
            continue;

        StringCchCopy(atText, MAX_STRING, ptText);
        if (nullptr != pstAccel)
        {
            AccelKeyTextBuild(atText, MAX_STRING, gstPgTlBarInfo[i].idCommand, pstAccel, iEntry);
        }
        SendMessage(hWnd, TB_SETBUTTONINFO, (WPARAM)(gstPgTlBarInfo[i].idCommand), (LPARAM)&stButtonInfo);
    }
}

HRESULT PageToolBarInfoChange(LPACCEL pstAccel, INT iEntry)
{
    if (!PageListToolWindowGet())
        return S_FALSE;

    PageToolBarRewriteToolTips(PageListToolWindowGet(), pstAccel, iEntry);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

static HRESULT PageListPreviewTextGetAlloc(INT iPage, LPTSTR *pptText)
{
    ONEPAGE *pstPage;
    wstring wsPreview;
    UINT_PTR cchCount;
    UINT dLineCount;
    BOOLEAN bTruncated;

    if (!pptText)
        return E_POINTER;
    *pptText = nullptr;

    if (0 > iPage || DocCurrentFile().vcCont.size() <= static_cast<size_t>(iPage))
        return E_INVALIDARG;

    pstPage = &DocCurrentFile().vcCont.at(iPage);
    wsPreview.reserve(kPageListTooltipMaxChars + 8);

    cchCount = 0;
    dLineCount = 0;
    bTruncated = FALSE;

    auto appendChar = [&](TCHAR cchChar) -> BOOLEAN
    {
        if (kPageListTooltipMaxChars <= cchCount)
        {
            bTruncated = TRUE;
            return FALSE;
        }

        wsPreview.push_back(cchChar);
        cchCount++;
        return TRUE;
    };

    auto appendCrLf = [&]() -> BOOLEAN
    {
        if (kPageListTooltipMaxLines <= (dLineCount + 1))
        {
            bTruncated = TRUE;
            return FALSE;
        }

        if (!appendChar(TEXT('\r')))
            return FALSE;
        if (!appendChar(TEXT('\n')))
            return FALSE;

        dLineCount++;
        return TRUE;
    };

    if (pstPage->ptRawData)
    {
        const TCHAR *ptText = pstPage->ptRawData;

        while (*ptText)
        {
            if (TEXT('\r') == *ptText && TEXT('\n') == *(ptText + 1))
            {
                if (!appendCrLf())
                    break;
                ptText += 2;
                continue;
            }

            if (!appendChar(*ptText))
                break;
            ptText++;
        }
    }
    else
    {
        LINE_ITR itLine;
        LINE_ITR endLine;

        itLine = pstPage->ltPage.begin();
        endLine = pstPage->ltPage.end();

        for (; itLine != endLine; itLine++)
        {
            const size_t dLetters = itLine->vcLine.size();

            for (size_t i = 0; dLetters > i; i++)
            {
                if (!appendChar(itLine->vcLine.at(i).cchMozi))
                    break;
            }

            if (bTruncated)
                break;

            if (endLine != std::next(itLine))
            {
                if (!appendCrLf())
                    break;
            }
        }
    }

    if (bTruncated)
    {
        if (wsPreview.size() + 3 <= kPageListTooltipMaxChars)
        {
            wsPreview += TEXT("...");
        }
        else if (3 <= wsPreview.size())
        {
            wsPreview.replace(wsPreview.size() - 3, 3, TEXT("..."));
        }
    }

    *pptText = (LPTSTR)malloc((wsPreview.size() + 1) * sizeof(TCHAR));
    if (!(*pptText))
        return E_OUTOFMEMORY;

    ZeroMemory(*pptText, (wsPreview.size() + 1) * sizeof(TCHAR));
    StringCchCopy(*pptText, wsPreview.size() + 1, wsPreview.c_str());

    return S_OK;
}

// ページ用りすとびゅ～の作成
HWND PageListInitialise(HINSTANCE hInstance, HWND hParentWnd, LPRECT pstFrame)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();

    LVCOLUMN stLvColm;
    RECT tbRect;
    DWORD dwExStyle, dwStyle;
    HWND hPrWnd;

    UINT ici;
    HBITMAP hImg, hMsq;
    WNDCLASSEX wcex;
    RECT wdRect, clRect, rect;

    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PageListProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr; // MAKEINTRESOURCE(IDC_PGLVPOPUPMENU);
    wcex.lpszClassName = kPageListClass;
    wcex.hIconSm = nullptr;

    RegisterClassEx(&wcex);

    PageListStateReset(&stState);
    stState.hInstance = hInstance;
    stState.bPageTooltipView = InitParamValue(INIT_LOAD, VL_PAGETIP_VIEW, 1);

    //    フォーカス操作
    stState.bReturnFocus = InitParamValue(INIT_LOAD, VL_PGL_RETFCS, 0);

    InitWindowPos(INIT_LOAD, WDP_PLIST, &rect);
    if (0 == rect.right || 0 == rect.bottom) //    幅高さが０はデータ無し
    {
        GetWindowRect(hParentWnd, &wdRect);
        rect.left = wdRect.left - kPageListDefaultWidth;
        rect.top = wdRect.top;
        rect.right = kPageListDefaultWidth;
        rect.bottom = kPageListDefaultHeight;
        InitWindowPos(INIT_SAVE, WDP_PLIST, &rect); //    起動時保存
    }

    if (gbTmpltDock) //    メーンウィンドーにドッキュする
    {
        const APP_DOCKED_WINDOW_LAYOUT stDockedLayout = AppLayoutDockedWindowsCalc(pstFrame, grdSplitPos, 0, gbDockTmplView);

        hPrWnd = hParentWnd;
        dwExStyle = 0;
        dwStyle = WS_CHILD | WS_VISIBLE;
        rect = stDockedLayout.stPageListRect; //    クライヤント에 쓰는 영역
    }
    else
    {
        dwExStyle = WS_EX_TOOLWINDOW;
        if (InitWindowTopMost(INIT_LOAD, WDP_PLIST, 0))
        {
            dwExStyle |= WS_EX_TOPMOST;
        }
        dwStyle = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
        hPrWnd = nullptr;
    }
    stState.hPageWindow = CreateWindowEx(dwExStyle, kPageListClass, TEXT("페이지 목록"), dwStyle,
                                         rect.left, rect.top, rect.right, rect.bottom, hPrWnd, nullptr, hInstance, nullptr);
    //    ウインドウを作成
    PageListAttachState(stState.hPageWindow, &stState);

    GetClientRect(stState.hPageWindow, &clRect);

    //    ツールバー・縦のやつ
    stState.hToolWindow = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("pagetoolbar"),
                                         WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_LEFT | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS, // | TBSTYLE_WRAPABLE,
                                         0, 0, 0, 0, stState.hPageWindow, (HMENU)IDTB_PAGE_TOOLBAR, hInstance, nullptr);
    PageListAttachState(stState.hToolWindow, &stState);
    AppUiFontApply(stState.hToolWindow, FALSE);
    //    自動ツールチップスタイルを追加
    SendMessage(stState.hToolWindow, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    stState.hToolbarImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, kPageToolbarItemCount, 1);
    for (ici = 0; kPageToolbarItemCount > ici; ici++)
    {
        hImg = LoadBitmap(hInstance, MAKEINTRESOURCE(gadPageToolbarImageIds[ici]));
        hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE(gadPageToolbarMaskIds[ici]));
        ImageList_Add(stState.hToolbarImageList, hImg, hMsq); //    イメージリストにイメージを追加
        DeleteBitmap(hImg);
        DeleteBitmap(hMsq);
    }
    SendMessage(stState.hToolWindow, TB_SETIMAGELIST, 0, (LPARAM)stState.hToolbarImageList);

    SendMessage(stState.hToolWindow, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    PageToolBarAssignToolTips(stState.hToolWindow);

    SendMessage(stState.hToolWindow, TB_SETROWS, MAKEWPARAM(kPageToolbarItemCount, TRUE), (LPARAM)(&tbRect));

    SendMessage(stState.hToolWindow, TB_ADDBUTTONS, (WPARAM)kPageToolbarItemCount, (LPARAM)&gstPgTlBarInfo);

    SendMessage(stState.hToolWindow, TB_GETITEMRECT, 0, (LPARAM)(&tbRect));
    MoveWindow(stState.hToolWindow, 0, 0, tbRect.right, rect.bottom, TRUE);
    InvalidateRect(stState.hToolWindow, nullptr, TRUE); //    クライヤント全体を再描画する

    //    サブクラス化
    stState.pfOrigPageToolProc = SubclassWindow(stState.hToolWindow, gpfPageToolProc);

    tbRect.bottom = rect.bottom;
    tbRect.left = 0;
    tbRect.top = 0;

    // リストビュー    LVS_SHOWSELALWAYS
    stState.hPageListWindow = CreateWindowEx(0, WC_LISTVIEW, TEXT("pagelist"),
                                             WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL,
                                             tbRect.right, clRect.top, clRect.right - tbRect.right, clRect.bottom, stState.hPageWindow,
                                             (HMENU)IDLV_PAGELISTVIEW, hInstance, nullptr);
    PageListAttachState(stState.hPageListWindow, &stState);
    ListView_SetExtendedListViewStyle(stState.hPageListWindow, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SetWindowFont(stState.hPageListWindow, AppUiFontGet(), TRUE);

    stState.pfOrigPageViewProc = SubclassWindow(stState.hPageListWindow, gpfPageViewProc);

    ZeroMemory(&stLvColm, sizeof(LVCOLUMN));
    stLvColm.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    stLvColm.fmt = LVCFMT_LEFT;
    stLvColm.pszText = TEXT("No");
    stLvColm.cx = 28;
    stLvColm.iSubItem = 0;
    ListView_InsertColumn(stState.hPageListWindow, 0, &stLvColm);
    stLvColm.pszText = TEXT("이름");
    stLvColm.cx = 67;
    stLvColm.iSubItem = 1;
    ListView_InsertColumn(stState.hPageListWindow, 1, &stLvColm);
    stLvColm.pszText = TEXT("용량");
    stLvColm.cx = 45;
    stLvColm.iSubItem = 2;
    ListView_InsertColumn(stState.hPageListWindow, 2, &stLvColm);
    stLvColm.pszText = TEXT("줄 수");
    stLvColm.cx = 45;
    stLvColm.iSubItem = 3;
    ListView_InsertColumn(stState.hPageListWindow, 3, &stLvColm);

    ShowWindow(stState.hPageWindow, SW_SHOW);
    UpdateWindow(stState.hPageWindow);

#ifdef USE_HOVERTIP
    HoverTipInitialise(hInstance, stState.hPageWindow);
#endif

    return stState.hPageWindow;
}
//-------------------------------------------------------------------------------------------------

// ツールバーサブクラス
static LRESULT CALLBACK gpfPageToolProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateResolve(hWnd);

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

    return CallWindowProc(stState.pfOrigPageToolProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// ウインドウプロシージャ
LRESULT CALLBACK PageListProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateResolve(hWnd);

    switch (message)
    {
        HANDLE_MSG(hWnd, WM_COMMAND, Plt_OnCommand);
        HANDLE_MSG(hWnd, WM_SIZE, Plt_OnSize);
        HANDLE_MSG(hWnd, WM_NOTIFY, Plt_OnNotify); //    コモンコントロールの個別イベント
        HANDLE_MSG(hWnd, WM_CONTEXTMENU, Plt_OnContextMenu);

    case WM_DESTROY:
    #ifdef USE_HOVERTIP
        HoverTipInitialise(nullptr, nullptr);
    #endif
        if (stState.hToolbarImageList)
        {
            ImageList_Destroy(stState.hToolbarImageList);
            stState.hToolbarImageList = nullptr;
        }
        return 0;

    case WM_CLOSE:
        ShowWindow(stState.hPageWindow, SW_HIDE);
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

static INT PageListSelectedIndexGet()
{
    const INT iItem = ListView_GetNextItem(PageListViewWindowGet(), -1, LVNI_ALL | LVNI_SELECTED);
    return (0 <= iItem) ? iItem : DocCurrentPageIndex();
}

// COMMANDメッセージの受け取り。ボタン押されたとかで発生
static VOID Plt_OnCommand(HWND hWnd, INT id, HWND, UINT)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();
    INT iPage, iItem, mRslt;
    LONG_PTR rdExStyle;

    switch (id)
    {
    case IDM_WINDOW_CHANGE:
        WindowFocusChange(WND_PAGE, 1);
        return;
    case IDM_WINDOW_CHG_RVRS:
        WindowFocusChange(WND_PAGE, -1);
        return;

    case IDM_PAGEL_ADD: //    末尾新規作成はいつでも有効
        iPage = DocPageCreate(-1);
        PageListInsert(iPage); //    ページリストビューに追加
        PageListSelectAndSync(iPage, TRUE, TRUE);
        DocModifyContent(TRUE);
        DocFileBackup(hWnd); //    バックアップしておく
        return;

    case IDM_TOPMOST_TOGGLE: //    常時最全面と通常ウインドウのトグル
        rdExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        if (WS_EX_TOPMOST & rdExStyle)
        {
            SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            InitWindowTopMost(INIT_SAVE, WDP_PLIST, 0);
        }
        else
        {
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            InitWindowTopMost(INIT_SAVE, WDP_PLIST, 1);
        }
        return;

    case IDM_PAGEL_AATIP_TOGGLE: //    内容ポッパップするか
        stState.bPageTooltipView = stState.bPageTooltipView ? FALSE : TRUE;
        InitParamValue(INIT_SAVE, VL_PAGETIP_VIEW, stState.bPageTooltipView);
        return;

    case IDM_PAGEL_RETURN_FOCUS: //    頁選択したらフォーカス戻すか
        stState.bReturnFocus = stState.bReturnFocus ? FALSE : TRUE;
        InitParamValue(INIT_SAVE, VL_PGL_RETFCS, stState.bReturnFocus);
        return;

    default:
        break;
    }

    iItem = PageListSelectedIndexGet();

    switch (id)
    {
    case IDM_PAGE_PREV: //    前の頁へ移動
        PageListJump(iItem - 1);
        return;

    case IDM_PAGE_NEXT: //    次の頁へ移動
        PageListJump(iItem + 1);
        return;

    case IDM_PAGEL_INSERT: //    任意位置新規作成
        iPage = DocPageCreate(iItem);
        PageListInsert(iPage); //    ページリストビューに追加
        PageListSelectAndSync(iPage, FALSE, TRUE);
        DocFileBackup(hWnd); //    バックアップしておく
        break;

    case IDM_PAGEL_RENAME: //    頁名称変更
        if (FAILED(PageListNameChange(iItem)))
        {
            return;
        }
        break;

    case IDM_PAGEL_DELETE: //    この頁を削除
        mRslt = PageListDeleteConfirm(hWnd);
        if (IDYES == mRslt)
        {
            DocPageDelete(iItem, -1);
        }
        break;

    case IDM_PAGEL_UPFLOW: //    ↑移動
        PageListSpinning(iItem, 1);
        break;

    case IDM_PAGEL_DOWNSINK: //    ↓移動
        PageListSpinning(iItem, -1);
        break;

    case IDM_PAGEL_DUPLICATE: //    頁複製
        PageListDuplicate(iItem);
        break;

    default:
        return;
    }

    DocModifyContent(TRUE);

    ViewFocusSet();

    return;
}
//-------------------------------------------------------------------------------------------------

// サイズ変更された
VOID Plt_OnSize(HWND hWnd, UINT state, INT cx, INT cy)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateResolve(hWnd);
    RECT tbRect;

    tbRect.right = 0;
    if (stState.hToolWindow)
    {
        SendMessage(stState.hToolWindow, TB_GETITEMRECT, 0, (LPARAM)(&tbRect));
        MoveWindow(stState.hToolWindow, 0, 0, tbRect.right, cy, TRUE);
    }

    MoveWindow(stState.hPageListWindow, tbRect.right, 0, cx - tbRect.right, cy, TRUE);
    return;
}
//-------------------------------------------------------------------------------------------------

// ノーティファイメッセージの処理
static LRESULT Plt_OnNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    if (IDLV_PAGELISTVIEW == idFrom)
    {
        return PageListNotify(hWnd, (LPNMLISTVIEW)pstNmhdr);
    }

    return 0; //    何もないなら０を戻す
}
//-------------------------------------------------------------------------------------------------

// コンテキストメニュー呼びだしアクション(要は右クルック）
static VOID Plt_OnContextMenu(HWND hWnd, HWND, UINT xPos, UINT yPos)
{
    const PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();
    HMENU hMenu, hSubMenu;
    INT iCount, iItem;
    BOOLEAN bSel;
    LONG_PTR rdExStyle;

    POINT stPoint;

    stPoint.x = (SHORT)xPos; //    画面座標はマイナスもありうる
    stPoint.y = (SHORT)yPos;

    bSel = FALSE;
    iCount = ListView_GetItemCount(stState.hPageListWindow);
    iItem = ListView_GetNextItem(stState.hPageListWindow, -1, LVNI_ALL | LVNI_SELECTED);
    if (0 <= iItem)
        bSel = TRUE;

    hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDC_PGLVPOPUPMENU));
    hSubMenu = GetSubMenu(hMenu, 0);
    MenuMnemonicApply(hSubMenu);

    //    頁が１しかないなら、削除を無効に
    if (1 >= iCount)
    {
        EnableMenuItem(hSubMenu, IDM_PAGEL_DELETE, MF_GRAYED);
    }

    if (gbTmpltDock)
    {
        EnableMenuItem(hSubMenu, IDM_TOPMOST_TOGGLE, MF_GRAYED);
    }
    else
    {
        //    最前面であるか
        rdExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        if (WS_EX_TOPMOST & rdExStyle)
        {
            CheckMenuItem(hSubMenu, IDM_TOPMOST_TOGGLE, MF_BYCOMMAND | MF_CHECKED);
        }
    }

    //    ポッパップ表示有るか？
    if (stState.bPageTooltipView)
    {
        CheckMenuItem(hSubMenu, IDM_PAGEL_AATIP_TOGGLE, MF_CHECKED);
    }

    //    選択でフォーカス戻りか？
    if (stState.bReturnFocus)
    {
        CheckMenuItem(hSubMenu, IDM_PAGEL_RETURN_FOCUS, MF_CHECKED);
    }

    //    未選択なら、選択してないと使えない機能を無効に
    if (!(bSel))
    {
        EnableMenuItem(hSubMenu, IDM_PAGEL_INSERT, MF_GRAYED);    //    任意作成
        EnableMenuItem(hSubMenu, IDM_PAGEL_DELETE, MF_GRAYED);    //    削除
        EnableMenuItem(hSubMenu, IDM_PAGEL_UPFLOW, MF_GRAYED);    //    浮上
        EnableMenuItem(hSubMenu, IDM_PAGEL_DOWNSINK, MF_GRAYED);  //    沈降
        EnableMenuItem(hSubMenu, IDM_PAGEL_DUPLICATE, MF_GRAYED); //    複製
        EnableMenuItem(hSubMenu, IDM_PAGEL_RENAME, MF_GRAYED);    //    名称
    }

    TrackPopupMenu(hSubMenu, 0, stPoint.x, stPoint.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);

    return;
}
//-------------------------------------------------------------------------------------------------

// フローティング頁一覧の位置リセット
HRESULT PageListPositionReset(HWND hMainWnd)
{
    RECT wdRect, rect;

    GetWindowRect(hMainWnd, &wdRect);
    rect.left = wdRect.left - kPageListDefaultWidth;
    rect.top = wdRect.top;
    rect.right = kPageListDefaultWidth;
    rect.bottom = kPageListDefaultHeight;

    SetWindowPos(PageListWindowGet(), HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_SHOWWINDOW | SWP_NOZORDER);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

/* !
    親ウインドウが移動したり大きさ変わったら
    @param[in]    hPrntWnd    親ウインドウハンドル
    @param[in]    pstFrame    クライアントサイズ
*/
VOID PageListResize(HWND hPrntWnd, LPRECT pstFrame)
{
    const APP_DOCKED_WINDOW_LAYOUT stDockedLayout = AppLayoutDockedWindowsCalc(
        pstFrame,
        grdSplitPos,
        AppLayoutDockTabHeightGet(DockingTabGet()),
        gbDockTmplView);

    UNREFERENCED_PARAMETER(hPrntWnd);

    SetWindowPos(PageListWindowGet(), HWND_TOP,
                 stDockedLayout.stPageListRect.left,
                 stDockedLayout.stPageListRect.top,
                 stDockedLayout.stPageListRect.right,
                 stDockedLayout.stPageListRect.bottom,
                 SWP_SHOWWINDOW);

    return;
}
//-------------------------------------------------------------------------------------------------

// リストびゅーのNOTIFYメッセージ処理
static LRESULT PageListNotify(HWND, LPNMLISTVIEW pstLv)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();
    HWND hLvWnd;
    INT iCount, iItem, nmCode; //, iPage;

    INT iSel;

    DWORD lvClmn;
    INT lvLine;
    LPNMLVCUSTOMDRAW pstDraw;

    PAGEINFOS stInfo;

    hLvWnd = pstLv->hdr.hwndFrom;
    nmCode = pstLv->hdr.code;
    //    なんらかのアクションの起こったROW位置をゲットする
    iCount = ListView_GetItemCount(hLvWnd);
    iItem = pstLv->iItem;

    //    普通のクルックについて
    if (NM_CLICK == nmCode)
    {
        if (0 <= iItem) //    該当ページへの移動
        {
            PageListSelectAndSync(iItem, stState.bReturnFocus, FALSE);
        }
    }

    //    ダブルクルック
    if (NM_DBLCLK == nmCode)
    {
        if (0 <= iItem) //    頁名称DIALOGUEをニョキ
        {
            if (SUCCEEDED(PageListNameChange(iItem)))
            {
                DocModifyContent(TRUE);
            }
        }
    }

    if (NM_RETURN == nmCode)
    {
        iSel = ListView_GetNextItem(hLvWnd, -1, LVNI_ALL | LVNI_SELECTED);
        if (0 > iSel)
            return 0; //    選択してなかったら終わり
    }
    //    NM_KEYDOWN    NM_CHAR    関知できず

    // カスタムドロー・サブクラスの中だと上手くいかない・Why?
    if (NM_CUSTOMDRAW == nmCode)
    {
        pstDraw = (LPNMLVCUSTOMDRAW)pstLv;

        //    ここで扱う処理内容を返す。いわゆる初回登録
        if (pstDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
        {
            return CDRF_NOTIFYSUBITEMDRAW;
        }
        if (pstDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
        {
            return CDRF_NOTIFYSUBITEMDRAW;
        }

        if (pstDraw->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) //(CDDS_ITEMPREPAINT|CDDS_SUBITEM)
        {
            const BOOLEAN bDelayed = PageIsDelayed(DocCurrentFileIterator(), pstDraw->nmcd.dwItemSpec);
            const BOOLEAN bVisited = DocCurrentFile().vcCont.at(pstDraw->nmcd.dwItemSpec).bVisited;

            lvLine = pstDraw->nmcd.dwItemSpec; //    行・頁番号になる
            lvClmn = pstDraw->iSubItem;        //    カラム

            if (DocCurrentPageIndex() == lvLine) //    今の行・とりあえず青
            {
                pstDraw->clrTextBk = 0x00FF8080;
            }
            else
            {
                pstDraw->clrTextBk = 0xFF000000; //    これでデフォ色指定・多分
            }

            if (0 == lvClmn)
            {
                if (bDelayed)
                {
                    pstDraw->clrTextBk = 0x00C0C0C0;
                }
                else if (!(bVisited))
                {
                    pstDraw->clrTextBk = 0x00E0E0E0;
                }
            }

            //    再描画せずともリヤルに変わる・バイト数計算のところで再描画してる？
            if (2 == lvClmn)
            {
                stInfo.dMasqus = PI_BYTES;
                NowPageInfoGet(lvLine, &stInfo);

                if (gdPageByteMax < (UINT)(stInfo.iBytes))
                    pstDraw->clrTextBk = 0x000000FF;
            }

            return CDRF_NEWFONT;
        }
    }

    return 0; //    通常は０
}
//-------------------------------------------------------------------------------------------------

// リストビュークリヤ
HRESULT PageListClear(VOID)
{
    ListView_DeleteAllItems(PageListViewWindowGet());

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 開いている頁内容を変更
HRESULT PageListViewChange(INT iPage, INT iPrePage)
{
    //    フォーカスページは、ここに来る前に変更しておくこと

    LONG iItem;

    SendMessage(PageListViewWindowGet(), LVM_REDRAWITEMS, iPrePage, iPrePage);

    iItem = ListView_GetItemCount(PageListViewWindowGet());
    if (iItem <= iPage || 0 > iPage)
        return E_OUTOFMEMORY;

    //    選択状態を変更
    ListView_SetItemState(PageListViewWindowGet(), iPage, LVIS_SELECTED, LVIS_SELECTED);

    //    ビューを書き直し
    ViewEditReset();

    gixDropPage = iPage;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 指定の位置にページを追加
HRESULT PageListInsert(INT iBefore)
{
    UINT iItem, i;
    TCHAR atBuffer[MIN_STRING];
    LVITEM stLvi;

    iItem = ListView_GetItemCount(PageListViewWindowGet());

    stLvi = {};
    stLvi.mask = LVIF_TEXT;

    ZeroMemory(atBuffer, sizeof(atBuffer));

    if (0 > iBefore)
    {
        stLvi.iItem = iItem;

        StringCchPrintf(atBuffer, MIN_STRING, TEXT("%u"), iItem + 1);
        stLvi.pszText = atBuffer;
    }
    else
    {
        stLvi.iItem = iBefore;
        stLvi.pszText = TEXT("");
    }

    stLvi.iSubItem = 0;
    ListView_InsertItem(PageListViewWindowGet(), &stLvi);

    stLvi.pszText = TEXT("");
    stLvi.iSubItem = 1;
    ListView_SetItem(PageListViewWindowGet(), &stLvi);

    stLvi.pszText = TEXT("0"); //    byte
    stLvi.iSubItem = 2;
    ListView_SetItem(PageListViewWindowGet(), &stLvi);

    stLvi.pszText = TEXT("1"); //    line
    stLvi.iSubItem = 3;
    ListView_SetItem(PageListViewWindowGet(), &stLvi);

    if (0 <= iBefore)
    {
        iItem = ListView_GetItemCount(PageListViewWindowGet());
        for (i = iBefore; iItem > i; i++)
        {
            StringCchPrintf(atBuffer, MIN_STRING, TEXT("%u"), i + 1);
            ListView_SetItemText(PageListViewWindowGet(), i, 0, atBuffer);
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 頁リストを再構成する
HRESULT PageListBuild(LPVOID)
{
    INT i, iLastPage;
    LVITEM stLvi;
    TCHAR atBuffer[MIN_STRING];

    stLvi = {};
    stLvi.mask = LVIF_TEXT;

    i = 0;
    for (auto &stPage : DocCurrentFile().vcCont)
    {
        stLvi.iItem = i;
        StringCchPrintf(atBuffer, MIN_STRING, TEXT("%u"), i + 1);
        stLvi.pszText = atBuffer;
        stLvi.iSubItem = 0;
        ListView_InsertItem(PageListViewWindowGet(), &stLvi);

        stLvi.pszText = stPage.atPageName;
        stLvi.iSubItem = 1;
        ListView_SetItem(PageListViewWindowGet(), &stLvi);

        StringCchPrintf(atBuffer, MIN_STRING, TEXT("%d"), stPage.dByteSz);
        stLvi.pszText = atBuffer;
        stLvi.iSubItem = 2;
        ListView_SetItem(PageListViewWindowGet(), &stLvi);

        StringCchPrintf(atBuffer, MIN_STRING, TEXT("%u"), stPage.ltPage.size());
        stLvi.iSubItem = 3;
        ListView_SetItem(PageListViewWindowGet(), &stLvi);

        i++;
    }

    //    見てた頁をリストに露出させる
    iLastPage = DocCurrentFile().dNowPage;
    ListView_EnsureVisible(PageListViewWindowGet(), iLastPage, FALSE); //    対象頁のラインまでスクロールさせちゃう

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 선택한 페이지를 위아래로 이동
static HRESULT PageListSpinning(INT iPage, INT bDir)
{
    const INT iItem = ListView_GetItemCount(PageListViewWindowGet());
    INT iSwapPage;

    if (0 == bDir)
    {
        return E_INVALIDARG;
    }

    if ((0 == iPage) && (0 < bDir))
    {
        return E_ABORT;
    }

    if ((iItem <= (iPage + 1)) && (0 > bDir))
    {
        return E_ABORT;
    }

    iSwapPage = iPage - ((0 < bDir) ? 1 : -1);
    std::swap(DocCurrentFile().vcCont.at(iPage), DocCurrentFile().vcCont.at(iSwapPage));

    PageListViewRewrite(iPage);
    PageListViewRewrite(iSwapPage);

    ListView_SetItemState(PageListViewWindowGet(), iSwapPage, LVIS_SELECTED, LVIS_SELECTED);

    PageListSelectAndSync(iSwapPage, FALSE, FALSE);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 一覧の指定の頁お削除する
HRESULT PageListDelete(INT iPage)
{
    UINT iItem, i;
    TCHAR atBuffer[MIN_STRING];

    ListView_DeleteItem(PageListViewWindowGet(), iPage);
    //    Deleteしたら、リストビューは自動で詰められる

    //    連番振り直し
    iItem = ListView_GetItemCount(PageListViewWindowGet());
    for (i = 0; iItem > i; i++)
    {
        StringCchPrintf(atBuffer, MIN_STRING, TEXT("%u"), i + 1);
        ListView_SetItemText(PageListViewWindowGet(), i, 0, atBuffer);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 各頁の状況・引数はそのうち構造体にしたほうがいいかも
HRESULT PageListInfoSet(INT iPage, INT dByte, INT dLine)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();
    INT iPageCnt;
    TCHAR atBuffer[MIN_STRING];

    iPageCnt = ListView_GetItemCount(PageListViewWindowGet());
    if (iPageCnt <= iPage)
    {
        return E_OUTOFMEMORY;
    }

    // 2024kai ページリストのByteと行のちらつきを削減
    if (dByte != stState.dLastInfoByte || iPage != stState.ixLastInfoPage)
    {
        StringCchPrintf(atBuffer, MIN_STRING, TEXT("%d"), dByte); //    byte
        ListView_SetItemText(PageListViewWindowGet(), iPage, 2, atBuffer);
        stState.dLastInfoByte = dByte;
    }
    if (dLine != stState.dLastInfoLine || iPage != stState.ixLastInfoPage)
    {
        StringCchPrintf(atBuffer, MIN_STRING, TEXT("%d"), dLine); //    line
        ListView_SetItemText(PageListViewWindowGet(), iPage, 3, atBuffer);
        stState.dLastInfoLine = dLine;
    }

    stState.ixLastInfoPage = iPage;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ある頁の情報を書き換えする
HRESULT PageListViewRewrite(INT iPage)
{
    UINT_PTR dLines;
    UINT dBytes;
    INT iPageCount, i;
    TCHAR atBuffer[MIN_STRING];

    iPageCount = ListView_GetItemCount(PageListViewWindowGet());
    if (iPageCount <= iPage)
        return E_OUTOFMEMORY;

    if (0 > iPage) //    再帰で全描画
    {
        for (i = 0; iPageCount > i; i++)
        {
            PageListViewRewrite(i);
        }

        return S_OK;
    }

    StringCchPrintf(atBuffer, MIN_STRING, TEXT("%d"), iPage + 1);
    ListView_SetItemText(PageListViewWindowGet(), iPage, 0, atBuffer);

    ListView_SetItemText(PageListViewWindowGet(), iPage, 1, DocCurrentFile().vcCont.at(iPage).atPageName);

    dBytes = DocCurrentFile().vcCont.at(iPage).dByteSz;
    StringCchPrintf(atBuffer, MIN_STRING, TEXT("%d"), dBytes); //    byte
    ListView_SetItemText(PageListViewWindowGet(), iPage, 2, atBuffer);

    dLines = DocCurrentFile().vcCont.at(iPage).ltPage.size();
    StringCchPrintf(atBuffer, MIN_STRING, TEXT("%d"), dLines); //    line
    ListView_SetItemText(PageListViewWindowGet(), iPage, 3, atBuffer);

    ListView_RedrawItems(PageListViewWindowGet(), iPage, iPage);
    UpdateWindow(PageListViewWindowGet());

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 포커스 페이지 이동
static HRESULT PageListJump(INT iDiff)
{
    if (0 > iDiff)
        return E_OUTOFMEMORY;

    const INT iItem = ListView_GetItemCount(PageListViewWindowGet());
    if (iItem <= iDiff)
        return E_OUTOFMEMORY;

    return PageListSelectAndSync(iDiff, TRUE, TRUE);
}
//-------------------------------------------------------------------------------------------------

// 페이지 이름 변경 대화상자 메시지 핸들러
static INT_PTR CALLBACK PageNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    const INT cdPage = static_cast<INT>(WndTagGet(hDlg));

    switch (message)
    {
    case WM_INITDIALOG:
        WndTagSet(hDlg, lParam); //    編集する頁番号
        Edit_SetText(GetDlgItem(hDlg, IDE_PAGENAME), DocCurrentFile().vcCont.at(static_cast<INT>(lParam)).atPageName);
        SetFocus(GetDlgItem(hDlg, IDE_PAGENAME));
        return (INT_PTR)FALSE;

    case WM_COMMAND:
        if (IDOK == LOWORD(wParam))
        {
            Edit_GetText(GetDlgItem(hDlg, IDE_PAGENAME), DocCurrentFile().vcCont.at(cdPage).atPageName, SUB_STRING);
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        if (IDCANCEL == LOWORD(wParam))
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }

        break;
    }
    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

static HRESULT PageListNameChange(INT dPage)
{
    const PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();
    INT_PTR iRslt;

    iRslt = DialogBoxParam(stState.hInstance, MAKEINTRESOURCE(IDD_PAGE_NAME_DLG), stState.hPageWindow, PageNameDlgProc, dPage);
    if (IDOK == iRslt)
    {
        PageListNameSet(dPage, DocCurrentFile().vcCont.at(dPage).atPageName);
        return S_OK;
    }

    return E_ABORT;
}
//-------------------------------------------------------------------------------------------------

// 頁名前をセット
HRESULT PageListNameSet(INT dPage, LPTSTR ptName)
{
    INT iPageCount;
    LVITEM stLvi;

    iPageCount = ListView_GetItemCount(PageListViewWindowGet());
    if (iPageCount <= dPage)
        return E_OUTOFMEMORY;

    stLvi = {};
    stLvi.mask = LVIF_TEXT;
    stLvi.iItem = dPage;
    stLvi.pszText = ptName;
    stLvi.iSubItem = 1; //    名前
    ListView_SetItem(PageListViewWindowGet(), &stLvi);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 名前の付いている頁があるか
INT PageListIsNamed(FILES_ITR itFile)
{
    return std::any_of(itFile->vcCont.begin(), itFile->vcCont.end(), [](const ONEPAGE &stPage)
                       { return 0 != stPage.atPageName[0]; })
               ? TRUE
               : FALSE;
}
//-------------------------------------------------------------------------------------------------

// フォーカス頁の内容を、次の頁を作ってコピーする
static HRESULT PageListDuplicate(INT iNowPage)
{
    INT iNewPage;

    iNewPage = DocPageCreate(iNowPage); //    新頁
    PageListInsert(iNewPage);           //    ページリストビューに追加

    DocCurrentFile().vcCont.at(iNewPage).ltPage = DocCurrentFile().vcCont.at(iNowPage).ltPage;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// アイテムリストビューサブクラス
static LRESULT CALLBACK gpfPageViewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateResolve(hWnd);

    switch (msg)
    {
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, Plv_OnMouseMove);
        HANDLE_MSG(hWnd, WM_COMMAND, Plt_OnCommand);

#ifdef USE_HOVERTIP
    case WM_MOUSEHOVER:
        HoverTipOnMouseHover(hWnd, wParam, lParam, PageListHoverTipInfo);
        return 0;

    case WM_MOUSELEAVE:
        HoverTipOnMouseLeave(hWnd);
        stState.ixMouseSelection = -1;
        return 0;
#endif
    }

    return CallWindowProc(stState.pfOrigPageViewProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// マウスが動いたときの処理
static VOID Plv_OnMouseMove(HWND hWnd, INT, INT y, UINT)
{
    PAGE_LIST_CONTROLLER_STATE &stState = PageListStateResolve(hWnd);
    LVHITTESTINFO stHitInfo;
    INT iItem;
    BOOLEAN bReDraw = FALSE;

    stHitInfo = {};
    stHitInfo.pt.x = 10; //    高さが重要なのでここは適当でいい
    stHitInfo.pt.y = y;

    iItem = ListView_HitTest(hWnd, &stHitInfo);
    if (stState.ixMouseSelection != iItem)
        bReDraw = TRUE;
    stState.ixMouseSelection = iItem;

#ifdef USE_HOVERTIP
    if (bReDraw)
    {
        HoverTipResist(stState.hPageListWindow);
    }
#endif

    return;
}

#ifdef USE_HOVERTIP
// HoverTip用のコールバック受取
static LPTSTR CALLBACK PageListHoverTipInfo(LPVOID)
{
    const PAGE_LIST_CONTROLLER_STATE &stState = PageListStateGet();
    LPTSTR ptBuffer = nullptr;

    if (!(stState.bPageTooltipView))
    {
        return nullptr;
    } //    非表示なら何もしないでおｋ
    if (0 > stState.ixMouseSelection)
    {
        return nullptr;
    }

    PageListPreviewTextGetAlloc(stState.ixMouseSelection, &ptBuffer);

    return ptBuffer;
}
//-------------------------------------------------------------------------------------------------
#endif
