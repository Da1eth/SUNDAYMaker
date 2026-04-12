#include "Palette.h"
#include "AppLayoutInternal.h"
#include "AppUiContextInternal.h"

#define INSERT_PALETTE_CLASS TEXT("INSERT_PALETTE")
#define LT_WIDTH 240
#define LT_HEIGHT 240

#define PI_CLICK_NEW

extern HFONT ghAaFont;   //    AA用フォント
extern HFONT ghNameFont; //    ファイルタブ用フォント

extern INT gbTmpltDock;        //    頁・壱行テンプレのドッキング
extern BOOLEAN gbDockTmplView; //    くっついてるテンプレは見えているか

static HWND ghPalInsertHostWnd; // このウインドウハンドル

static HWND ghCtgryBxWnd;        // カテゴリコンボックス
static HWND ghLvItemWnd;         // アイテム一覧リストビュー
static HWND ghPalInsertLvTipWnd; // 팔레트 리스트 툴팁

static HWND ghDockTabWnd; // ドッキングしたときの選択肢タブ

static UINT gNowGroup; // 今みてるグループ番号

static UINT gLnClmCnt; // 表示カラム数

static WNDPROC gpfOrigPalInsertCtgryProc;
static WNDPROC gpfOrigPalInsertItemProc;

static vector<JSON_TEMPLATE_GROUP> gvcPalInsertTmpls; // JSON 템플릿 그룹

LRESULT CALLBACK InsertPaletteProc(HWND, UINT, WPARAM, LPARAM);
VOID InsertPaletteOnCommand(HWND, INT, HWND, UINT);
VOID InsertPaletteOnSize(HWND, UINT, INT, INT);
#ifndef PI_CLICK_NEW
LRESULT InsertPaletteOnNotify(HWND, INT, LPNMHDR);
#endif

HRESULT PaletteCommonItemListOn(UINT);

LRESULT CALLBACK gpfInsertPaletteCtgryProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK gpfInsertPaletteItemProc(HWND, UINT, WPARAM, LPARAM);
LRESULT InsertPaletteListOnNotify(HWND, INT, LPNMHDR);
#ifdef PI_CLICK_NEW
VOID PaletteCommonListOnMouseButtonUp(HWND, UINT, INT, INT, UINT);
#endif

HWND DockingTabCreate(HINSTANCE, HWND, const RECT *);

