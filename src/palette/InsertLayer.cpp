// 합성용 레이어 박스 뷰 관리
#include "InsertUi.h"
#include "DocViewBridgeInternal.h"
#include "ViewCentralInternal.h"
#include "UiText.h"
#include "MenuMnemonic.h"
//-------------------------------------------------------------------------------------------------
struct LAYERBOXSTRUCT
{
    LONG id;
    INSERT_UI_GEOMETRY stGeometry;
    HWND hBoxWnd;
    HWND hTextWnd;
    HWND hToolWnd;
    vector<ONELINE> vcLyrImg;
};

using LAYER_ITR = list<LAYERBOXSTRUCT>::iterator;
using LYLINE_ITR = vector<ONELINE>::iterator;

struct LAYER_FILE_STATE
{
    LONG dNextBoxId{};
    INT dToolBarHeight{};
    BOOLEAN bQuickClose{TRUE};
    WNDPROC pfOrigToolbarProc{};
    WNDPROC pfOrigEditProc{};
    HIMAGELIST hImageList{};
    list<LAYERBOXSTRUCT> ltLayers;
};

namespace
{
constexpr auto kLayerBoxClass = TEXT("LAYER_BOX");
constexpr int kLayerBoxWidth = 310;
constexpr int kLayerBoxHeight = 220;
constexpr int kEdgeBlankNarrow = 16;
constexpr int kEdgeBlankWide = 22;
constexpr UINT kLayerToolbarStringIndices[] = {0, 1, 3, 5, 7};

TBBUTTON gstTBInfo[] = {
    {0, IDM_LYB_INSERT, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0},   //    挿入
    {1, IDM_LYB_OVERRIDE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0}, //    上書
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {2, IDM_LYB_COPY, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0}, //    コピー
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {3, IDM_LYB_DO_EDIT, TBSTATE_ENABLED, TBSTYLE_CHECK | TBSTYLE_AUTOSIZE, {0, 0}, 0, 0}, //    編集ボックスON/OFF
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0},
    {4, IDM_LYB_DELETE, TBSTATE_ENABLED, TBSTYLE_AUTOSIZE, {0, 0}, 0, 0} //    20120507    内容クルヤー

};
} // namespace
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

EXTERNED BYTE gbAlpha; // 透明度
static LAYER_FILE_STATE gstLayerState;

namespace
{
[[nodiscard]] auto FindLayerByWindow(HWND hWnd) -> LAYER_ITR
{
    return find_if(gstLayerState.ltLayers.begin(), gstLayerState.ltLayers.end(), [hWnd](const LAYERBOXSTRUCT &layer) {
        return layer.hBoxWnd == hWnd;
    });
}

void AppendEmptyLayerLine(LAYERBOXSTRUCT &layer)
{
    ONELINE line;
    ZeroONELINE(&line);
    layer.vcLyrImg.push_back(line);
}
}
//-------------------------------------------------------------------------------------------------

static LRESULT CALLBACK gpfLayerTBProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK gpfLyrEditProc(HWND, UINT, WPARAM, LPARAM);
static HWND LayerCreateWindow(HINSTANCE, DWORD);
static HWND LayerCreateTextWindow(const LAYERBOXSTRUCT &, HINSTANCE);
static VOID LayerLoadInitialContent(LAYER_ITR, LPCTSTR, BOOLEAN, UINT);
static HWND LayerCreateToolbar(HWND, HINSTANCE);
static VOID LayerCreateToolbarOptions(HWND, HINSTANCE);
static VOID LayerHandleInsertCommand(HWND, UINT);
static VOID LayerHandleEditToggle(HWND);
static VOID LayerHandleQuickCloseToggle(HWND);
static VOID LayerHandleEdgeBlankChange(HWND, HWND, UINT);
static BOOLEAN LayerEditHandleCommandShortcut(HWND, INT);

LRESULT CALLBACK LayerBoxProc(HWND, UINT, WPARAM, LPARAM);

BOOLEAN Lyb_OnCreate(HWND, LPCREATESTRUCT); // WM_CREATE の処理
VOID Lyb_OnCommand(HWND, INT, HWND, UINT);
VOID Lyb_OnKey(HWND, UINT, BOOL, INT, UINT);
VOID Lyb_OnPaint(HWND);
VOID Lyb_OnDestroy(HWND);
VOID Lyb_OnMoving(HWND, LPRECT);
BOOL Lyb_OnWindowPosChanging(HWND, LPWINDOWPOS);
VOID Lyb_OnWindowPosChanged(HWND, const LPWINDOWPOS);
VOID Lyb_OnLButtonDown(HWND, BOOL, INT, INT, UINT);
VOID Lyb_OnContextMenu(HWND, HWND, UINT, UINT);

HRESULT LayerEditOnOff(HWND, UINT);

HRESULT LayerStringObliterate(LAYER_ITR);
HRESULT LayerFromString(LAYER_ITR, LPCTSTR);
HRESULT LayerFromSelectArea(LAYER_ITR, UINT);
HRESULT LayerFromClipboard(LAYER_ITR);
HRESULT LayerForClipboard(HWND, UINT);
HRESULT LayerOnDelete(HWND);
INT LayerInputLetter(LAYER_ITR, INT, INT, TCHAR);
LPTSTR LayerLineTextGetAlloc(LAYER_ITR, INT);
HRESULT LayerBoxSetString(LAYER_ITR, LPCTSTR, UINT, LPPOINT, UINT);
HRESULT LayerBoxSizeAdjust(LAYER_ITR);

INT LayerTransparentAdjust(LAYER_ITR, INT, INT);

#ifdef EDGE_BLANK_STYLE
HRESULT LayerEdgeBlankSizeCheck(HWND, INT);
#endif

static HWND LayerCreateWindow(HINSTANCE hInst, DWORD dwStyle)
{
    return CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, kLayerBoxClass,
                          TEXT("레이어"), dwStyle, 0, 0, kLayerBoxWidth, kLayerBoxHeight, nullptr, nullptr, hInst, nullptr);
}

static HWND LayerCreateTextWindow(const LAYERBOXSTRUCT &stLayer, HINSTANCE hInst)
{
    RECT rect;

    GetClientRect(stLayer.hBoxWnd, &rect);
    return CreateWindowEx(0, WC_EDIT, TEXT(""),
                          WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                          0, gstLayerState.dToolBarHeight, rect.right, rect.bottom - gstLayerState.dToolBarHeight,
                          stLayer.hBoxWnd, (HMENU)IDE_LYB_TEXTEDIT, hInst, nullptr);
}

static HWND LayerCreateToolbar(HWND hWnd, HINSTANCE hInst)
{
    LPCTSTR aptToolbarTexts[] = {
        UiTextGetLabel(IDM_LYB_INSERT),
        UiTextGetLabel(IDM_LYB_OVERRIDE),
        UiTextGetLabel(IDM_LYB_COPY),
        UiTextGetLabel(IDM_LYB_DO_EDIT),
        UiTextGetLabel(IDM_LYB_DELETE)};
    HWND hToolWnd;

    hToolWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TOOLBARCLASSNAME, TEXT("toolbar"),
                              WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
                              0, 0, 0, 0, hWnd, (HMENU)IDW_LYB_TOOL_BAR, hInst, nullptr);

    if (0 == gstLayerState.dToolBarHeight)
    {
        gstLayerState.dToolBarHeight = InsertUiToolbarInitialise(hToolWnd, gstLayerState.hImageList, gstTBInfo, (UINT)std::size(gstTBInfo),
                                                                 kLayerToolbarStringIndices, aptToolbarTexts, (UINT)std::size(aptToolbarTexts), FALSE);
    }
    else
    {
        InsertUiToolbarInitialise(hToolWnd, gstLayerState.hImageList, gstTBInfo, (UINT)std::size(gstTBInfo),
                                  kLayerToolbarStringIndices, aptToolbarTexts, (UINT)std::size(aptToolbarTexts), FALSE);
    }
    gstLayerState.pfOrigToolbarProc = SubclassWindow(hToolWnd, gpfLayerTBProc);

    return hToolWnd;
}

