#include "Palette.h"
#include "AppLayoutInternal.h"
#include "AppUiContextInternal.h"
#include "UiText.h"

#define INSERT_BUCKET_CLASS TEXT("INSERT_BUCKET")
#define BT_WIDTH 240
#define BT_HEIGHT 240

#define PB_COMBO_DROPDOWN_HEIGHT 127
#define BTV_R_MARGIN 18

#define TB_ITEMS 1
static TBBUTTON gstBrTBInfo[] = {
    {0, IDM_BRUSH_ON_OFF, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0}
};

extern HFONT ghAaFont; //    AA用フォント

extern INT gbTmpltDock;        //    テンプレのドッキング
extern BOOLEAN gbDockTmplView; //    くっついてるテンプレは見えているか

static HIMAGELIST ghBrushImgLst;

static UINT gbPalBucketMode; // 非零ブラシモード
static HWND ghPalBucketHostWnd;    // 채우기 팔레트 본체
static HWND ghPalBucketToolBarWnd; // 툴바
static HWND ghCtgryBxWnd;          // カテゴリコンボックス
static HWND ghLvItemWnd;           // 内容リストビュー
static HWND ghPalBucketLvTipWnd;   // 팔레트 버킷 리스트 툴팁

static UINT gNowGroup; // カテゴリ

static WNDPROC gpfOrigPalBucketCtgryProc;
static WNDPROC gpfOrigPalBucketItemProc;

static UINT gBrhClmCnt; // 表示カラム数

static WNDPROC gpfOrigTBProc;

static vector<JSON_TEMPLATE_GROUP> gvcPalBucketTmpls; // JSON 템플릿 그룹
static TCHAR gatCurrentBucketPattern[SUB_STRING]; // 현재 사용 중인 버킷 문자열

LRESULT CALLBACK InsertBucketProc(HWND, UINT, WPARAM, LPARAM);
VOID InsertBucketOnCommand(HWND, INT, HWND, UINT);
VOID InsertBucketOnSize(HWND, UINT, INT, INT);
LRESULT InsertBucketOnNotify(HWND, INT, LPNMHDR);

UINT InsertBucketItemListOn(UINT);

LRESULT CALLBACK gpfInsertBucketCtgryProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK gpfInsertBucketItemProc(HWND, UINT, WPARAM, LPARAM);
LRESULT InsertBucketListOnNotify(HWND, INT, LPNMHDR);
static VOID PaletteBucketListOnMouseButtonUp(HWND, INT, INT, UINT);

static LRESULT CALLBACK gpfToolbarProc(HWND, UINT, WPARAM, LPARAM);
static VOID InsertBucketCurrentPatternSet(LPCTSTR);
static BOOLEAN InsertBucketCellMatches(INT);
static VOID InsertBucketInvalidateSelection(VOID);
static BOOLEAN InsertBucketCellRectGet(INT, INT, LPRECT);
static VOID InsertBucketDrawCellBorder(HDC, INT);
//-------------------------------------------------------------------------------------------------