// 삽입 팔레트 윈도우 생성
HWND PaletteCommonInitialise(HINSTANCE hInstance, HWND hParentWnd, LPRECT pstFrame)
{

    WNDCLASSEX wcex;
    RECT wdRect, clRect, rect, cbxRect;
    UINT_PTR dItems, i;
    DWORD dwExStyle, dwStyle;
    HWND hPrWnd;

    TTTOOLINFO stToolInfo;
    LVCOLUMN stLvColm;

    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = InsertPaletteProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = INSERT_PALETTE_CLASS;
    wcex.hIconSm = nullptr;

    RegisterClassEx(&wcex);

    // 템플릿 데이터를 JSON 그룹 그대로 읽는다.
    PaletteListGroupsLoad(FALSE, &gvcPalInsertTmpls);

    InitWindowPos(INIT_LOAD, WDP_LNTMPL, &rect);
    if (0 == rect.right || 0 == rect.bottom) //    幅高さが０はデータ無し
    {
        GetWindowRect(hParentWnd, &wdRect);
        rect.left = wdRect.right;
        rect.top = wdRect.top;
        rect.right = LT_WIDTH;
        rect.bottom = LT_HEIGHT;
        InitWindowPos(INIT_SAVE, WDP_LNTMPL, &rect); // 起動時保存
    }

    //    カラム数確認
    gLnClmCnt = InitParamValue(INIT_LOAD, VL_LINETMP_CLM, 4);

    if (gbTmpltDock)
    {
        APP_DOCKED_WINDOW_LAYOUT stDockedLayout;

        hPrWnd = hParentWnd;
        dwExStyle = 0;
        dwStyle = WS_CHILD | WS_VISIBLE;

        //    ブラシと切換タブを作成
        rect = AppDockLayoutCalc(pstFrame, AppViewContextGet().dSplitPos).stDockRect;
        rect.top += (rect.bottom >> 1);
        rect.bottom = 10;
        ghDockTabWnd = DockingTabCreate(hInstance, hPrWnd, &rect);

        stDockedLayout = AppLayoutDockedWindowsCalc(
            pstFrame,
            AppViewContextGet().dSplitPos,
            AppLayoutDockTabHeightGet(ghDockTabWnd),
            gbDockTmplView);
        rect = stDockedLayout.stPaletteRect;
    }
    else
    {
        //    常に最全面に表示を？
        dwExStyle = WS_EX_TOOLWINDOW;
        if (InitWindowTopMost(INIT_LOAD, WDP_LNTMPL, 0))
        {
            dwExStyle |= WS_EX_TOPMOST;
        }
        dwStyle = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
        hPrWnd = nullptr;
    }

    //    ウインドウ作成
    ghPalInsertHostWnd = CreateWindowEx(dwExStyle, INSERT_PALETTE_CLASS, TEXT("팔레트"),
                                        dwStyle, rect.left, rect.top, rect.right, rect.bottom, hPrWnd, nullptr, hInstance, nullptr);

    GetClientRect(ghPalInsertHostWnd, &clRect);

    //    カテゴリコンボックス
    ghCtgryBxWnd = CreateWindowEx(0, WC_COMBOBOX, TEXT("category"),
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST,
                                  0, 0, clRect.right, 127, ghPalInsertHostWnd,
                                  (HMENU)IDCB_LT_CATEGORY, hInstance, nullptr);
    SetWindowFont(ghCtgryBxWnd, AppUiDropDownFontGet(), FALSE);

    gpfOrigPalInsertCtgryProc = SubclassWindow(ghCtgryBxWnd, gpfInsertPaletteCtgryProc);

    dItems = gvcPalInsertTmpls.size();
    PaletteListComboReload(ghCtgryBxWnd, gvcPalInsertTmpls);
    if (0 < dItems)
        ComboBox_SetCurSel(ghCtgryBxWnd, 0);
    gNowGroup = 0;

    GetClientRect(ghCtgryBxWnd, &cbxRect);

    //    アイテム一覧リストビュー
    ghLvItemWnd = CreateWindowEx(0, WC_LISTVIEW, TEXT("lineitem"),
                                 WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LVS_REPORT | LVS_NOSORTHEADER | LVS_NOCOLUMNHEADER,
                                 0, cbxRect.bottom, clRect.right, clRect.bottom - cbxRect.bottom,
                                 ghPalInsertHostWnd, (HMENU)IDLV_LT_ITEMVIEW, hInstance, nullptr);
    ListView_SetExtendedListViewStyle(ghLvItemWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP | LVS_EX_ONECLICKACTIVATE);

    SetWindowFont(ghLvItemWnd, ghAaFont, TRUE);

    gpfOrigPalInsertItemProc = SubclassWindow(ghLvItemWnd, gpfInsertPaletteItemProc); //    サブクラス

    ZeroMemory(&stLvColm, sizeof(LVCOLUMN));
    stLvColm.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    stLvColm.fmt = LVCFMT_LEFT;
    stLvColm.pszText = const_cast<LPTSTR>(TEXT("Item"));
    stLvColm.cx = 10; //    後で合わせるので適当で良い
    for (i = 0; gLnClmCnt > i; i++)
    {
        stLvColm.iSubItem = i;
        ListView_InsertColumn(ghLvItemWnd, i, &stLvColm);
    }

    if (0 < dItems)
        PaletteCommonItemListOn(0); //    中身追加

    //    リストビューツールチップ
    ghPalInsertLvTipWnd = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
                                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, ghPalInsertHostWnd, nullptr, hInstance, nullptr);
    AppUiFontApply(ghPalInsertLvTipWnd, FALSE);

    //    ツールチップをコールバックで割り付け
    ZeroMemory(&stToolInfo, sizeof(TTTOOLINFO));
    stToolInfo.cbSize = sizeof(TTTOOLINFO);
    stToolInfo.uFlags = TTF_SUBCLASS;
    stToolInfo.hinst = nullptr; //
    stToolInfo.hwnd = ghLvItemWnd;
    stToolInfo.uId = IDLV_LT_ITEMVIEW;
    GetClientRect(ghLvItemWnd, &stToolInfo.rect);
    stToolInfo.lpszText = LPSTR_TEXTCALLBACK; //    コレを指定するとコールバックになる
    SendMessage(ghPalInsertLvTipWnd, TTM_ADDTOOL, 0, (LPARAM)&stToolInfo);
    SendMessage(ghPalInsertLvTipWnd, TTM_SETMAXTIPWIDTH, 0, 0); //    チップの幅。０設定でいい。これしとかないと改行されない

    ShowWindow(ghPalInsertHostWnd, SW_SHOW);
    UpdateWindow(ghPalInsertHostWnd);

    return ghPalInsertHostWnd;
}
//-------------------------------------------------------------------------------------------------

