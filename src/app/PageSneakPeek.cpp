// 페이지 미리보기
#include "Sunday.h"

#ifdef USE_HOVERTIP

#define HOVER_TIPS_CLASS TEXT("HOVER_TIPS")

#define HOVER_TIME 15000 // デフォルト表示ｍｓ
#define HOVER_TMID 1234  // タイマＩＤ
#define HOVER_MOVE 8     // これ以上マウスムーブが入ったら消す閾値

#define HOVER_DELAY 1000 // マウス停止からの出現待ち時間
//-------------------------------------------------------------------------------------------------
static HWND ghTipWnd;   // ホバーチップのウインドウハンドル
static HFONT ghTipFont; // ツールチップ用

static UINT gdMoveVol; // 移動量カウント

static LPTSTR gptContent;   // 表示内容
static RECT gstContSize;    // 表示大きさ
static RECT gstContSizeRaw; // 16px 기준 원본 표시 크기
//-------------------------------------------------------------------------------------------------

VOID HoverTipClose(HWND);

LRESULT CALLBACK HoverTipProc(HWND, UINT, WPARAM, LPARAM);
VOID Htp_OnPaint(HWND);
VOID htp_OnTimer(HWND, UINT);
VOID Htp_OnKillFocus(HWND, HWND);
VOID Htp_OnLButtonUp(HWND, INT, INT, UINT);
VOID Htp_OnMButtonUp(HWND, INT, INT, UINT);
VOID Htp_OnRButtonUp(HWND, INT, INT, UINT);
VOID Htp_OnMouseMove(HWND, INT, INT, UINT);
//-------------------------------------------------------------------------------------------------