// 삽입 버킷 윈도우 생성
HWND PaletteBucketInitialise(HINSTANCE hInstance, HWND hParentWnd, LPRECT pstFrame, HWND hDockWnd)
{
    DWORD dwExStyle, dwStyle;
    HWND hPrWnd;
    UINT_PTR dItems, i;
    TCHAR atBuffer[MAX_STRING];

    HBITMAP hImg, hMsq;

    WNDCLASSEX wcex;
    RECT wdRect, clRect, rect, tbRect;
    LVCOLUMN stLvColm;

    TTTOOLINFO stToolInfo;

    //    破壊
    if (!(hInstance) && !(hParentWnd))
    {
        ImageList_Destroy(ghBrushImgLst);
        return nullptr;
    }

    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = InsertBucketProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = INSERT_BUCKET_CLASS;
    wcex.hIconSm = nullptr;

    RegisterClassEx(&wcex);

    gbPalBucketMode = FALSE;
    ZeroMemory(gatCurrentBucketPattern, sizeof(gatCurrentBucketPattern));

    // 템플릿 데이터를 JSON 그룹 그대로 읽는다.
    PaletteListGroupsLoad(TRUE, &gvcPalBucketTmpls);

    InitWindowPos(INIT_LOAD, WDP_BRTMPL, &rect);
    if (0 == rect.right || 0 == rect.bottom) //    幅高さが０はデータ無し
    {
        GetWindowRect(hParentWnd, &wdRect);
        rect.left = wdRect.right + 32;
        rect.top = wdRect.top + 32;
        rect.right = BT_WIDTH;
        rect.bottom = BT_HEIGHT;
        InitWindowPos(INIT_SAVE, WDP_BRTMPL, &rect); // 起動時保存
    }

    //    カラム数確認
    gBrhClmCnt = InitParamValue(INIT_LOAD, VL_BRUSHTMP_CLM, 4);

    if (gbTmpltDock)
    {
        const APP_DOCKED_WINDOW_LAYOUT stDockedLayout = AppLayoutDockedWindowsCalc(
            pstFrame,
            AppViewContextGet().dSplitPos,
            AppLayoutDockTabHeightGet(DockingTabGet()),
            gbDockTmplView);

        hPrWnd = hParentWnd;
        dwExStyle = 0;
        dwStyle = WS_CHILD;

        rect = stDockedLayout.stBucketRect; //    クライヤントに使える領域
    }
    else
    {
        hPrWnd = nullptr;
        //    常に最全面に表示を？
        dwExStyle = WS_EX_TOOLWINDOW;
        if (InitWindowTopMost(INIT_LOAD, WDP_BRTMPL, 0))
        {
            dwExStyle |= WS_EX_TOPMOST;
        }
        dwStyle = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
    }

    //    本体
    ghPalBucketHostWnd = CreateWindowEx(dwExStyle, INSERT_BUCKET_CLASS, TEXT("채우기"),
                                        dwStyle, rect.left, rect.top, rect.right, rect.bottom, hPrWnd, nullptr, hInstance, nullptr);

    //    ツールバー
    ghPalBucketToolBarWnd = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("brtoolbar"), WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NORESIZE | CCS_NOPARENTALIGN, 0, 0, 0, 0, ghPalBucketHostWnd, (HMENU)IDW_BRUSH_TOOL_BAR, hInstance, nullptr);
    AppUiFontApply(ghPalBucketToolBarWnd, FALSE);

    //    自動ツールチップスタイルを追加
    SendMessage(ghPalBucketToolBarWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    ghBrushImgLst = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);
    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMP_BRUSH_MODE)));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMQ_BRUSH_MODE)));
    ImageList_Add(ghBrushImgLst, hImg, hMsq); //    イメージリストにイメージを追加
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);
    SendMessage(ghPalBucketToolBarWnd, TB_SETIMAGELIST, 0, (LPARAM)ghBrushImgLst);

    SendMessage(ghPalBucketToolBarWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    //    ツールチップ文字列を設定・ボタンテキストがツールチップになる
    StringCchCopy(atBuffer, MAX_STRING, UiTextGetLabel(IDM_BRUSH_ON_OFF));
    gstBrTBInfo[0].iString = SendMessage(ghPalBucketToolBarWnd, TB_ADDSTRING, 0, (LPARAM)atBuffer);

    SendMessage(ghPalBucketToolBarWnd, TB_ADDBUTTONS, (WPARAM)TB_ITEMS, (LPARAM)&gstBrTBInfo); //    ツールバーにボタンを挿入

    SendMessage(ghPalBucketToolBarWnd, TB_AUTOSIZE, 0, 0); //    ボタンのサイズに合わせてツールバーをリサイズ
    InvalidateRect(ghPalBucketToolBarWnd, nullptr, TRUE);  //    クライアント全体を再描画する命令

    //    ツールバーサブクラス化
    gpfOrigTBProc = SubclassWindow(ghPalBucketToolBarWnd, gpfToolbarProc);

    GetClientRect(ghPalBucketToolBarWnd, &tbRect);

    GetClientRect(ghPalBucketHostWnd, &clRect);

    //    カテゴリコンボックス
    ghCtgryBxWnd = CreateWindowEx(0, WC_COMBOBOX, TEXT("BrCategory"),
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST,
                                  0, 0, clRect.right, PB_COMBO_DROPDOWN_HEIGHT, ghPalBucketHostWnd,
                                  (HMENU)IDCB_BT_CATEGORY, hInstance, nullptr);
    SetWindowFont(ghCtgryBxWnd, AppUiDropDownFontGet(), FALSE);

    gpfOrigPalBucketCtgryProc = SubclassWindow(ghCtgryBxWnd, gpfInsertBucketCtgryProc);

    dItems = gvcPalBucketTmpls.size();
    PaletteListComboReload(ghCtgryBxWnd, gvcPalBucketTmpls);
    if (0 < dItems)
        ComboBox_SetCurSel(ghCtgryBxWnd, 0);
    gNowGroup = 0;

    //    一覧リストビュー
    ghLvItemWnd = CreateWindowEx(0, WC_LISTVIEW, TEXT("brushitem"),
                                 WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LVS_REPORT | LVS_NOSORTHEADER | LVS_NOCOLUMNHEADER | LVS_SINGLESEL,
                                 0, 0, clRect.right, clRect.bottom,
                                 ghPalBucketHostWnd, (HMENU)IDLV_BT_ITEMVIEW, hInstance, nullptr);
    ListView_SetExtendedListViewStyle(ghLvItemWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SetWindowFont(ghLvItemWnd, ghAaFont, TRUE);

    gpfOrigPalBucketItemProc = SubclassWindow(ghLvItemWnd, gpfInsertBucketItemProc);

    ZeroMemory(&stLvColm, sizeof(LVCOLUMN));
    stLvColm.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    stLvColm.fmt = LVCFMT_LEFT;
    stLvColm.pszText = const_cast<LPTSTR>(TEXT("Brush"));
    stLvColm.cx = 10; //    後で合わせるので適当で良い
    stLvColm.iSubItem = 0;

    for (i = 0; gBrhClmCnt > i; i++)
    {
        stLvColm.iSubItem = i;
        ListView_InsertColumn(ghLvItemWnd, i, &stLvColm);
    }

    if (0 < dItems)
        InsertBucketItemListOn(0); //    中身追加

    //    リストビューツールチップ
    ghPalBucketLvTipWnd = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
                                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, ghPalBucketHostWnd, nullptr, hInstance, nullptr);
    AppUiFontApply(ghPalBucketLvTipWnd, FALSE);

    //    ツールチップをコールバックで割り付け
    ZeroMemory(&stToolInfo, sizeof(TTTOOLINFO));
    stToolInfo.cbSize = sizeof(TTTOOLINFO);
    stToolInfo.uFlags = TTF_SUBCLASS;
    stToolInfo.hinst = nullptr; //
    stToolInfo.hwnd = ghLvItemWnd;
    stToolInfo.uId = IDLV_BT_ITEMVIEW;
    GetClientRect(ghLvItemWnd, &stToolInfo.rect);
    stToolInfo.lpszText = LPSTR_TEXTCALLBACK; //    コレを指定するとコールバックになる
    SendMessage(ghPalBucketLvTipWnd, TTM_ADDTOOL, 0, (LPARAM)&stToolInfo);
    SendMessage(ghPalBucketLvTipWnd, TTM_SETMAXTIPWIDTH, 0, 0); //    チップの幅。０設定でいい。これしとかないと改行されない

    InsertBucketOnSize(ghPalBucketHostWnd, SIZE_RESTORED, clRect.right, clRect.bottom);

    if (!(gbTmpltDock))
    {
        ShowWindow(ghPalBucketHostWnd, SW_SHOW);
        UpdateWindow(ghPalBucketHostWnd);
    }

    return ghPalBucketHostWnd;
}
//-------------------------------------------------------------------------------------------------