static VOID LayerCreateToolbarOptions(HWND hToolWnd, HINSTANCE hInst)
{
    HFONT hUIFont;
    HWND hBtn;
#ifdef EDGE_BLANK_STYLE
    HWND hWorkWnd;
#endif

    hUIFont = AppUiFontGet();
    if (!(hUIFont))
        hUIFont = (HFONT)GetStockFont(DEFAULT_GUI_FONT);

    hBtn = CreateWindowExW(
        0, WC_BUTTON, L"작업 후 창 닫기", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 150, 1, 110, 22, hToolWnd, (HMENU)IDCB_LAYER_QUICKCLOSE, hInst, nullptr);

    CheckDlgButton(hToolWnd, IDCB_LAYER_QUICKCLOSE, gstLayerState.bQuickClose ? BST_CHECKED : BST_UNCHECKED);
    SendMessageW(hBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

#ifdef EDGE_BLANK_STYLE
    hWorkWnd = CreateWindowExW(0, WC_COMBOBOX, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 260, 1, 120, 70, hToolWnd, (HMENU)IDCB_LAYER_EDGE_BLANK, hInst, nullptr);
    ComboBox_AddString(hWorkWnd, L"여백 추가 없음");
    ComboBox_AddString(hWorkWnd, L"작은 여백 추가");
    ComboBox_AddString(hWorkWnd, L"큰 여백 추가");
    ComboBox_SetCurSel(hWorkWnd, 0);
    SendMessageW(hWorkWnd, WM_SETFONT, (WPARAM)hUIFont, TRUE);
#endif
}

static VOID LayerLoadInitialContent(LAYER_ITR itLyr, LPCTSTR ptStr, BOOLEAN bSelect, UINT bSqSel)
{
    if (ptStr)
    {
        TRACE(TEXT("LAYER from STRING"));
        LayerFromString(itLyr, ptStr);
    }
    else if (bSelect)
    {
        TRACE(TEXT("LAYER from Select"));
        LayerFromSelectArea(itLyr, bSqSel);
    }
    else
    {
        TRACE(TEXT("LAYER from ClipBoard"));
        LayerFromClipboard(itLyr);
    }
}

//-------------------------------------------------------------------------------------------------

// ボックスの作成・最初の1回のみ
VOID LayerBoxInitialise(HINSTANCE hInstance, LPRECT pstFrame)
{
    WNDCLASSEX wcex;
    HBITMAP hImg, hMsq;

    if (!(hInstance)) //    破壊命令
    {
        ImageList_Destroy(gstLayerState.hImageList);

        return;
    }

    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = LayerBoxProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_MENU + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = kLayerBoxClass;
    wcex.hIconSm = nullptr;

    RegisterClassEx(&wcex);

    gstLayerState = LAYER_FILE_STATE{};

    // ツールバー用イメージリスト作成
    gstLayerState.hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 5, 1);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMP_LAYERINSERT)));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMQ_LAYERINSERT)));
    ImageList_Add(gstLayerState.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMP_LAAYEROVERWRITE)));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMQ_LAAYEROVERWRITE)));
    ImageList_Add(gstLayerState.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMP_COPY)));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMQ_COPY)));
    ImageList_Add(gstLayerState.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMP_LAYERTEXTEDIT)));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMQ_LAYERTEXTEDIT)));
    ImageList_Add(gstLayerState.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    hImg = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMP_DELETE)));
    hMsq = LoadBitmap(hInstance, MAKEINTRESOURCE((IDBMQ_DELETE)));
    ImageList_Add(gstLayerState.hImageList, hImg, hMsq);
    DeleteBitmap(hImg);
    DeleteBitmap(hMsq);

    return;
}
//-------------------------------------------------------------------------------------------------