// ドッキング状態で発生・ドッキングしてる内容切換タブ
HWND DockingTabCreate(HINSTANCE hInst, HWND hPrWnd, const RECT *pstRect)
{
    HWND hWorkWnd;
    RECT itRect;
    TCITEM stTcItem;

    hWorkWnd = CreateWindowEx(0, WC_TABCONTROL, TEXT("dockseltab"),
                              WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | TCS_TABS | TCS_SINGLELINE,
                              pstRect->left, pstRect->top, pstRect->right, pstRect->bottom, hPrWnd, (HMENU)IDTB_DOCK_TAB, hInst, nullptr); //    TCS_SINGLELINE
    SetWindowFont(hWorkWnd, ghNameFont, FALSE);

    stTcItem = {};
    stTcItem.mask = TCIF_TEXT;
    stTcItem.pszText = const_cast<LPTSTR>(TEXT("팔레트"));
    TabCtrl_InsertItem(hWorkWnd, 0, &stTcItem);
    stTcItem.pszText = const_cast<LPTSTR>(TEXT("채우기"));
    TabCtrl_InsertItem(hWorkWnd, 1, &stTcItem);

    //    選ばれしファイルをタブ的に追加？　タブ幅はウインドウ幅

    TabCtrl_GetItemRect(hWorkWnd, 1, &itRect);
    itRect.bottom += itRect.top;
    MoveWindow(hWorkWnd, pstRect->left, pstRect->top, pstRect->right, itRect.bottom, TRUE);

    return hWorkWnd;
}
//-------------------------------------------------------------------------------------------------