// フローティングブラシテンプレの位置リセット
HRESULT PaletteBucketPositionReset(HWND hMainWnd)
{
    RECT wdRect, rect;

    GetWindowRect(hMainWnd, &wdRect);
    rect.left = wdRect.right + 32;
    rect.top = wdRect.top + 32;
    rect.right = BT_WIDTH;
    rect.bottom = BT_HEIGHT;

    SetWindowPos(ghPalBucketHostWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_SHOWWINDOW | SWP_NOZORDER);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

HRESULT PaletteBucketReload(VOID)
{
    HRESULT hRslt;

    hRslt = PaletteListGroupsLoad(TRUE, &gvcPalBucketTmpls);
    if (FAILED(hRslt))
        return hRslt;

    if (gNowGroup >= gvcPalBucketTmpls.size())
        gNowGroup = gvcPalBucketTmpls.empty() ? 0 : (UINT)gvcPalBucketTmpls.size() - 1;

    if (ghCtgryBxWnd)
    {
        PaletteListComboReload(ghCtgryBxWnd, gvcPalBucketTmpls);
        if (!(gvcPalBucketTmpls.empty()))
            ComboBox_SetCurSel(ghCtgryBxWnd, gNowGroup);
    }

    if (ghLvItemWnd)
    {
        if (gvcPalBucketTmpls.empty())
            ListView_DeleteAllItems(ghLvItemWnd);
        else
            InsertBucketItemListOn(gNowGroup);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ドッキング状態で発生・くっついてるウインドウがリサイズされたら
VOID PaletteBucketResize(HWND hPrntWnd, LPRECT pstFrame)
{
    const APP_DOCKED_WINDOW_LAYOUT stDockedLayout = AppLayoutDockedWindowsCalc(
        pstFrame,
        AppViewContextGet().dSplitPos,
        AppLayoutDockTabHeightGet(DockingTabGet()),
        gbDockTmplView);

    UNREFERENCED_PARAMETER(hPrntWnd);

    if (!(ghPalBucketHostWnd))
        return;

    AppLayoutApplyDockedWindows(stDockedLayout, nullptr, nullptr, nullptr, ghPalBucketHostWnd);

    return;
}
//-------------------------------------------------------------------------------------------------

// ツールバーサブクラス
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
//-------------------------------------------------------------------------------------------------

// ウインドウプロシージャ
LRESULT CALLBACK InsertBucketProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_SIZE, InsertBucketOnSize);
        HANDLE_MSG(hWnd, WM_COMMAND, InsertBucketOnCommand);
        HANDLE_MSG(hWnd, WM_NOTIFY, InsertBucketOnNotify); //    コモンコントロールの個別イベント

    case WM_MOUSEWHEEL:
        SendMessage(ghLvItemWnd, WM_MOUSEWHEEL, wParam, lParam);
        return 0;

    case WM_CONTEXTMENU:
        return 0;
    case WM_CLOSE:
        ShowWindow(ghPalBucketHostWnd, SW_HIDE);
        return 0;

    case WMP_BRUSH_TOGGLE:
        if (gbPalBucketMode)
        {
            SendMessage(ghPalBucketToolBarWnd, TB_SETSTATE, IDM_BRUSH_ON_OFF, TBSTATE_ENABLED);
            gbPalBucketMode = FALSE;
        }
        else
        {
            SendMessage(ghPalBucketToolBarWnd, TB_SETSTATE, IDM_BRUSH_ON_OFF, (TBSTATE_CHECKED | TBSTATE_ENABLED));
            gbPalBucketMode = TRUE;
        }
        ViewBrushStyleSetting(gbPalBucketMode, nullptr); //    ビューウインドウにモード付ける
        InsertBucketInvalidateSelection();
        return gbPalBucketMode;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// COMMANDメッセージの受け取り。ボタン押されたとかで発生
VOID InsertBucketOnCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    INT rslt;
    UINT dClm;
    LRESULT lRslt;
    LONG_PTR rdExStyle;
    TCHAR atItem[SUB_STRING];

    ZeroMemory(atItem, sizeof(atItem));

    switch (id)
    {
    case IDCB_BT_CATEGORY: //    カテゴリ選択コンボックス
        if (0 < gvcPalBucketTmpls.size())
        {
            if (CBN_SELCHANGE == codeNotify) //    選択変更されたら
            {
                rslt = ComboBox_GetCurSel(ghCtgryBxWnd);
                gNowGroup = rslt;

                InsertBucketItemListOn(rslt);

                //    Brush解除
                gbPalBucketMode = FALSE;
                SendMessage(ghPalBucketToolBarWnd, TB_SETSTATE, IDM_BRUSH_ON_OFF, TBSTATE_ENABLED);
                if (!(gvcPalBucketTmpls.at(gNowGroup).vcItems.empty()))
                {
                    StringCchCopy(atItem, SUB_STRING, gvcPalBucketTmpls.at(gNowGroup).vcItems.at(0).c_str());
                    InsertBucketCurrentPatternSet(atItem);
                    ViewBrushStyleSetting(gbPalBucketMode, atItem);
                }
                else
                {
                    InsertBucketCurrentPatternSet(nullptr);
                    ViewBrushStyleSetting(gbPalBucketMode, nullptr);
                }
                InsertBucketInvalidateSelection();
            }
        }
        break;

    case IDM_BRUSH_ON_OFF: //    ここに来る時点でON/OFF切り替わってる
        lRslt = SendMessage(ghPalBucketToolBarWnd, TB_GETSTATE, IDM_BRUSH_ON_OFF, 0);
        gbPalBucketMode = (lRslt & TBSTATE_CHECKED) ? TRUE : FALSE;
        ViewBrushStyleSetting(gbPalBucketMode, nullptr); //    状態をおくる
        InsertBucketInvalidateSelection();
        break;

    case IDM_WINDOW_CHANGE:
        WindowFocusChange(WND_BRUSH, 1);
        break;
    case IDM_WINDOW_CHG_RVRS:
        WindowFocusChange(WND_BRUSH, -1);
        break;

    case IDM_TMPL_GRID_INCREASE:
    case IDM_TMPL_GRID_DECREASE:
        dClm = PaletteListGridFluctuate(ghLvItemWnd, ((IDM_TMPL_GRID_INCREASE == id) ? 1 : -1));
        if (dClm)
        {
            gBrhClmCnt = dClm;
            InsertBucketItemListOn(gNowGroup);
            InitParamValue(INIT_SAVE, VL_BRUSHTMP_CLM, gBrhClmCnt);
        }
        break;

    case IDM_TMPLT_GROUP_PREV:
        if (0 < gNowGroup)
        {
            gNowGroup--;
            ComboBox_SetCurSel(ghCtgryBxWnd, gNowGroup);
            InsertBucketItemListOn(gNowGroup);
        }
        break;

    case IDM_TMPLT_GROUP_NEXT:
        if ((gNowGroup + 1) < gvcPalBucketTmpls.size())
        {
            gNowGroup++;
            ComboBox_SetCurSel(ghCtgryBxWnd, gNowGroup);
            InsertBucketItemListOn(gNowGroup);
        }
        break;

    case IDM_TOPMOST_TOGGLE: //    常時最全面と通常ウインドウのトグル
        rdExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        if (WS_EX_TOPMOST & rdExStyle)
        {
            SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            InitWindowTopMost(INIT_SAVE, WDP_BRTMPL, 0);
        }
        else
        {
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            InitWindowTopMost(INIT_SAVE, WDP_BRTMPL, 1);
        }
        break;

    default:
        break;
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// サイズ変更された
VOID InsertBucketOnSize(HWND hWnd, UINT state, INT cx, INT cy)
{
    LONG width;
    RECT cbxRect, tbItemRect;

    if (!(ghPalBucketToolBarWnd))
        return;
    ZeroMemory(&tbItemRect, sizeof(RECT));
    SendMessage(ghPalBucketToolBarWnd, TB_GETITEMRECT, 0, (LPARAM)&tbItemRect);
    width = tbItemRect.right - tbItemRect.left;
    if (0 >= width)
        width = 24;
    if (width > cx)
        width = cx;
    MoveWindow(ghPalBucketToolBarWnd, 0, -3, width, tbItemRect.bottom - tbItemRect.top, TRUE);

    if (!(ghCtgryBxWnd))
        return;
    MoveWindow(ghCtgryBxWnd, width, 0, cx - width, PB_COMBO_DROPDOWN_HEIGHT, TRUE);
    GetClientRect(ghCtgryBxWnd, &cbxRect);

    if (!(ghLvItemWnd))
        return;
    MoveWindow(ghLvItemWnd, 0, cbxRect.bottom, cx, cy - cbxRect.bottom, TRUE);

    PaletteListResizeColumns(ghLvItemWnd, gBrhClmCnt);

    return;
}
//-------------------------------------------------------------------------------------------------

// ノーティファイメッセージの処理
LRESULT InsertBucketOnNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    HWND hLvWnd;
    INT nmCode;
    LPNMLISTVIEW pstLv;

    if (IDLV_BT_ITEMVIEW == idFrom)
    {
        if (NM_CUSTOMDRAW == pstNmhdr->code)
        {
            LPNMLVCUSTOMDRAW pstCustomDraw;

            pstCustomDraw = (LPNMLVCUSTOMDRAW)pstNmhdr;
            switch (pstCustomDraw->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT:
                return CDRF_NOTIFYPOSTPAINT;

            case CDDS_ITEMPOSTPAINT:
                InsertBucketDrawCellBorder(pstCustomDraw->nmcd.hdc, (INT)pstCustomDraw->nmcd.dwItemSpec);
                return CDRF_DODEFAULT;
            }
        }

        pstLv = (LPNMLISTVIEW)pstNmhdr;

        hLvWnd = pstLv->hdr.hwndFrom;
        nmCode = pstLv->hdr.code;
    }

    return 0; //    何もないなら０を戻す
}
//-------------------------------------------------------------------------------------------------

static VOID PaletteBucketListOnMouseButtonUp(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    PALETTE_LIST_HIT stHit;
    LPCTSTR ptItem;

    UNREFERENCED_PARAMETER(keyFlags);

    if (!(PaletteListHitTest(hWnd, gBrhClmCnt, POINT{x, y}, &stHit)))
    {
        ViewFocusSet();
        return;
    }

    PaletteListClearSelection(hWnd);

    ptItem = PaletteListGroupItemGet(gvcPalBucketTmpls, gNowGroup, stHit.iIndex);
    if (!(ptItem))
    {
        ViewFocusSet();
        return;
    }

    gbPalBucketMode = TRUE;
    SendMessage(ghPalBucketToolBarWnd, TB_SETSTATE, IDM_BRUSH_ON_OFF, (TBSTATE_CHECKED | TBSTATE_ENABLED));
    InsertBucketCurrentPatternSet(ptItem);
    ViewBrushStyleSetting(gbPalBucketMode, ptItem);
    InsertBucketInvalidateSelection();

    ViewFocusSet();
}
//-------------------------------------------------------------------------------------------------

// アイテムをリストに展開
UINT InsertBucketItemListOn(UINT listNum)
{
    if (gvcPalBucketTmpls.empty() || listNum >= gvcPalBucketTmpls.size())
    {
        return 0;
    }

    const vector<wstring> &vcItems = gvcPalBucketTmpls.at(listNum).vcItems;

    PaletteListPopulate(ghLvItemWnd, vcItems, gBrhClmCnt);

    InsertBucketInvalidateSelection();

    return (UINT)vcItems.size();
}
//-------------------------------------------------------------------------------------------------

static VOID InsertBucketCurrentPatternSet(LPCTSTR ptPattern)
{
    if (ptPattern && ptPattern[0])
    {
        StringCchCopy(gatCurrentBucketPattern, SUB_STRING, ptPattern);
    }
    else
    {
        gatCurrentBucketPattern[0] = TEXT('\0');
    }
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN InsertBucketCellMatches(INT iPos)
{
    INT_PTR items;

    if (!(gbPalBucketMode) || 0 > iPos || !(gatCurrentBucketPattern[0]) || 0 >= gvcPalBucketTmpls.size())
        return FALSE;

    items = (INT_PTR)gvcPalBucketTmpls.at(gNowGroup).vcItems.size();
    if (iPos >= items)
        return FALSE;

    return (0 == lstrcmp(gvcPalBucketTmpls.at(gNowGroup).vcItems.at(iPos).c_str(), gatCurrentBucketPattern));
}
//-------------------------------------------------------------------------------------------------

static VOID InsertBucketInvalidateSelection(VOID)
{
    if (ghLvItemWnd)
    {
        InvalidateRect(ghLvItemWnd, nullptr, FALSE);
    }
}
//-------------------------------------------------------------------------------------------------

static BOOLEAN InsertBucketCellRectGet(INT iItem, INT iSubItem, LPRECT pstRect)
{
    RECT itemRect;
    INT iColumn;

    if (!(pstRect) || !(ghLvItemWnd) || 0 > iItem || 0 > iSubItem || gBrhClmCnt <= (UINT)iSubItem)
        return FALSE;

    ZeroMemory(&itemRect, sizeof(RECT));
    if (!(ListView_GetItemRect(ghLvItemWnd, iItem, &itemRect, LVIR_BOUNDS)))
        return FALSE;

    *pstRect = itemRect;
    pstRect->right = pstRect->left;

    for (iColumn = 0; iSubItem > iColumn; iColumn++)
    {
        pstRect->left += ListView_GetColumnWidth(ghLvItemWnd, iColumn);
    }

    pstRect->right = pstRect->left + ListView_GetColumnWidth(ghLvItemWnd, iSubItem);

    return (pstRect->right > pstRect->left && pstRect->bottom > pstRect->top);
}
//-------------------------------------------------------------------------------------------------

static VOID InsertBucketDrawCellBorder(HDC hdc, INT iItem)
{
    HBRUSH hBorderBrush;
    RECT rect;
    INT iPos, iSubItem;

    if (!(hdc) || !(ghLvItemWnd) || !(gbPalBucketMode) || !(gatCurrentBucketPattern[0]))
        return;

    hBorderBrush = CreateSolidBrush(RGB(0, 120, 215));
    if (!(hBorderBrush))
        return;

    for (iSubItem = 0; (INT)gBrhClmCnt > iSubItem; iSubItem++)
    {
        iPos = iItem * gBrhClmCnt + iSubItem;
        if (!(InsertBucketCellMatches(iPos)))
            continue;

        ZeroMemory(&rect, sizeof(RECT));
        if (!(InsertBucketCellRectGet(iItem, iSubItem, &rect)))
            continue;

        InflateRect(&rect, -1, -1);
        FrameRect(hdc, &rect, hBorderBrush);
        InflateRect(&rect, -1, -1);
        FrameRect(hdc, &rect, hBorderBrush);
    }

    DeleteObject(hBorderBrush);
}
//-------------------------------------------------------------------------------------------------

// カテゴリコンボックスサブクラス
LRESULT CALLBACK gpfInsertBucketCtgryProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
            SendMessage(ghPalBucketHostWnd, WM_COMMAND, wParam, lParam);
            return 0;
        }
        break;

    case WM_MOUSEWHEEL:
        SendMessage(ghLvItemWnd, WM_MOUSEWHEEL, wParam, lParam);
        return 0;
    }

    return CallWindowProc(gpfOrigPalBucketCtgryProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// アイテムリストビューサブクラス
LRESULT CALLBACK gpfInsertBucketItemProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT id;

    switch (msg)
    {
        HANDLE_MSG(hWnd, WM_NOTIFY, InsertBucketListOnNotify); //    コモンコントロールの個別イベント

    case WM_CONTEXTMENU:
        return 0;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
        PaletteListClearSelection(hWnd);
        return 0;

    case WM_LBUTTONDOWN:
        return 0;

    case WM_LBUTTONUP:
        PaletteBucketListOnMouseButtonUp(hWnd, (INT)(SHORT)LOWORD(lParam), (INT)(SHORT)HIWORD(lParam), (UINT)wParam);
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
            SendMessage(ghPalBucketHostWnd, WM_COMMAND, wParam, lParam);
            return 0;
        }
        break;
    }

    return CallWindowProc(gpfOrigPalBucketItemProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// ノーティファイメッセージの処理
LRESULT InsertBucketListOnNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    HWND hLvWnd;
    INT nmCode;
    LPNMLISTVIEW pstLv;
    LPNMTTDISPINFO pstDispInfo;

    pstLv = (LPNMLISTVIEW)pstNmhdr;

    //    リストビュー自体のプロシージャなので
    hLvWnd = hWnd;
    nmCode = pstLv->hdr.code;

    if (TTN_GETDISPINFO == nmCode)
    {
        if (IDLV_BT_ITEMVIEW == idFrom)
        {
            pstDispInfo = (LPNMTTDISPINFO)pstNmhdr;

            ZeroMemory(&(pstDispInfo->szText), sizeof(pstDispInfo->szText));
            pstDispInfo->lpszText = pstDispInfo->szText;

            PaletteListBuildTooltipText(hLvWnd, gvcPalBucketTmpls, gNowGroup, gBrhClmCnt, pstDispInfo->szText, 80);

            return 0;
        }
    }

    return CallWindowProc(gpfOrigPalBucketItemProc, hWnd, WM_NOTIFY, (WPARAM)idFrom, (LPARAM)pstNmhdr);
}
//-------------------------------------------------------------------------------------------------