// れいやぼっくちゅのアルファを更新!?
HRESULT LayerBoxAlphaSet(UINT dParam)
{
    gbAlpha = dParam & 0xFF;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

/*！
    レイヤボックスを作成
    @param[in]    hInst    実存値
    @param[in]    ptStr    表示すべき文字列なら有効、違うならnullptr
    @param[in]    bNormal    0x00普通に処理　0x10裏処理
    @return        HWND    作成されたレイヤボックスのウインドウハンドル
*/
HWND LayerBoxVisibalise(HINSTANCE hInst, LPCTSTR ptStr, UINT bNormal)
{
    INT x, y;
    RECT rect;
    DWORD dwStyle;
    auto caret = ViewCurrentCaret();

    BOOLEAN bSelect = FALSE;
    UINT bSqSel = 0;

    LAYERBOXSTRUCT stLayer;
    LAYER_ITR itLyr;

    stLayer.id = gstLayerState.dNextBoxId; //    ボックスの認識番号

    bSelect = IsSelecting(&bSqSel);

    stLayer.vcLyrImg.clear(); //    表示するデータの保持・AA用

    if (0x10 & bNormal)
    {
        dwStyle = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU;
    }
    else
    {
        dwStyle = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
    }

    //    場所は０にしておけばクライヤント位置で計算出来る
    stLayer.hBoxWnd = LayerCreateWindow(hInst, dwStyle);

    //    ＩＤをウインドウハンドルに保存しておく
    WndTagSet(stLayer.hBoxWnd, stLayer.id);

    SetLayeredWindowAttributes(stLayer.hBoxWnd, 0, gbAlpha, LWA_ALPHA);

    stLayer.hToolWnd = GetDlgItem(stLayer.hBoxWnd, IDW_LYB_TOOL_BAR);

    x = caret.dXdot;
    y = caret.dLine * LINE_HEIGHT;
    ViewPositionTransform(&x, &y, TRUE);
    TRACE(TEXT("%d x %d"), x, y);

    InsertUiPlaceContentAtViewPoint(ghViewWnd, stLayer.hBoxWnd, &(stLayer.stGeometry), gstLayerState.dToolBarHeight, x, y);

    GetClientRect(stLayer.hBoxWnd, &rect);

    //    編集用エディット
    stLayer.hTextWnd = LayerCreateTextWindow(stLayer, hInst);
    SetWindowFont(stLayer.hTextWnd, ghAaFont, TRUE);

    //    サブクラス
    gstLayerState.pfOrigEditProc = SubclassWindow(stLayer.hTextWnd, gpfLyrEditProc);

    //    レイヤリストに記録
    gstLayerState.ltLayers.push_back(stLayer);
    itLyr = gstLayerState.ltLayers.end();
    itLyr--; //    追加したのは末端だからこれでいい

    LayerLoadInitialContent(itLyr, ptStr, bSelect, bSqSel);

    if (!(0x10 & bNormal))
    {
        ShowWindow(stLayer.hBoxWnd, SW_SHOW);
        UpdateWindow(stLayer.hBoxWnd);

        GetWindowRect(stLayer.hBoxWnd, &rect);
        Lyb_OnMoving(stLayer.hBoxWnd, &rect);
    }

    gstLayerState.dNextBoxId++;

    return stLayer.hBoxWnd;
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスの位置を外部から変更
HRESULT LayerBoxPositionChange(HWND hWnd, LONG x, LONG y)
{
    const auto itLyr = FindLayerByWindow(hWnd);
    if (itLyr != gstLayerState.ltLayers.end())
    {
        x -= itLyr->stGeometry.stContentOffset.x;
        y -= itLyr->stGeometry.stContentOffset.y;
        SetWindowPos(hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ツールバーサブクラス
LRESULT CALLBACK gpfLayerTBProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT itemID;
    HDC hdc;
    HWND hWndChild;

    switch (msg)
    {
    case WM_CTLCOLORSTATIC: //    チェックボックスの文字列部分の色変更
        hdc = (HDC)(wParam);
        hWndChild = (HWND)(lParam);

        itemID = GetDlgCtrlID(hWndChild);

        if (IDCB_LAYER_QUICKCLOSE == itemID || IDCB_LAYER_EDGE_BLANK == itemID)
        {
            SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }
        break;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (SendMessage(hWnd, TB_GETHOTITEM, 0, 0) >= 0)
        {
            ReleaseCapture();
        }
        return 0;
    }

    return CallWindowProc(gstLayerState.pfOrigToolbarProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// エディットボックスサブクラス
static VOID LayerHandleInsertCommand(HWND hWnd, UINT id)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    INT iXpos;
    INT iYln;

    LayerContentsImportable(hWnd, id, &iXpos, &iYln, 0);
    DocViewResetCaret(iXpos, iYln);
    DocPageInfoRenew(-1, 1);
    EditChangeSetApply(stChangeSet);
    if (gstLayerState.bQuickClose)
    {
        DestroyWindow(hWnd);
    }
}

static VOID LayerHandleEditToggle(HWND hWnd)
{
    LRESULT lRslt;
    HWND hToolWnd;

    hToolWnd = GetDlgItem(hWnd, IDW_LYB_TOOL_BAR);
    lRslt = SendMessage(hToolWnd, TB_GETSTATE, IDM_LYB_DO_EDIT, 0);
    LayerEditOnOff(hWnd, (lRslt & TBSTATE_CHECKED) ? TRUE : FALSE);
    SendMessage(hToolWnd, TB_SETSTATE, IDM_LYB_INSERT, (lRslt & TBSTATE_CHECKED) ? 0 : TBSTATE_ENABLED);
    SendMessage(hToolWnd, TB_SETSTATE, IDM_LYB_OVERRIDE, (lRslt & TBSTATE_CHECKED) ? 0 : TBSTATE_ENABLED);
}

static VOID LayerHandleQuickCloseToggle(HWND hWnd)
{
    gstLayerState.bQuickClose = IsDlgButtonChecked(GetDlgItem(hWnd, IDW_LYB_TOOL_BAR), IDCB_LAYER_QUICKCLOSE) ? TRUE : FALSE;
    SetFocus(hWnd);
}

static VOID LayerHandleEdgeBlankChange(HWND hWnd, HWND hWndCtl, UINT codeNotify)
{
#ifdef EDGE_BLANK_STYLE
    INT bEdgeBlank;

    if (CBN_SELCHANGE != codeNotify)
        return;

    bEdgeBlank = ComboBox_GetCurSel(hWndCtl);
    if (1 == bEdgeBlank)
    {
        LayerEdgeBlankSizeCheck(hWnd, kEdgeBlankNarrow);
    }
    else if (2 == bEdgeBlank)
    {
        LayerEdgeBlankSizeCheck(hWnd, kEdgeBlankWide);
    }
    SetFocus(hWnd);
#endif
}

static BOOLEAN LayerEditHandleCommandShortcut(HWND hWnd, INT id)
{
    INT len;

    switch (id)
    {
    case IDM_PASTE:
        SendMessage(hWnd, WM_PASTE, 0, 0);
        return TRUE;
    case IDM_COPY:
        SendMessage(hWnd, WM_COPY, 0, 0);
        return TRUE;
    case IDM_CUT:
        SendMessage(hWnd, WM_CUT, 0, 0);
        return TRUE;
    case IDM_UNDO:
        SendMessage(hWnd, WM_UNDO, 0, 0);
        return TRUE;
    case IDM_ALLSEL:
        len = GetWindowTextLength(hWnd);
        SendMessage(hWnd, EM_SETSEL, 0, len);
        return TRUE;
    default:
        return FALSE;
    }
}

LRESULT CALLBACK gpfLyrEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT id;
    HWND hWndCtl;
    UINT codeNotify;

    switch (msg)
    {
    default:
        break;

    case WM_COMMAND:
        id = LOWORD(wParam);         //    発生したコマンドの識別子
        hWndCtl = (HWND)lParam;      //    コマンドを発生させた子ウインドウのハンドル
        codeNotify = HIWORD(wParam); //    追加の通知メッセージ
        TRACE(TEXT("[%X]LyrEdit COMMAND %d"), hWnd, id);

        if (LayerEditHandleCommandShortcut(hWnd, id))
        {
            return 0;
        }

        break;
    }

    return CallWindowProc(gstLayerState.pfOrigEditProc, hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスのウインドウプロシージャ
LRESULT CALLBACK LayerBoxProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_CREATE, Lyb_OnCreate);
        HANDLE_MSG(hWnd, WM_COMMAND, Lyb_OnCommand);
        HANDLE_MSG(hWnd, WM_PAINT, Lyb_OnPaint);
        HANDLE_MSG(hWnd, WM_DESTROY, Lyb_OnDestroy);
        HANDLE_MSG(hWnd, WM_KEYDOWN, Lyb_OnKey);
        HANDLE_MSG(hWnd, WM_LBUTTONDBLCLK, Lyb_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_CONTEXTMENU, Lyb_OnContextMenu);
        HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGING, Lyb_OnWindowPosChanging);
        HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGED, Lyb_OnWindowPosChanged);

    case WM_MOVING:
        Lyb_OnMoving(hWnd, (LPRECT)lParam);
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスのクリエイト。
BOOLEAN Lyb_OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    HINSTANCE lcInst = lpCreateStruct->hInstance; //    受け取った初期化情報から、インスタンスハンドルをひっぱる
    HWND hToolWnd;

    hToolWnd = LayerCreateToolbar(hWnd, lcInst);
    LayerCreateToolbarOptions(hToolWnd, lcInst);

    return TRUE;
}
//-------------------------------------------------------------------------------------------------

// COMMANDメッセージの受け取り。ボタン押されたとかで発生
VOID Lyb_OnCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDE_LYB_TEXTEDIT: //    レイヤ非表示にしてもKILLFOCUS出る
        if (EN_SETFOCUS == codeNotify)
        {
            TRACE(TEXT("LYREDIT_SETFOCUS"));
        }

        if (EN_KILLFOCUS == codeNotify)
        {
            TRACE(TEXT("LYREDIT_KILLFOCUS"));
        }
        break;

    case IDM_LYB_INSERT: //    貼り付ける
    case IDM_LYB_OVERRIDE:
        LayerHandleInsertCommand(hWnd, id);
        break;

    case IDM_LYB_COPY: //    クルップボードへ
        LayerForClipboard(hWnd, D_UNI);
        break;

    case IDM_LYB_DO_EDIT: //    文字列を編集
        LayerHandleEditToggle(hWnd);
        break;

    case IDM_LYB_DELETE: //    内容を削除
        LayerOnDelete(hWnd);
        break;

    case IDCB_LAYER_QUICKCLOSE: //    貼り付けたら閉じるか？
        LayerHandleQuickCloseToggle(hWnd);
        break;

#ifdef EDGE_BLANK_STYLE
    case IDCB_LAYER_EDGE_BLANK: //    白ヌキするか
        LayerHandleEdgeBlankChange(hWnd, hWndCtl, codeNotify);
        break;
#endif

    case IDM_LYB_TRANCE_RELEASE: //    透過選択を解除
        LayerTransparentToggle(hWnd, 0);
        InvalidateRect(hWnd, nullptr, TRUE);
        break;

    case IDM_LYB_TRANCE_ALL: //    空白を全部透過領域に設定
        LayerTransparentToggle(hWnd, 1);
        InvalidateRect(hWnd, nullptr, TRUE);
        break;

    default:
        TRACE(TEXT("Layer未知のコマンド %d"), id);
        break;
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// キーダウンが発生・キーボードで移動用
VOID Lyb_OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    RECT rect;
    INT dotLeft = 0;

    GetWindowRect(hWnd, &rect);

    if (fDown)
    {
        switch (vk)
        {
        case VK_RIGHT:
            TRACE(TEXT("右"));
            dotLeft = 1;
            break;
        case VK_LEFT:
            TRACE(TEXT("左"));
            dotLeft = -1;
            break;
        case VK_DOWN:
            TRACE(TEXT("下"));
            rect.top += LINE_HEIGHT;
            break;
        case VK_UP:
            TRACE(TEXT("上"));
            rect.top -= LINE_HEIGHT;
            break;
        // 2024kai Enterでレイヤボックス上書き貼り付け対応
        case VK_RETURN:
            TRACE(TEXT("Enter"));
            Lyb_OnCommand(hWnd, IDM_LYB_OVERRIDE, nullptr, 0);
            break;
        // 2024kai Escでレイヤボックスを閉じる
        case VK_ESCAPE:
            TRACE(TEXT("Esc"));
            DestroyWindow(hWnd);
            break;
        default:
            return;
        }
    }
    // 2024kai Shift←→で11ドットずつ移動、さらにCtrlも押していると5ドットずつ移動
    BOOLEAN gbShiftOn = (0x8000 & GetKeyState(VK_SHIFT)) ? TRUE : FALSE;
    BOOLEAN gbCtrlOn = (0x8000 & GetKeyState(VK_CONTROL)) ? TRUE : FALSE;
    if (gbShiftOn && dotLeft != 0)
    {
        if (gbCtrlOn)
        {
            dotLeft *= 5;
        }
        else
        {
            dotLeft *= 11;
        }
    }
    rect.left += dotLeft;

    SetWindowPos(hWnd, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    Lyb_OnMoving(hWnd, &rect);

    return;
}
//-------------------------------------------------------------------------------------------------

// PAINT。無効領域が出来たときに発生。背景の扱いに注意。背景を塗りつぶしてから、オブジェクトを描画
VOID Lyb_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HFONT hFtOld;
    COLORREF clrTextOld, clrBackOld;
    HDC hdc;
    INT height;
    UINT_PTR iLines, i;
    LPTSTR ptText;
    RECT rect;
    LAYER_ITR itLyr;

    UINT_PTR mz, cchLen;
    UINT bStyle, cchMr, cbSize;
    INT width, rdStart;
    BOOLEAN doDraw, bRslt;

    hdc = BeginPaint(hWnd, &ps);

#ifdef DO_TRY_CATCH
    try
    {
#endif

        height = gstLayerState.dToolBarHeight;

        for (itLyr = gstLayerState.ltLayers.begin(); itLyr != gstLayerState.ltLayers.end(); itLyr++)
        {
            if (itLyr->hBoxWnd == hWnd)
            {
                clrTextOld = SetTextColor(hdc, CLR_BLACK);
                clrBackOld = SetBkColor(hdc, CLR_WHITE);

                GetClientRect(hWnd, &rect);
                FillRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));

                hFtOld = SelectFont(hdc, ghAaFont);

                iLines = itLyr->vcLyrImg.size();

                for (i = 0; iLines > i; i++)
                {
                    cchLen = itLyr->vcLyrImg.at(i).vcLine.size(); //    必要文字数
                    if (0 >= cchLen)
                    {
                        height += LINE_HEIGHT;
                        continue;
                    }

                    cbSize = (cchLen + 1) * sizeof(TCHAR);
                    ptText = (LPTSTR)malloc(cbSize); //    ぬるたーみねーた分増やす
                    ZeroMemory(ptText, cbSize);

                    bStyle = itLyr->vcLyrImg.at(i).vcLine.at(0).mzStyle;
                    bStyle &= CT_LYR_TRNC;
                    cchMr = 0;
                    width = 0;
                    rdStart = 0; // itLyr->vcLyrImg.at( i ).dOffset;
                    doDraw = FALSE;

                    for (mz = 0; cchLen >= mz; mz++)
                    {
                        if (cchLen == mz)
                        {
                            doDraw = TRUE;
                        } //    末端まできちゃったら
                        else
                        {
                            //    同じスタイルが続くなら
                            if (bStyle == (itLyr->vcLyrImg.at(i).vcLine.at(mz).mzStyle & CT_LYR_TRNC))
                            {
                                ptText[cchMr++] = itLyr->vcLyrImg.at(i).vcLine.at(mz).cchMozi; //    壱繋がりの文字列として確保
                                width += itLyr->vcLyrImg.at(i).vcLine.at(mz).rdWidth;
                            }
                            else
                            {
                                doDraw = TRUE;
                            }
                        }

                        if (doDraw) //    描画タイミングであるなら
                        {
                            if (bStyle & CT_LYR_TRNC) //    透過部分の場合背景色と枠塗り潰し
                            {
                                SetBkColor(hdc, CLR_SILVER); //    LTGRAY_BRUSH

                                SetRect(&rect, rdStart, height, rdStart + width, height + LINE_HEIGHT);
                                FillRect(hdc, &rect, GetStockBrush(LTGRAY_BRUSH));
                            }
                            else
                            {
                                SetBkColor(hdc, CLR_WHITE);
                            }

                            bRslt = ExtTextOut(hdc, rdStart, height, 0, nullptr, ptText, cchMr, nullptr);
                            if (!(bRslt))
                            {
                                TRACE(TEXT("ExtTextOut error"));
                            }

                            if (cchLen != mz)
                            {
                                rdStart += width;
                                //    描画したら、今の文字を新しいスタイルとして登録してループ再開
                                bStyle = itLyr->vcLyrImg.at(i).vcLine.at(mz).mzStyle;
                                bStyle &= CT_LYR_TRNC;
                                ZeroMemory(ptText, cbSize);
                                ptText[0] = itLyr->vcLyrImg.at(i).vcLine.at(mz).cchMozi;
                                width = itLyr->vcLyrImg.at(i).vcLine.at(mz).rdWidth;
                                cchMr = 1;
                            }
                            doDraw = FALSE;
                        }
                    }

                    FREE(ptText);

                    height += LINE_HEIGHT;
                }

                SelectFont(hdc, hFtOld);

                break;
            }
        }

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        ETC_MSG(err.what(), 0);
        return;
    }
    catch (...)
    {
        ETC_MSG(("etc error"), 0);
        return;
    }