VOID DockingTabSizeGet(LPRECT pstRect)
{
    ZeroMemory(pstRect, sizeof(RECT));

    if (ghDockTabWnd)
    {
        GetWindowRect(ghDockTabWnd, pstRect);
        pstRect->right -= pstRect->left;
        pstRect->bottom -= pstRect->top;
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// 壱行ブラシタブのコンテキストメニューか？
HRESULT DockingTabContextMenu(HWND hWnd, HWND hWndContext, LONG xPos, LONG yPos)
{
    HMENU hPopupMenu = nullptr;

    //    関係ないなら何もしない
    if (hWndContext != ghDockTabWnd)
    {
        return E_ABORT;
    }

    hPopupMenu = CreatePopupMenu();

    if (gbDockTmplView)
        AppendMenu(hPopupMenu, MF_STRING, IDM_LINE_BRUSH_TMPL_VIEW, TEXT("팔레트 비표시"));
    else
        AppendMenu(hPopupMenu, MF_STRING, IDM_LINE_BRUSH_TMPL_VIEW, TEXT("팔레트 표시"));

    TrackPopupMenu(hPopupMenu, 0, xPos, yPos, 0, hWnd, nullptr);
    DestroyMenu(hPopupMenu);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// DOCKINGテンプレ選択タブのハンドル確保
HWND DockingTabGet(VOID)
{
    if (gbTmpltDock)
        return ghDockTabWnd;

    return nullptr;
}
//-------------------------------------------------------------------------------------------------

// フローティング壱行テンプレの位置リセット
HRESULT PaletteCommonPositionReset(HWND hMainWnd)
{
    RECT wdRect, rect;

    GetWindowRect(hMainWnd, &wdRect);
    rect.left = wdRect.right;
    rect.top = wdRect.top;
    rect.right = LT_WIDTH;
    rect.bottom = LT_HEIGHT;

    SetWindowPos(ghPalInsertHostWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_SHOWWINDOW | SWP_NOZORDER);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ドッキング状態で発生・くっついてるウインドウがリサイズされたら
VOID PaletteCommonResize(HWND hPrntWnd, LPRECT pstFrame)
{
    const APP_DOCKED_WINDOW_LAYOUT stDockedLayout = AppLayoutDockedWindowsCalc(
        pstFrame,
        AppViewContextGet().dSplitPos,
        AppLayoutDockTabHeightGet(ghDockTabWnd),
        gbDockTmplView);

    UNREFERENCED_PARAMETER(hPrntWnd);

    AppLayoutApplyDockedWindows(stDockedLayout, nullptr, ghDockTabWnd, ghPalInsertHostWnd, nullptr);

    return;
}
//-------------------------------------------------------------------------------------------------

// ウインドウプロシージャ
LRESULT CALLBACK InsertPaletteProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_SIZE, InsertPaletteOnSize);
        HANDLE_MSG(hWnd, WM_COMMAND, InsertPaletteOnCommand);
#ifndef PI_CLICK_NEW
    HANDLE_MSG(hWnd, WM_NOTIFY, InsertPaletteOnNotify); //    コモンコントロールの個別イベント
#endif

    // WM_MOUSEWHEEL 전달은 특정 조합에서 충돌 이력이 있어 비활성 상태로 유지한다.

    case WM_CONTEXTMENU:
        return 0;
    case WM_CLOSE:
        ShowWindow(ghPalInsertHostWnd, SW_HIDE);
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// COMMANDメッセージの受け取り。ボタン押されたとかで発生
VOID InsertPaletteOnCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    INT rslt;
    UINT dClm;
    LONG_PTR rdExStyle;

    switch (id)
    {
    case IDCB_LT_CATEGORY:
        if (CBN_SELCHANGE == codeNotify)
        {
            rslt = ComboBox_GetCurSel(ghCtgryBxWnd);
            gNowGroup = rslt;

            PaletteCommonItemListOn(rslt);
        }
        break;

    case IDM_WINDOW_CHANGE:
        WindowFocusChange(WND_LINE, 1);
        break;
    case IDM_WINDOW_CHG_RVRS:
        WindowFocusChange(WND_LINE, -1);
        break;

    case IDM_TMPL_GRID_INCREASE:
    case IDM_TMPL_GRID_DECREASE:
        dClm = PaletteListGridFluctuate(ghLvItemWnd, ((IDM_TMPL_GRID_INCREASE == id) ? 1 : -1));
        if (dClm)
        {
            gLnClmCnt = dClm;
            PaletteCommonItemListOn(gNowGroup);
            InitParamValue(INIT_SAVE, VL_LINETMP_CLM, gLnClmCnt);
        }
        break;

    case IDM_TMPLT_GROUP_PREV:
        if (0 < gNowGroup)
        {
            gNowGroup--;
            ComboBox_SetCurSel(ghCtgryBxWnd, gNowGroup);
            PaletteCommonItemListOn(gNowGroup);
        }
        break;

    case IDM_TMPLT_GROUP_NEXT:
        if ((gNowGroup + 1) < gvcPalInsertTmpls.size())
        {
            gNowGroup++;
            ComboBox_SetCurSel(ghCtgryBxWnd, gNowGroup);
            PaletteCommonItemListOn(gNowGroup);
        }
        break;

    case IDM_TOPMOST_TOGGLE: //    常時最全面と通常ウインドウのトグル
        rdExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        if (WS_EX_TOPMOST & rdExStyle)
        {
            SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            InitWindowTopMost(INIT_SAVE, WDP_LNTMPL, 0);
        }
        else
        {
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            InitWindowTopMost(INIT_SAVE, WDP_LNTMPL, 1);
        }
        break;

    case IDM_TMPLGROUPSTYLE_TGL:
        break;

    default:
        break;
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// サイズ変更された
VOID InsertPaletteOnSize(HWND hWnd, UINT state, INT cx, INT cy)
{
    RECT cbxRect;

    MoveWindow(ghCtgryBxWnd, 0, 0, cx, 127, TRUE);
    GetClientRect(ghCtgryBxWnd, &cbxRect);

    MoveWindow(ghLvItemWnd, 0, cbxRect.bottom, cx, cy - cbxRect.bottom, TRUE);

    PaletteListResizeColumns(ghLvItemWnd, gLnClmCnt);

    return;
}
//-------------------------------------------------------------------------------------------------

#ifndef PI_CLICK_NEW
// ノーティファイメッセージの処理
LRESULT InsertPaletteOnNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    HWND hLvWnd;
    INT iPos, iItem, nmCode, iSubItem;
    INT_PTR items;
    TCHAR atItem[SUB_STRING];
    LPNMLISTVIEW pstLv;
    LVHITTESTINFO stHitTestInfo;

    if (IDLV_LT_ITEMVIEW == idFrom)
    {
        pstLv = (LPNMLISTVIEW)pstNmhdr;

        hLvWnd = pstLv->hdr.hwndFrom;
        nmCode = pstLv->hdr.code;

        //    普通のクルックについて
        if (NM_CLICK == nmCode)
        {
            stHitTestInfo.pt = pstLv->ptAction;
            ListView_SubItemHitTest(hLvWnd, &stHitTestInfo);

            iItem = stHitTestInfo.iItem;
            iSubItem = stHitTestInfo.iSubItem;
            iPos = iItem * gLnClmCnt + iSubItem;

            if (0 < gvcPalInsertTmpls.size())
            {
                items = gvcPalInsertTmpls.at(gNowGroup).vcItems.size();

                TRACE(TEXT("LINE TMPL[%d x %d]"), iItem, iSubItem);

                if (0 <= iPos && iPos < items) //    なんか選択した
                {
                    StringCchCopy(atItem, SUB_STRING, gvcPalInsertTmpls.at(gNowGroup).vcItems.at(iPos).c_str());
                    ViewInsertTmpleString(atItem); //    挿入処理

                    ViewFocusSet(); //    20110720    フォーカスを描画に戻す
                }
            }
            else
            {
                ViewFocusSet(); //    20110720    フォーカスを描画に戻す
            }
        }
    }

    return 0; //    何もないなら０を戻す
}
//-------------------------------------------------------------------------------------------------
#endif

// アイテムをコンボックスとリストに展開
HRESULT PaletteCommonItemListOn(UINT listNum)
{
    if (gvcPalInsertTmpls.empty() || listNum >= gvcPalInsertTmpls.size())
    {
        return E_OUTOFMEMORY;
    }

    const vector<wstring> &vcItems = gvcPalInsertTmpls.at(listNum).vcItems;

    TRACE(TEXT("LINE open NUM[%u] ITEM[%u] GRID[%d]"), listNum, (UINT)vcItems.size(), gLnClmCnt);

    return PaletteListPopulate(ghLvItemWnd, vcItems, gLnClmCnt);
}
//-------------------------------------------------------------------------------------------------

// カテゴリコンボックスサブクラス
LRESULT CALLBACK gpfInsertPaletteCtgryProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT id;

    switch (msg)
    {
    case WM_CONTEXTMENU:
        return 0;

    case WM_COMMAND:
        id = LOWORD(wParam);
        switch (id)
        {
        case IDM_WINDOW_CHANGE:
        case IDM_WINDOW_CHG_RVRS:
        case IDM_TMPL_GRID_INCREASE:
        case IDM_TMPL_GRID_DECREASE:
        case IDM_TMPLT_GROUP_PREV:
        case IDM_TMPLT_GROUP_NEXT:
            SendMessage(ghPalInsertHostWnd, WM_COMMAND, wParam, lParam);
            return 0;
        }
        break;

    case WM_MOUSEWHEEL: //    カテゴリコンボックスでのWHEELでページ送りなると面倒なのでリストビューに送る
        SendMessage(ghLvItemWnd, WM_MOUSEWHEEL, wParam, lParam);
        return 0;
    }

    return CallWindowProc(gpfOrigPalInsertCtgryProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// アイテムリストビューサブクラス
LRESULT CALLBACK gpfInsertPaletteItemProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT id;

    //    Ctrl押しながらマウスホイール廻るとまずい？

    switch (msg)
    {
        HANDLE_MSG(hWnd, WM_NOTIFY, InsertPaletteListOnNotify); //    コモンコントロールの個別イベント

    case WM_CONTEXTMENU:
        return 0;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
        PaletteListClearSelection(hWnd);
        return 0;

#ifdef PI_CLICK_NEW
    case WM_LBUTTONDOWN: //    この部分がないとクルックに反応しない
    case WM_MBUTTONDOWN:
        TRACE(TEXT("LTL_MOUSenAN"));
        return 0;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
        PaletteCommonListOnMouseButtonUp(hWnd, msg, (INT)(SHORT)LOWORD(lParam), (INT)(SHORT)HIWORD(lParam), (UINT)(wParam));
        return 0;
#endif
    case WM_COMMAND:
        id = LOWORD(wParam);
        switch (id)
        {
        case IDM_WINDOW_CHANGE:
        case IDM_WINDOW_CHG_RVRS:
        case IDM_TMPL_GRID_INCREASE:
        case IDM_TMPL_GRID_DECREASE:
        case IDM_TMPLT_GROUP_PREV:
        case IDM_TMPLT_GROUP_NEXT:
            SendMessage(ghPalInsertHostWnd, WM_COMMAND, wParam, lParam);
            return 0;
        }
        break;
    }

    return CallWindowProc(gpfOrigPalInsertItemProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

#ifdef PI_CLICK_NEW
// ビューでマウスのボタンがうっｐされたとき
VOID PaletteCommonListOnMouseButtonUp(HWND hWnd, UINT msg, INT x, INT y, UINT keyFlags)
{
    PALETTE_LIST_HIT stHit;
    LPCTSTR ptItem;

    TRACE(TEXT("LTL_MOUSEB %d x %d"), x, y);

    if (!(PaletteListHitTest(hWnd, gLnClmCnt, POINT{x, y}, &stHit)))
    {
        ViewFocusSet();
        return;
    }

    TRACE(TEXT("LINE TMPL[%d x %d][%d]"), stHit.iItem, stHit.iSubItem, stHit.iIndex);

    ptItem = PaletteListGroupItemGet(gvcPalInsertTmpls, gNowGroup, stHit.iIndex);
    if (!(ptItem))
    {
        ViewFocusSet();
        return;
    }

    if (WM_LBUTTONUP == msg)
    {
        ViewInsertTmpleString(ptItem);
        ViewFocusSet();
    }
    else if (WM_MBUTTONUP == msg)
    {
        LayerBoxVisibalise(GetModuleHandle(nullptr), ptItem, 0x00);
    }

    return;
}
//-------------------------------------------------------------------------------------------------
#endif

// ノーティファイメッセージの処理
LRESULT InsertPaletteListOnNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    HWND hLvWnd;
    INT nmCode;
    LPNMLISTVIEW pstLv;
    LPNMTTDISPINFO pstDispInfo;

    pstLv = (LPNMLISTVIEW)pstNmhdr;

    //    リストビュー自体のプロシージャなので
    hLvWnd = hWnd; // pstLv->hdr.hwndFrom;<--ツールチップのハンドルになってるかもだ
    nmCode = pstLv->hdr.code;

    if (TTN_GETDISPINFO == nmCode)
    {
        TRACE(TEXT("LT_TOOL %u"), idFrom);
        if (IDLV_LT_ITEMVIEW == idFrom)
        {
            pstDispInfo = (LPNMTTDISPINFO)pstNmhdr;

            ZeroMemory(&(pstDispInfo->szText), sizeof(pstDispInfo->szText));
            pstDispInfo->lpszText = pstDispInfo->szText;

            PaletteListBuildTooltipText(hLvWnd, gvcPalInsertTmpls, gNowGroup, gLnClmCnt, pstDispInfo->szText, 80);

            return 0;
        }
    }

    return CallWindowProc(gpfOrigPalInsertItemProc, hWnd, WM_NOTIFY, (WPARAM)idFrom, (LPARAM)pstNmhdr);
}
//-------------------------------------------------------------------------------------------------