HRESULT HoverTipFontRefresh(VOID)
{
    LOGFONT stFont;
    HFONT hNewFont;

    if (ghTipWnd)
        HoverTipClose(ghTipWnd);

    ViewingFontGet(&stFont);

    hNewFont = CreateFontIndirect(&stFont);
    if (!hNewFont)
        return E_OUTOFMEMORY;

    if (ghTipFont)
        DeleteFont(ghTipFont);
    ghTipFont = hNewFont;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 初期化
HRESULT HoverTipInitialise(HINSTANCE hInstance, HWND hPtWnd)
{
    WNDCLASSEX wcex;

    if (hInstance)
    {
        gptContent = nullptr;
        gdMoveVol = 0;

        //    表示チップウインドウクラス作成
        ZeroMemory(&wcex, sizeof(WNDCLASSEX));
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = HoverTipProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = nullptr;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1); //    ツールチップコントロールの背景色
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = HOVER_TIPS_CLASS;
        wcex.hIconSm = nullptr;

        RegisterClassEx(&wcex);

        //    表示チップウインドウ作成 | WS_EX_TOPMOST
        ghTipWnd = CreateWindowEx(WS_EX_TOOLWINDOW, HOVER_TIPS_CLASS, TEXT("InfoTip"), WS_POPUP | WS_BORDER, 0, 0, 15, 15, nullptr, nullptr, hInstance, nullptr);
        //    最初は非表示

        //    페이지 미리보기 표시는 AA 폰트를 그대로 사용한다
        HoverTipFontRefresh();
    }
    else
    {
        DeleteFont(ghTipFont);
        ghTipFont = nullptr;

        DestroyWindow(ghTipWnd);
        ghTipWnd = nullptr;

        FREE(gptContent);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// MouseHoverの登録する
HRESULT HoverTipResist(HWND hTgtWnd)
{
    //    Hover固定・LEAVEは使わないか

    TRACKMOUSEEVENT stTrackMsEv;

    ZeroMemory(&stTrackMsEv, sizeof(TRACKMOUSEEVENT));
    stTrackMsEv.cbSize = sizeof(TRACKMOUSEEVENT);
    stTrackMsEv.dwFlags = TME_HOVER | TME_LEAVE;
    stTrackMsEv.hwndTrack = hTgtWnd;
    stTrackMsEv.dwHoverTime = HOVER_DELAY; //    時間、そのうち調整出来るように    HOVER_DEFAULT
    TrackMouseEvent(&stTrackMsEv);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// WM_MOUSEHOVERを受け取る
LRESULT HoverTipOnMouseHover(HWND hEvWnd, WPARAM wParam, LPARAM lParam, HOVERTIPDISP pfInfoGet)
{
    INT x, y;
    UINT keyFlags;
    LPTSTR ptText;

    HDC hdc;
    HFONT hOldFnt;

    RECT deskRect;
    POINT point;
    INT xSub, ySub;

    keyFlags = (UINT)wParam;

    x = (INT)(SHORT)LOWORD(lParam);
    y = (INT)(SHORT)HIWORD(lParam);

    point.x = x;
    point.y = y;
    ClientToScreen(hEvWnd, &point);

    FREE(gptContent);

    ptText = pfInfoGet(nullptr);
    if (!(ptText)) //    なんかおかしい
    {
        return 0;
    }

    gptContent = ptText;

    hdc = GetDC(ghTipWnd);

    SetRect(&gstContSizeRaw, 0, 0, 2222, 200); //    16px 기준 원본 크기를 먼저 잰다

    hOldFnt = SelectFont(hdc, ghTipFont); //    サイズ併せる必要がある

    //    必要な領域を確認
    DrawText(hdc, gptContent, -1, &gstContSizeRaw, DT_LEFT | DT_CALCRECT | DT_NOPREFIX);

    SelectFont(hdc, hOldFnt);

    //    16px 기준으로 잰 뒤 50%로 축소해서 8px처럼 보이게 한다
    gstContSize.left = 0;
    gstContSize.top = 0;
    gstContSize.right = max(1L, (gstContSizeRaw.right + 1) / 2) + 4;
    gstContSize.bottom = max(1L, (gstContSizeRaw.bottom + 1) / 2) + 4;

    //    デスクトップサイズ確保
    GetWindowRect(GetDesktopWindow(), &deskRect);
    //    画面よりデカいならカット
    if (gstContSize.right > deskRect.right)
    {
        gstContSize.right = deskRect.right;
    }
    if (gstContSize.bottom > deskRect.bottom)
    {
        gstContSize.bottom = deskRect.bottom;
    }

    //    右下にはみ出すなら、表示出来るところまでオフセットする
    xSub = (point.x + gstContSize.right) - deskRect.right;
    if (0 < xSub)
    {
        point.x -= xSub;
    }
    ySub = (point.y + gstContSize.bottom) - deskRect.bottom;
    if (0 < ySub)
    {
        point.y -= ySub;
    }

    gdMoveVol = 0;
    SetWindowPos(ghTipWnd, HWND_TOPMOST, (point.x + 1), (point.y + 1), gstContSize.right, gstContSize.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    SetTimer(ghTipWnd, HOVER_TMID, HOVER_TIME, nullptr); //    消し用タイマ

    ReleaseDC(ghTipWnd, hdc);

    //    ドラフトボードと同じように、フローティングウインドウ作って、WM_PAINT で描画すればいい

    return 0; //    If an application processes this message, it should return zero.
}
//-------------------------------------------------------------------------------------------------

// WM_MOUSELEAVEを受け取る
LRESULT HoverTipOnMouseLeave(HWND hEvWnd)
{
    //    ここで、マウスカーソルがチップの上にくるとヤバイ

    return 0; //    If an application processes this message, it should return zero.
}
//-------------------------------------------------------------------------------------------------

// HoverTipを閉じる
VOID HoverTipClose(HWND hWnd)
{
    KillTimer(hWnd, HOVER_TMID);
    ShowWindow(hWnd, SW_HIDE);

    return;
}
//-------------------------------------------------------------------------------------------------

// ウインドウプロシージャ
LRESULT CALLBACK HoverTipProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_LBUTTONUP, Htp_OnLButtonUp); //    マウスクルック・纏めてもおｋ？
        HANDLE_MSG(hWnd, WM_MBUTTONUP, Htp_OnMButtonUp);
        HANDLE_MSG(hWnd, WM_RBUTTONUP, Htp_OnRButtonUp);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, Htp_OnMouseMove);
        HANDLE_MSG(hWnd, WM_KILLFOCUS, Htp_OnKillFocus); //    フォーカスを失った
        HANDLE_MSG(hWnd, WM_PAINT, Htp_OnPaint);
        HANDLE_MSG(hWnd, WM_TIMER, htp_OnTimer);

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

// PAINT。無効領域が出来たときに発生。背景の扱いに注意。背景を塗りつぶしてから、オブジェクトを描画
VOID Htp_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    HDC hdcMem;
    HFONT hOldFnt;
    HBITMAP hBmp;
    HBITMAP hOldBmp;
    RECT rect;
    RECT rectRaw;

    hdc = BeginPaint(hWnd, &ps);

    hdcMem = CreateCompatibleDC(hdc);
    hBmp = CreateCompatibleBitmap(hdc, max(1L, gstContSizeRaw.right + 8), max(1L, gstContSizeRaw.bottom + 8));
    hOldBmp = SelectBitmap(hdcMem, hBmp);

    rect = {0, 0, gstContSizeRaw.right + 8, gstContSizeRaw.bottom + 8};
    FillRect(hdcMem, &rect, (HBRUSH)(COLOR_INFOBK + 1));

    hOldFnt = SelectFont(hdcMem, ghTipFont);
    SetTextColor(hdcMem, GetSysColor(COLOR_INFOTEXT));
    SetBkMode(hdcMem, TRANSPARENT);

    rectRaw = {4, 4, gstContSizeRaw.right + 4, gstContSizeRaw.bottom + 4};
    DrawText(hdcMem, gptContent, -1, &rectRaw, DT_LEFT | DT_NOPREFIX);

    SetStretchBltMode(hdc, HALFTONE);
    StretchBlt(hdc, 0, 0, gstContSize.right, gstContSize.bottom, hdcMem, 0, 0, rect.right, rect.bottom, SRCCOPY);

    SelectFont(hdcMem, hOldFnt);
    SelectBitmap(hdcMem, hOldBmp);
    DeleteBitmap(hBmp);
    DeleteDC(hdcMem);

    EndPaint(hWnd, &ps);

    return;
}
//-------------------------------------------------------------------------------------------------

// タイマイベント発生
VOID htp_OnTimer(HWND hWnd, UINT id)
{
    //    関係ない場合・先ずありえないハズだが
    if (HOVER_TMID != id)
        return;

    HoverTipClose(hWnd);

    return;
}
//-------------------------------------------------------------------------------------------------

// フォーカスを失った場合
VOID Htp_OnKillFocus(HWND hWnd, HWND hwndNewFocus)
{
    return;
}
//-------------------------------------------------------------------------------------------------

// マウスの左ボタンがうっｐされたとき
VOID Htp_OnLButtonUp(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    HoverTipClose(hWnd);

    return;
}
//-------------------------------------------------------------------------------------------------

// マウスの中ボタンがうっｐされたとき
VOID Htp_OnMButtonUp(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    HoverTipClose(hWnd);

    return;
}
//-------------------------------------------------------------------------------------------------

// マウスの右ボタンがうっｐされたとき
VOID Htp_OnRButtonUp(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    HoverTipClose(hWnd);

    return;
}
//-------------------------------------------------------------------------------------------------

// マウスが動いたときの処理
VOID Htp_OnMouseMove(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    gdMoveVol++;
    if (HOVER_MOVE < gdMoveVol)
    {
        HoverTipClose(hWnd);
    }

    return;
}
//-------------------------------------------------------------------------------------------------

#endif