#endif

    EndPaint(hWnd, &ps);

    return;
}
//-------------------------------------------------------------------------------------------------

// ウインドウを閉じるときに発生。デバイスコンテキストとか確保した画面構造のメモリとかも終了。
VOID Lyb_OnDestroy(HWND hWnd)
{
    LAYER_ITR itLyr;

    for (itLyr = gstLayerState.ltLayers.begin(); itLyr != gstLayerState.ltLayers.end(); itLyr++)
    {
        if (itLyr->hBoxWnd == hWnd)
        {
            LayerStringObliterate(itLyr);
            MainStatusBarSetText(SB_LAYER, TEXT(""));

            gstLayerState.ltLayers.erase(itLyr);

            break;
        }
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// ウィンドウのサイズ変更が完了する前に送られてくる
BOOL Lyb_OnWindowPosChanging(HWND hWnd, LPWINDOWPOS pstWpos)
{
    const auto itLyr = FindLayerByWindow(hWnd);

    if (itLyr == gstLayerState.ltLayers.end())
        return TRUE;

    return InsertUiSnapWindowYToViewLine(ghViewWnd, &(itLyr->stGeometry), pstWpos);
}
//-------------------------------------------------------------------------------------------------

// ウィンドウのサイズ変更が完了したら送られてくる
VOID Lyb_OnWindowPosChanged(HWND hWnd, const LPWINDOWPOS pstWpos)
{
    const auto itLyr = FindLayerByWindow(hWnd);
    RECT rect;

    if (itLyr == gstLayerState.ltLayers.end())
        return;

    GetClientRect(hWnd, &rect);
    SendMessage(itLyr->hToolWnd, TB_AUTOSIZE, 0, 0); //    2024kai MoveWindowだとツールバー部分がちらつくためこれで対応
    SetWindowPos(itLyr->hTextWnd, HWND_TOP, 0, 0, rect.right, rect.bottom - gstLayerState.dToolBarHeight, SWP_NOMOVE | SWP_NOZORDER);
    InsertUiCaptureContentOffset(hWnd, gstLayerState.dToolBarHeight, &(itLyr->stGeometry.stContentOffset));

    //    移動がなかったときは何もしないでおｋ
    if (SWP_NOMOVE & pstWpos->flags)
        return;

    InsertUiUpdateOffsetFromWindowPos(ghViewWnd, &(itLyr->stGeometry), pstWpos);

    return;
}
//-------------------------------------------------------------------------------------------------

// 動かされているときに発生・マウスでウインドウドラッグ中とか
VOID Lyb_OnMoving(HWND hWnd, LPRECT pstPos)
{
    TCHAR atBuffer[SUB_STRING];
    INT dHideXdot;
    INT dTopLine;
    const auto itLyr = FindLayerByWindow(hWnd);

    if (itLyr == gstLayerState.ltLayers.end())
        return;

    dHideXdot = ViewCurrentOrigin().dHideXdot;
    dTopLine = ViewCurrentOrigin().dTopLine;

    InsertUiFormatStatusText(pstPos, &(itLyr->stGeometry), dHideXdot, dTopLine, TEXT("레이어"), atBuffer, SUB_STRING);
    MainStatusBarSetText(SB_LAYER, atBuffer);

    return;
}
//-------------------------------------------------------------------------------------------------

// マウスの左ボタンがダウンされたとき・ダブルクルック用・クラススタイルにCS_DBLCLKSを付けないとメッセージ来ない
VOID Lyb_OnLButtonDown(HWND hWnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags)
{
    INT sy, iDot, iLine;
    RECT rect;

    iDot = x; //    位置合わせ
    sy = y - gstLayerState.dToolBarHeight;
    if (0 > sy)
        sy = 0;
    iLine = sy / LINE_HEIGHT;

    TRACE(TEXT("マウスボタンダウン[%d][%dx%d(%d)]"), fDoubleClick, iDot, sy, iLine);

    if (!(fDoubleClick))
        return; //    ダブウクルックでないと用はない

    const auto itLyr = FindLayerByWindow(hWnd);
    if (itLyr == gstLayerState.ltLayers.end())
        return; //    ヒットしなかった・あり得ないはずだけど

    //    カーソルヒット位置が、連続空白の部分ならばマークを反転させる
    if (LayerTransparentAdjust(itLyr, iDot, iLine))
    {
        GetClientRect(hWnd, &rect);
        rect.top = (iLine * LINE_HEIGHT) + gstLayerState.dToolBarHeight;
        rect.bottom = rect.top + LINE_HEIGHT;
        InvalidateRect(hWnd, &rect, TRUE);
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// コンテキストメニュー呼びだしアクション(要は右クルック）
VOID Lyb_OnContextMenu(HWND hWnd, HWND hWndContext, UINT xPos, UINT yPos)
{
    INT posX, posY;
    HMENU hMenu, hSubMenu;
    UINT dRslt;

    posX = (SHORT)xPos; //    画面座標はマイナスもありうる
    posY = (SHORT)yPos;

    TRACE(TEXT("LAYER_WM_CONTEXTMENU %d x %d"), posX, posY);

    hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDM_LAYERBOX_POPUP));
    hSubMenu = GetSubMenu(hMenu, 0);
    MenuMnemonicApply(hSubMenu);

    dRslt = TrackPopupMenu(hSubMenu, 0, posX, posY, 0, hWnd, nullptr); //    TPM_CENTERALIGN | TPM_VCENTERALIGN |
    DestroyMenu(hMenu);

    return;
}
//-------------------------------------------------------------------------------------------------

// 透過合成エリアを全選択したり全解除したり
HRESULT LayerTransparentToggle(HWND hWnd, UINT bMode)
{
    TCHAR chb;
    INT_PTR iLines, iL;
    LETR_ITR itMozi;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        const auto itLyr = FindLayerByWindow(hWnd);
        if (itLyr == gstLayerState.ltLayers.end())
            return E_OUTOFMEMORY;

        TRACE(TEXT("공백 투과 선택 해제 %u"), bMode);

        //    行数確認
        iLines = itLyr->vcLyrImg.size();

        for (iL = 0; iLines > iL; iL++)
        {
            //    文字をイテレータで確保
            for (itMozi = itLyr->vcLyrImg.at(iL).vcLine.begin();
                 itMozi != itLyr->vcLyrImg.at(iL).vcLine.end(); itMozi++)
            {
                if (bMode)
                {
                    chb = itMozi->cchMozi;
                    if (iswspace(chb))
                    {
                        itMozi->mzStyle |= CT_LYR_TRNC;
                    }
                }
                else
                {
                    itMozi->mzStyle &= ~CT_LYR_TRNC;
                }
            }
            //    全部解除
        }

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
#endif

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 行数とドット値を受け取って、その場所の
INT LayerTransparentAdjust(LAYER_ITR itLyr, INT dNowDot, INT rdLine)
{
    INT_PTR i, iCount, iLines, iLetter;
    INT dDotCnt = 0, dPrvCnt = 0, rdWidth = 0;
    TCHAR ch, chb;
    LETR_ITR itMozi, itHead, itTail, itTemp;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        //    行のはみ出しを？
        iLines = itLyr->vcLyrImg.size();
        if (0 >= iLines)
            return 0;
        if (iLines <= rdLine)
            return 0;

        //    文字数確認
        iCount = itLyr->vcLyrImg.at(rdLine).vcLine.size();
        if (0 >= iCount)
            return 0;

        //    文字をイテレータで確保
        itMozi = itLyr->vcLyrImg.at(rdLine).vcLine.begin();

        for (i = 0, iLetter = 0; iCount > i; i++, iLetter++)
        {
            if (dNowDot <= dDotCnt)
            {
                break;
            }

            dPrvCnt = dDotCnt;
            rdWidth = itLyr->vcLyrImg.at(rdLine).vcLine.at(i).rdWidth;
            dDotCnt += rdWidth;
        } //    振り切るようなら末端

        if (iCount <= iLetter)
            return 0;

        if (1 <= iLetter) //    左文字で判定
        {
            iLetter--;
            itMozi += iLetter;
        }

        ch = itLyr->vcLyrImg.at(rdLine).vcLine.at(iLetter).cchMozi;
        //    該当箇所の文字を確認して
        if (!(iswspace(ch)))
            return 0;
        //    空白でないなら何もしないでおｋ

        //    その場所から頭方向に辿って、途切れ目を探す
        itHead = itLyr->vcLyrImg.at(rdLine).vcLine.begin();
        for (; itHead != itMozi; itMozi--)
        {
            chb = itMozi->cchMozi;
            if (!(iswspace(chb)))
            {
                itMozi++;
                break;
            }
        }
        if (itHead == itMozi) //    先頭を確認
        {
            chb = itMozi->cchMozi;
            if (!(iswspace(chb)))
            {
                itMozi++;
            }
        }
        //    非空白文字にヒットしたか、先頭位置である

        //    その場所から、同じグループの所まで確認
        itTail = itLyr->vcLyrImg.at(rdLine).vcLine.end();
        for (itTemp = itMozi; itTemp != itTail; itTemp++)
        {
            chb = itTemp->cchMozi; //    空白である間は
            if (!(iswspace(chb)))
            {
                break;
            }

            itTemp->mzStyle ^= CT_LYR_TRNC;
        }

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return ETC_MSG(err.what(), 0);
    }
    catch (...)
    {
        return ETC_MSG(("etc error"), 0);
    }
#endif

    return iLetter;
}
//-------------------------------------------------------------------------------------------------

// ビューが移動した
HRESULT LayerMoveFromView(HWND hWnd, UINT state)
{
    LAYER_ITR itLyr;

    for (itLyr = gstLayerState.ltLayers.begin(); itLyr != gstLayerState.ltLayers.end(); itLyr++)
    {
        InsertUiMoveFromView(ghViewWnd, itLyr->hBoxWnd, state, &(itLyr->stGeometry));
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 行の内容を文字列で確保・ポインタ開放は呼んだほうでやる
LPTSTR LayerLineTextGetAlloc(LAYER_ITR itLyr, INT il)
{
    UINT_PTR cchSize, i = 0;
    LPTSTR ptText;

    cchSize = itLyr->vcLyrImg.at(il).vcLine.size();
    if (0 >= cchSize)
        return nullptr;

    ptText = (LPTSTR)malloc((cchSize + 1) * sizeof(TCHAR)); //    ぬるたーみねーた分増やす
    ZeroMemory(ptText, (cchSize + 1) * sizeof(TCHAR));

    for (i = 0; cchSize > i; i++)
    {
        ptText[i] = itLyr->vcLyrImg.at(il).vcLine.at(i).cchMozi;
    }

    return ptText;
}
//-------------------------------------------------------------------------------------------------

// 対象のレイヤボックスの保持してる文字列を破壊する
HRESULT LayerStringObliterate(LAYER_ITR itLyr)
{
    UINT_PTR j, iLine;

    iLine = itLyr->vcLyrImg.size();
    for (j = 0; iLine > j; j++)
    {
        itLyr->vcLyrImg.at(j).vcLine.clear(); //    各行の中身全消し
    }
    itLyr->vcLyrImg.clear(); //    行を全消し

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスの中身の編集をON/OFF
HRESULT LayerEditOnOff(HWND hWnd, UINT dStyle)
{
    UINT_PTR i, iLines;
    INT ndx;
    UINT cchSize;
    LPTSTR ptStr;
    basic_string<TCHAR> srEditText;
    const auto itLyr = FindLayerByWindow(hWnd);

    if (itLyr != gstLayerState.ltLayers.end())
    {
        if (dStyle) //    編集する
        {
            iLines = itLyr->vcLyrImg.size();
            srEditText.clear();
            for (i = 0; iLines > i; i++)
            {
                srEditText.reserve(srEditText.size() + itLyr->vcLyrImg.at(i).vcLine.size() + 2);
                if (0 != i)
                {
                    srEditText += CH_CRLFW;
                }

                for (const auto &letter : itLyr->vcLyrImg.at(i).vcLine)
                {
                    srEditText.push_back(letter.cchMozi);
                }
            }

            Edit_SetText(itLyr->hTextWnd, srEditText.c_str());

            SetFocus(itLyr->hTextWnd);

            ShowWindow(itLyr->hTextWnd, SW_SHOW);
        }
        else //    終了
        {
            ndx = Edit_GetTextLength(itLyr->hTextWnd);
            ndx += 2; //    ぬるたみねた分
            ptStr = (LPTSTR)malloc(ndx * sizeof(TCHAR));
            ZeroMemory(ptStr, ndx * sizeof(TCHAR));
            Edit_GetText(itLyr->hTextWnd, ptStr, ndx);
            ShowWindow(itLyr->hTextWnd, SW_HIDE);

            StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);

            LayerStringObliterate(itLyr);
            AppendEmptyLayerLine(*itLyr);

            LayerBoxSetString(itLyr, ptStr, cchSize, nullptr, 0x00);

            FREE(ptStr);

            InvalidateRect(hWnd, nullptr, TRUE);
        }
    }

    UpdateWindow(hWnd);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスの内容を入れ替える・外部から？
HRESULT LayerStringReplace(HWND hLyrWnd, LPTSTR ptStr)
{
    UINT cchSize;
    const auto itLyr = FindLayerByWindow(hLyrWnd);

    if (itLyr != gstLayerState.ltLayers.end())
    {
        StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);

        LayerStringObliterate(itLyr);
        AppendEmptyLayerLine(*itLyr);

        LayerBoxSetString(itLyr, ptStr, cchSize, nullptr, 0x00);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 文字列を表示内容にする
HRESULT LayerFromString(LAYER_ITR itLyr, LPCTSTR ptStr)
{
    UINT cchSize;

    AppendEmptyLayerLine(*itLyr);

    StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);

    LayerBoxSetString(itLyr, ptStr, cchSize, nullptr, 0x01);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 表示内容を選択範囲から頂戴する
HRESULT LayerFromSelectArea(LAYER_ITR itLyr, UINT bSqSel)
{
    LPTSTR ptString = nullptr;
    UINT cchSize;
    LPPOINT pstPos;

    TRACE(TEXT("선택 범위에서 가져오기"));
#ifdef DO_TRY_CATCH
    try
    {
#endif

        AppendEmptyLayerLine(*itLyr);

        DocSelectTextGetAlloc(D_UNI | bSqSel, (LPVOID *)(&ptString), &pstPos);

        StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSize);

        LayerBoxSetString(itLyr, ptString, cchSize, (bSqSel & D_SQUARE) ? pstPos : nullptr, 0x01);

        FREE(ptString);
        FREE(pstPos);

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
#endif
    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 表示内容をくるっぷぼーどから頂戴する
HRESULT LayerFromClipboard(LAYER_ITR itLyr)
{
    LPTSTR ptString = nullptr;
    UINT cchSize, dStyle; //, i;

    AppendEmptyLayerLine(*itLyr);

    //    くるっぺぼーどからの場合は、矩形でも関係ない
    ptString = DocClipboardDataGet(&dStyle);

    StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSize);

    LayerBoxSetString(itLyr, ptString, cchSize, nullptr, 0x01);

    FREE(ptString);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ボックス内容に合わせてサイズ広げる
HRESULT LayerBoxSizeAdjust(LAYER_ITR itLyr)
{
    INT dViewXdot, dYline, dViewYdot;
    INT iMaxDot = 0, iYdot;
    INT_PTR iLine, i;
    SIZE wdSize, tgtSize; // clSize

#ifdef DO_TRY_CATCH
    try
    {
#endif
        //    今の画面の行数とドット数確認
        dYline = ViewAreaSizeGet(&dViewXdot);
        dViewYdot = dYline * LINE_HEIGHT;

        //    使っている内容からサイズを確認
        iLine = itLyr->vcLyrImg.size();
        iYdot = iLine * LINE_HEIGHT;
        for (i = 0; iLine > i; i++) //    最大ドット数を確認
        {
            if (iMaxDot < itLyr->vcLyrImg.at(i).iDotCnt)
            {
                iMaxDot = itLyr->vcLyrImg.at(i).iDotCnt;
            }
        }
        //    多分ウインドウサイズになるはず
        wdSize.cx = itLyr->stGeometry.stContentOffset.x + iMaxDot + itLyr->stGeometry.stContentOffset.x;
        wdSize.cy = itLyr->stGeometry.stContentOffset.y + iYdot + itLyr->stGeometry.stContentOffset.x;

        if (kLayerBoxWidth > wdSize.cx)
        {
            tgtSize.cx = kLayerBoxWidth;
        }
        else if (dViewXdot < wdSize.cx)
        {
            tgtSize.cx = dViewXdot;
        }
        else
        {
            tgtSize.cx = wdSize.cx;
        }

        if (kLayerBoxHeight > wdSize.cy)
        {
            tgtSize.cy = kLayerBoxHeight;
        }
        else if (dViewYdot < wdSize.cy)
        {
            tgtSize.cy = dViewYdot;
        }
        else
        {
            tgtSize.cy = wdSize.cy;
        }

    SetWindowPos(itLyr->hBoxWnd, HWND_TOPMOST, 0, 0, tgtSize.cx, tgtSize.cy, SWP_NOMOVE | SWP_NOZORDER);

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
#endif

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスに文字列を記録する
HRESULT LayerBoxSetString(LAYER_ITR itLyr, LPCTSTR ptText, UINT cchSize, LPPOINT pstPt, UINT bStyle)
{
    UINT_PTR i, j, iLine, iTexts;
    LONG dMin = 0;
    INT insDot, yLine, dSpDot, dSpMozi, iLines = 0, dOffset;
    LPTSTR ptBuff, ptSpace = nullptr;
    ONELINE stLine;

#ifdef DO_TRY_CATCH
    try
    {
#endif
        ZeroONELINE(&stLine);

        // 2024kai GetHdcCでhdcを一時共通にして高速化（0.9秒かかるAAのレイヤボックス表示が0.3秒に）
        GetHdcC();
        //    オフセット設定が有る場合、その分を埋める空白が必要
        if (pstPt) //    最小オフセット値を探して、そこを左端にする
        {
            dMin = pstPt[0].x;

            yLine = 0;
            for (i = 0; cchSize > i; i++)
            {
                if (CC_CR == ptText[i] && CC_LF == ptText[i + 1]) //    改行であったら
                {
                    //    オフセット最小をさがす
                    if (dMin > pstPt[yLine].x)
                    {
                        dMin = pstPt[yLine].x;
                    }

                    i++;     //    0x0D,0x0Aだから、壱文字飛ばすのがポイント
                    yLine++; //    改行したからFocusは次の行へ
                }
            }
            //    この時点で、yLineは行数になってる
            iLines = yLine;

            //    壱行目の空白を作って閃光入力しておく
            insDot = 0;
            dOffset = pstPt[0].x - dMin;
            ptSpace = DocPaddingSpaceUni(dOffset, nullptr, nullptr, nullptr);
            //    前方空白は無視されるのでユニコード使って問題無い
            StringCchLength(ptSpace, STRSAFE_MAX_CCH, &iTexts);
            for (j = 0; iTexts > j; j++)
            {
                insDot += LayerInputLetter(itLyr, insDot, 0, ptSpace[j]);
            }
            FREE(ptSpace);
        }

        yLine = 0;
        insDot = 0;
        for (i = 0; cchSize > i; i++)
        {
            if (CC_CR == ptText[i] && CC_LF == ptText[i + 1]) //    改行であったら
            {
                itLyr->vcLyrImg.push_back(stLine); //    次の行を作る

                i++;        //    0x0D,0x0Aだから、壱文字飛ばすのがポイント
                yLine++;    //    改行したからFocusは次の行へ
                insDot = 0; //    そして行の先頭である

                //    オフセット分の空白を作る
                if (pstPt && (iLines > yLine))
                {
                    dOffset = pstPt[yLine].x - dMin;
                    ptSpace = DocPaddingSpaceUni(dOffset, nullptr, nullptr, nullptr);
                    //    前方空白は無視されるのでユニコード使って問題無い
                    StringCchLength(ptSpace, STRSAFE_MAX_CCH, &iTexts);
                    for (j = 0; iTexts > j; j++)
                    {
                        insDot += LayerInputLetter(itLyr, insDot, yLine, ptSpace[j]);
                    }
                    FREE(ptSpace);
                }
            }
            else if (CC_TAB == ptText[i])
            {
                //    タブは挿入しない
            }
            else
            {
                insDot += LayerInputLetter(itLyr, insDot, yLine, ptText[i]);
            }
        }
        ReleaseHdcC();

        //    末尾整形と前方空白確認
        iLine = itLyr->vcLyrImg.size();
        for (i = 0; iLine > i; i++)
        {
            //    末端空白削除
            ptBuff = DocLastSpDel(&(itLyr->vcLyrImg.at(i).vcLine));
            FREE(ptBuff); //    使わないが、受けて開放しないとイケない

            //    先頭空白確認
            dSpMozi = 0;
            dSpDot = LayerHeadSpaceCheck(&(itLyr->vcLyrImg.at(i).vcLine), &dSpMozi);

            itLyr->vcLyrImg.at(i).dFrtSpDot = dSpDot;
            itLyr->vcLyrImg.at(i).dFrtSpMozi = dSpMozi;
        }

        //    サイズ調整
        if (bStyle)
        {
            LayerBoxSizeAdjust(itLyr);
        }

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
#endif

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 행두 공백 확인. 피리어드는 줄 맨 앞에서 연속될 때만 공백처럼 본다
INT LayerHeadSpaceCheck(vector<LETTER> *vcTgLine, PINT pdMozi)
{
    TCHAR ch;
    INT cchSp, dDot; //    文字数とドット数
    UINT_PTR i, iMozi;
    BOOLEAN bDotAtLineHead;

#ifdef DO_TRY_CATCH
    try
    {
#endif
        iMozi = vcTgLine->size();

        dDot = 0;
        cchSp = 0;
        bDotAtLineHead = TRUE;
        for (i = 0; iMozi > i; i++)
        {
            ch = vcTgLine->at(i).cchMozi;

            if (iswspace(ch))
            {
                dDot += vcTgLine->at(i).rdWidth; //    ドット数
                cchSp++;                         //    文字数
                bDotAtLineHead = FALSE;
                continue;
            }

            // 줄 시작부터 붙어 있는 피리어드만 공백 취급한다.
            if (TEXT('.') == ch && bDotAtLineHead)
            {
                dDot += vcTgLine->at(i).rdWidth; //    ドット数
                cchSp++;                         //    文字数
                continue;
            }

            //    字がスペースでも対象のピリオドでもないなら、余白はそこまで
            if (!(iswspace(ch)) && TEXT('.') != ch)
            {
                if (pdMozi)
                {
                    *pdMozi = cchSp;
                }
                return dDot;
            }

            if (pdMozi)
            {
                *pdMozi = cchSp;
            }
            return dDot;
        }

        if (pdMozi)
        {
            *pdMozi = cchSp;
        }
#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return ETC_MSG(err.what(), 0);
    }
    catch (...)
    {
        return ETC_MSG(("etc error"), 0);
    }
#endif

    return dDot;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)に壱文字追加する
INT LayerInputLetter(LAYER_ITR itLyr, INT nowDot, INT rdLine, TCHAR ch)
{
    LETTER stLetter;

#ifdef DO_TRY_CATCH
    try
    {
#endif
        //    データ作成
        DocLetterDataCheck(&stLetter, ch); //    指定行のドット位置(キャレット位置)に壱文字追加する・レイヤボックス

        itLyr->vcLyrImg.at(rdLine).vcLine.push_back(stLetter);

        itLyr->vcLyrImg.at(rdLine).iDotCnt += stLetter.rdWidth;
        itLyr->vcLyrImg.at(rdLine).iByteSz += stLetter.mzByte;

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return ETC_MSG(err.what(), 0);
    }
    catch (...)
    {
        return ETC_MSG(("etc error"), 0);
    }
#endif

    return stLetter.rdWidth;
}
//-------------------------------------------------------------------------------------------------

// 該当エリアに上書きしたり挿入したり
HRESULT LayerContentsImportable(HWND hWnd, UINT cmdID, LPINT pXdot, LPINT pYline, UINT dStyle)
{
    RECT vwRect, lyRect;
    POINT conPoint;
    INT xTgDot, xDot, iSrcDot, iSabun, iDivid, iSpDot;
    INT dGap, dInLen, dInBgn, dInEnd, dEndot;
    INT dLeft, dRight;
    INT iPageLine, yTgLine, dWkLine, dLyLine;
    INT iMinus, iMozi, iStMozi, iEdMozi;
    INT dBkLeft, dBkRight, dBkStMozi, dBkEdMozi;
    INT_PTR dNeedLine;
    UINT_PTR cchSize;
    LPTSTR ptStr, ptBuffer;
    BOOLEAN bFirst = TRUE; //    なんか処理したらFALSE
    BOOLEAN bSpace, bBkSpase;

#ifdef EDGE_BLANK_STYLE
    INT bEdgeBlank;
    INT xDotEx, iMoziEx;
#endif
    LETR_ITR itLtr, itDel;
    wstring wsBuff;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        const auto itLyr = FindLayerByWindow(hWnd);
        if (itLyr == gstLayerState.ltLayers.end())
            return E_OUTOFMEMORY;

        //    まず場所を確認
        GetWindowRect(ghViewWnd, &vwRect);
        vwRect.left += LINENUM_WID;
        vwRect.top += RULER_AREA;

        GetWindowRect(itLyr->hBoxWnd, &lyRect);
        conPoint.x = lyRect.left + itLyr->stGeometry.stContentOffset.x;
        conPoint.y = lyRect.top + itLyr->stGeometry.stContentOffset.y;
        //    左や上にはみ出してたら、ここはマイナスになっている
        xTgDot = conPoint.x - vwRect.left;
        yTgLine = conPoint.y - vwRect.top;

        yTgLine /= LINE_HEIGHT;

        //    20110704    この時点では、スクロールによるズレが考慮されてない
        xTgDot += ViewCurrentOrigin().dHideXdot;
        yTgLine += ViewCurrentOrigin().dTopLine;
        //    多分これで大丈夫

        xDot = xTgDot;

        TRACE(TEXT("LAYER IMPORT[%d:%d]"), xTgDot, yTgLine);

        if (pXdot)
            *pXdot = xTgDot;
        if (pYline)
            *pYline = yTgLine;

        //    使う行数確認
        dNeedLine = itLyr->vcLyrImg.size();
        //    最終行の空白確認
        ptStr = LayerLineTextGetAlloc(itLyr, dNeedLine - 1);
        if (!(ptStr))
            dNeedLine--; //    最後空白なら使わない
        FREE(ptStr);

        iPageLine = DocPageParamGet(nullptr, nullptr); //    この頁の行数確認・入れ替えていけるか

        //    全体行数より、追加行数が多かったら、改行増やす
        if (iPageLine < (dNeedLine + yTgLine))
        {
            iMinus = (dNeedLine + yTgLine) - iPageLine; //    追加する行数
            DocAdditionalLine(iMinus, &bFirst);         //    bFirst = FALSE;
            TRACE(TEXT("ADD LINE[%d]"), iMinus);
        }

        //    白ヌキするには、前後の空白文字量を増やせばいい
        //    透過領域が狭い場合は、非透過とする・先にスキャンするか。
        bEdgeBlank = ComboBox_GetCurSel(GetDlgItem(GetDlgItem(hWnd, IDW_LYB_TOOL_BAR), IDCB_LAYER_EDGE_BLANK));
        if (1 == bEdgeBlank)
        {
            LayerEdgeBlankSizeCheck(hWnd, kEdgeBlankNarrow);
        }
        else if (2 == bEdgeBlank)
        {
            LayerEdgeBlankSizeCheck(hWnd, kEdgeBlankWide);
        }

        // 白ヌキするには狭い透過領域を消す

        for (dWkLine = yTgLine, dLyLine = 0; (yTgLine + dNeedLine) > dWkLine; dWkLine++, dLyLine++)
        {
            if (0 > dWkLine)
                continue; //    上にめり込んでるのは処理しちゃいかん

            TRACE(TEXT("Check Line V[%d] L[%d]"), dWkLine, dLyLine);

            //    挿入内容の位置の確認・ここで、各部分毎にばらせばいい。
            //    行単位ではなく、透過領域で区切られた文字領域毎に判定する

            //    dLyLine：レイヤ内の行番号　dWkLine：ビューの行番号
            iSpDot = itLyr->vcLyrImg.at(dLyLine).dFrtSpDot; //    レイヤ内ドットオフセット
            //    行頭から、初めて非空白が出てくるドット

            xDot = xTgDot + iSpDot; //    レイヤ内オブジェクトを挿入位置
            //    マイナスだった場合レイヤ内文字列の開始位置をずらす

            //    必要な所を抽出・使用バイト数も確認しておく
            itLtr = itLyr->vcLyrImg.at(dLyLine).vcLine.begin();
            itLtr += itLyr->vcLyrImg.at(dLyLine).dFrtSpMozi; //    空白以外の開始位置
            // ここで開始位置までずらしている

            //    壱行ずつ状況をみながら挿入していく
            while (itLtr != itLyr->vcLyrImg.at(dLyLine).vcLine.end())
            {
                while (0 > xDot) //    マイナスだったら＋になるまでずらしていく
                {
                    //    該当行の末尾までイッたら終了
                    if (itLtr == itLyr->vcLyrImg.at(dLyLine).vcLine.end())
                        break;

                    // 透過フラグがあれば終了
                    if (itLtr->mzStyle & CT_LYR_TRNC)
                        break;

                    xDot += itLtr->rdWidth;
                    iSpDot += itLtr->rdWidth;

                    itLtr++;
                }

                //    挿入内容の確保
                wsBuff.clear();
                dInLen = 0;
                for (; itLtr != itLyr->vcLyrImg.at(dLyLine).vcLine.end(); itLtr++)
                {
                    // 透過フラグがあれば終了
                    if (itLtr->mzStyle & CT_LYR_TRNC)
                        break;

                    wsBuff += itLtr->cchMozi;
                    dInLen += itLtr->rdWidth;
                } //    そこから終わりまで

                if (0 != dInLen) //    挿入できる内容がなかったらなにもせんでいい
                {
                    cchSize = wsBuff.size() + 1; //    dInLen：挿入内容のドット幅
                    ptStr = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
                    StringCchCopy(ptStr, cchSize, wsBuff.c_str());

                    dGap = 0;

                    //    挿入位置の調整
                    iSrcDot = DocLineParamGet(dWkLine, nullptr, nullptr); //    挿入行の末端ドット位置
                    iSabun = xTgDot - iSrcDot;                            //    ＋なら足りてない
                    iDivid = iSabun + iSpDot;                             //    レイヤ内も考慮
                    if (0 < iDivid)                                       //    行末端より後にきてる
                    {
                        xDot = iSrcDot;
                        ptBuffer = DocPaddingSpaceWithPeriod(iDivid, nullptr, nullptr, nullptr, TRUE);
                        //    行末端からレイヤ内オブジェクトまでを埋める空白
                        if (ptBuffer)
                        {
                            DocInsertString(&xDot, &dWkLine, nullptr, ptBuffer, dStyle, bFirst);
                            bFirst = FALSE;
                            FREE(ptBuffer);
                        }

                        // 余裕があるなら、白ヌキはあまり関係ないか？
                    }
                    else if (0 > iDivid) //    既存の文字列のほうが長い場合
                    {
                        //    その地点の状況を確認して、空白エリアなら埋めに使う
                        //    文字エリアなら、直近からパディングできるところまでを埋め直す
                        iMozi = DocLetterPosGetAdjust(&xDot, dWkLine, -1); //    今の文字位置を確認
                                                                           //    iMozi：挿入位置文字数            xDot：文字列挿入位置ドット

#ifndef EDGE_BLANK_STYLE
                        //    そこの文字が空白か、空白ならどこまで続いてるか確認
                        DocLineStateCheckWithDot(xDot, dWkLine, &dLeft, &dRight, &iStMozi, nullptr, &bSpace);
                        //    dRight 使ってない
#endif
                        //    先に上書きエリアの処理しないと、パディング直したらずれる
                        //    上書きの場合ここから先をさらに削除してギャップパディング
                        if (IDM_LYB_OVERRIDE == cmdID)
                        {
                            dInBgn = xTgDot + iSpDot; //    ボックス左端＋内部オフセット＝文字列開始位置
                            dInEnd = dInBgn + dInLen; //    開始位置＋文字列幅＝文字列終端位置

                            //    20110817
                            dEndot = dInEnd;
#ifdef EDGE_BLANK_STYLE
                            //    ここで dEndot をオフセットする？
                            if (1 == bEdgeBlank)
                            {
                                dEndot += kEdgeBlankNarrow;
                            }
                            else if (2 == bEdgeBlank)
                            {
                                dEndot += kEdgeBlankWide;
                            }
#endif
                            iEdMozi = DocLetterPosGetAdjust(&dEndot, dWkLine, 1); //    上書きされる領域
                            //    キャレット位置修正

                            //    後半の、あまってるSpaceも考慮してパディング調整
                            DocLineStateCheckWithDot(dEndot, dWkLine, &dBkLeft, &dBkRight, &dBkStMozi, &dBkEdMozi, &bBkSpase);
                            if (bBkSpase) //    後半の空白再編
                            {
                                dEndot = dBkRight;
                                iEdMozi = DocLetterPosGetAdjust(&dEndot, dWkLine, 1); //    上書きされる領域
                                // ↑は要らないかもだ
                                dGap = dBkRight - dInEnd; //    元の部分の維持のためのギャップ量
                                //    dBkRight：空白位置末端　dInEnd：挿入文字列リアル末端
                            }
                            else
                            {
                                dGap = dEndot - dInEnd; //    元の部分の維持のためのギャップ量
                                //    dEndot：挿入文字列キャレット末端　dInEnd：挿入文字列リアル末端
                            }

                            //    該当部分を削除
                            DocRangeDeleteByMozi(xDot, dWkLine, iMozi, iEdMozi, &bFirst);

                            if (0 < dGap)
                            {
                                dInBgn = xDot;
                                //    レイヤ内オブジェクト末端から元絵の開始位置までを埋める数ドットの空白
                                ptBuffer = DocPaddingSpaceWithPeriod(dGap, nullptr, nullptr, nullptr, TRUE);
                                if (ptBuffer)
                                {
                                    DocInsertString(&dInBgn, &dWkLine, nullptr, ptBuffer, dStyle, bFirst);
                                    bFirst = FALSE;
                                    FREE(ptBuffer);
                                }
                            }
                        }

#ifdef EDGE_BLANK_STYLE
                        if (bEdgeBlank)
                        {
                            //    オフセット位置確認
                            xDotEx = (xTgDot + iSpDot);

                            if (1 == bEdgeBlank)
                            {
                                xDotEx -= kEdgeBlankNarrow;
                            }
                            else if (2 == bEdgeBlank)
                            {
                                xDotEx -= kEdgeBlankWide;
                            }
                            else
                            {
                                ;
                            }

                            if (0 > xDotEx)
                            {
                                xDotEx = 0;
                            }

                            iMoziEx = DocLetterPosGetAdjust(&xDotEx, dWkLine, -1); //    今の文字位置を確認
                                                                                   //    iMoziEx：挿入位置文字数            xDotEx：挿入位置オフセット

                            //    そこの文字が空白か、空白ならどこまで続いてるか確認
                            DocLineStateCheckWithDot(xDotEx, dWkLine, &dLeft, &dRight, &iStMozi, nullptr, &bSpace);

                            dGap = (xTgDot + iSpDot) - xDotEx; //    前側の埋め処理
                            xDot = xDotEx;
                        }
                        else
                        {
                            //    そこの文字が空白か、空白ならどこまで続いてるか確認
                            DocLineStateCheckWithDot(xDot, dWkLine, &dLeft, &dRight, &iStMozi, nullptr, &bSpace);
                            iMoziEx = iStMozi; //    特に意味はない
#endif
                            dGap = (xTgDot + iSpDot) - xDot; //    前側の埋め処理
#ifdef EDGE_BLANK_STYLE
                        }
#endif
                        if (bSpace) //    空白なら、ギャップ分と合わせて入れ直す
                        {
                            dGap += (xDot - dLeft); //    パディングドット数
                            //    既存の空白を一旦削除して
                            DocRangeDeleteByMozi(dLeft, dWkLine, iStMozi, iMozi, &bFirst);
                        }
                        else //    文字であるなら、なにもしない
                        {
#ifdef EDGE_BLANK_STYLE
                            //    必要なら、既存の文字列を一旦削除して
                            if (bEdgeBlank)
                                DocRangeDeleteByMozi(xDot, dWkLine, iMoziEx, iMozi, &bFirst);
#endif
                            dLeft = xDot; //    ギャップ開始位置
                        }

                        ptBuffer = DocPaddingSpaceWithPeriod(dGap, nullptr, nullptr, nullptr, TRUE);
                        if (ptBuffer)
                        {
                            DocInsertString(&dLeft, &dWkLine, nullptr, ptBuffer, dStyle, bFirst);
                            bFirst = FALSE;
                            FREE(ptBuffer);
                        }

                        xDot = dLeft; //    挿入位置であるように
                    }
                    else //    末端位置にピタリである
                    {
                        //    特にすることない？
                    }

                    //    該当の場所へ文字列挿入
                    DocInsertString(&xDot, &dWkLine, nullptr, ptStr, dStyle, bFirst);
                    bFirst = FALSE;
                    //    xDotには挿入終端ドットが入る

                    FREE(ptStr);
                }

                //    この時点で、itLtrはendか透過開始位置である・xDotは透過開始ドットである
                if (itLtr == itLyr->vcLyrImg.at(dLyLine).vcLine.end())
                    break;
                //    終わってたらこの行の処理終わり

                //    透過しないところまで進める
                for (; itLtr != itLyr->vcLyrImg.at(dLyLine).vcLine.end(); itLtr++)
                {
                    // 透過フラグが無くなれば終了
                    if (!(itLtr->mzStyle & CT_LYR_TRNC))
                        break;
                    xDot += itLtr->rdWidth;
                } //    そこから終わりまで

                iSpDot = xDot; //    挿入開始位置を調整
                iSpDot -= xTgDot;
            }
        }

        TRACE(TEXT("Layer Insert OK！"));
#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
#endif

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスの内容をクルップする
HRESULT LayerForClipboard(HWND hWnd, UINT bStyle)
{
    INT_PTR iLines, iL, cchSize, cbSize;
    LETR_ITR itMozi;

    string srString;
    wstring wsString;

    const auto itLyr = FindLayerByWindow(hWnd);
    if (itLyr == gstLayerState.ltLayers.end())
        return E_OUTOFMEMORY;

    iLines = itLyr->vcLyrImg.size(); //    行数確認

    srString.clear();
    wsString.clear();

    for (iL = 0; iLines > iL; iL++)
    {
        //    文字をイテレータで確保
        for (itMozi = itLyr->vcLyrImg.at(iL).vcLine.begin(); itMozi != itLyr->vcLyrImg.at(iL).vcLine.end(); itMozi++)
        {
            srString += string(itMozi->acNoSjisText);
            wsString += itMozi->cchMozi;
        }

        srString += string(CH_CRLFA);
        wsString += wstring(CH_CRLFW);
    }

    if (bStyle & D_UNI) //    ユニコードである
    {
        cchSize = static_cast<INT_PTR>(wsString.size()) + 1;
        DocClipboardDataSet((LPTSTR)(wsString.c_str()), static_cast<INT>(cchSize * sizeof(TCHAR)), bStyle);
    }
    else
    {
        cbSize = static_cast<INT_PTR>(srString.size()) + 1;
        DocClipboardDataSet((LPSTR)(srString.c_str()), static_cast<INT>(cbSize), bStyle);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// レイヤボックスの内容を削除する
HRESULT LayerOnDelete(HWND hWnd)
{
#ifdef DO_TRY_CATCH
    try
    {
#endif

        const auto itLyr = FindLayerByWindow(hWnd);
        if (itLyr == gstLayerState.ltLayers.end())
            return E_OUTOFMEMORY;

        LayerStringObliterate(itLyr); //    中身破壊
        AppendEmptyLayerLine(*itLyr); //    空データを作成しておく

        InvalidateRect(hWnd, nullptr, TRUE);

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
#endif

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

#ifdef EDGE_BLANK_STYLE

// 白抜き指定したときに、幅の狭い透過領域をキャンセルする
HRESULT LayerEdgeBlankSizeCheck(HWND hWnd, INT iCanWid)
{
    INT_PTR iLines;
    INT iWidth;

    LYLINE_ITR itLine;
    LETR_ITR itMozi, itMzx;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        const auto itLyr = FindLayerByWindow(hWnd);
        if (itLyr == gstLayerState.ltLayers.end())
            return E_OUTOFMEMORY;

        iLines = itLyr->vcLyrImg.size(); //    行数確認

        //    壱行ずつ見ていく
        for (itLine = itLyr->vcLyrImg.begin(); itLine != itLyr->vcLyrImg.end(); itLine++)
        {
            //    文字をイテレータで確保
            for (itMozi = itLine->vcLine.begin(); itMozi != itLine->vcLine.end(); itMozi++)
            {
                if (itMozi->mzStyle & CT_LYR_TRNC) //    透過領域にヒットしたら
                {
                    //    その領域の幅をゲッツする
                    iWidth = 0;
                    for (itMzx = itMozi; itMzx != itLine->vcLine.end(); itMzx++)
                    {
                        if (!(itMzx->mzStyle & CT_LYR_TRNC))
                            break; //    外れたら終わり
                        iWidth += itMzx->rdWidth;
                    }

                    if (iCanWid >= iWidth) //    もしちっちゃいなら
                    {
                        for (; itMzx != itMozi; itMozi++)
                        {
                            itMozi->mzStyle &= ~CT_LYR_TRNC;
                        }
                    }
                    else
                    {
                        itMozi = itMzx;
                    }
                    itMozi--; //    ループ先頭でインクリするため、一旦戻る
                }
            }
        }

        InvalidateRect(hWnd, nullptr, TRUE); //    再描画

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }
#endif
    return S_OK;
}
//-------------------------------------------------------------------------------------------------

#endif
