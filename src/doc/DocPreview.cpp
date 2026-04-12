// 문서 미리보기를 텍스트 기반으로 렌더링한다.
#include "Sunday.h"
//-------------------------------------------------------------------------------------------------

#define DOC_PREVIEW_CLASS TEXT("PREVIEW_CLASS")

#define PVW_WIDTH 820
#define PVW_HEIGHT 480

static HWND ghPrevWnd;
static HWND ghTextWnd;
static HINSTANCE ghInst;
static HBRUSH ghPreviewBrush;
static WNDPROC gpOldPreviewEditProc;

extern HWND ghPrntWnd;
extern FILES_ITR gitFileIt;
extern INT gixFocusPage;
extern HFONT ghAaFont;

static CONST TCHAR *gcptWeek[7] = {
    TEXT("일"), TEXT("월"), TEXT("화"), TEXT("수"), TEXT("목"), TEXT("금"), TEXT("토")};

LRESULT CALLBACK PreviewWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PreviewEditProc(HWND, UINT, WPARAM, LPARAM);
VOID Pvw_OnSize(HWND, UINT, INT, INT);
VOID Pvw_OnPaint(HWND);
VOID Pvw_OnDestroy(HWND);
VOID Pvw_OnKey(HWND, UINT, BOOL, INT, UINT);
HRESULT PreviewPageWrite(INT);

static VOID PreviewAppendPageText(wstring &, INT, const SYSTEMTIME &, BOOLEAN);
static VOID PreviewEnsureTrailingBreak(wstring &);
static HBRUSH PreviewCtlColor(HDC hdc);
//-------------------------------------------------------------------------------------------------

VOID PreviewInitialise(HINSTANCE hInstance, HWND hParentWnd)
{
    (void)hParentWnd;

    WNDCLASSEX wcex;

    if (hInstance)
    {
        ghInst = hInstance;

        ZeroMemory(&wcex, sizeof(WNDCLASSEX));
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = PreviewWndProc;
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszClassName = DOC_PREVIEW_CLASS;

        RegisterClassEx(&wcex);

        ghPrevWnd = nullptr;
        ghTextWnd = nullptr;
        gpOldPreviewEditProc = nullptr;
        ghPreviewBrush = CreateSolidBrush(RGB(0xEF, 0xEF, 0xEF));
    }
    else
    {
        if (ghPrevWnd)
        {
            SendMessage(ghPrevWnd, WM_CLOSE, 0, 0);
        }

        if (ghPreviewBrush)
        {
            DeleteBrush(ghPreviewBrush);
            ghPreviewBrush = nullptr;
        }
    }
}
//-------------------------------------------------------------------------------------------------

HRESULT PreviewVisibalise(INT iNowPage, BOOLEAN bForeg)
{
    HWND hWnd;
    RECT rect;

    if (ghPrevWnd)
    {
        PreviewPageWrite(-1);
        InvalidateRect(ghPrevWnd, nullptr, TRUE);

        if (bForeg)
        {
            SetForegroundWindow(ghPrevWnd);
        }
        return S_FALSE;
    }

    if (!(bForeg))
    {
        return E_ABORT;
    }

    InitWindowPos(INIT_LOAD, WDP_PREVIEW, &rect);
    if (0 >= rect.right || 0 >= rect.bottom)
    {
        hWnd = GetDesktopWindow();
        GetWindowRect(hWnd, &rect);
        rect.left = (rect.right - PVW_WIDTH) / 2;
        rect.top = (rect.bottom - PVW_HEIGHT) / 2;
        rect.right = PVW_WIDTH;
        rect.bottom = PVW_HEIGHT;
        InitWindowPos(INIT_SAVE, WDP_PREVIEW, &rect);
    }

    ghPrevWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_COMPOSITED, DOC_PREVIEW_CLASS, TEXT("미리보기"),
                               WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_SYSMENU,
                               rect.left, rect.top, rect.right, rect.bottom, nullptr, nullptr, ghInst, nullptr);
    if (!(ghPrevWnd))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    GetClientRect(ghPrevWnd, &rect);

    ghTextWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, TEXT(""),
                               WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_READONLY |
                                   ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
                               rect.left, rect.top, rect.right, rect.bottom - rect.top,
                               ghPrevWnd, (HMENU)IDW_PVW_VIEW_WNDW, ghInst, nullptr);
    if (!(ghTextWnd))
    {
        DestroyWindow(ghPrevWnd);
        ghPrevWnd = nullptr;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    SendMessage(ghTextWnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(12, 12));
    SetWindowFont(ghTextWnd, ghAaFont ? ghAaFont : AppUiFontGet(), TRUE);
    gpOldPreviewEditProc = (WNDPROC)SetWindowLongPtr(ghTextWnd, GWLP_WNDPROC, (LONG_PTR)PreviewEditProc);

    PreviewPageWrite(iNowPage);
    UpdateWindow(ghPrevWnd);
    SetFocus(ghTextWnd);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

LRESULT CALLBACK PreviewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_SIZE, Pvw_OnSize);
        HANDLE_MSG(hWnd, WM_PAINT, Pvw_OnPaint);
        HANDLE_MSG(hWnd, WM_DESTROY, Pvw_OnDestroy);
        HANDLE_MSG(hWnd, WM_KEYDOWN, Pvw_OnKey);

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        return (LRESULT)PreviewCtlColor((HDC)wParam);

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

LRESULT CALLBACK PreviewEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (WM_KEYDOWN == message && VK_ESCAPE == wParam)
    {
        DestroyWindow(ghPrevWnd ? ghPrevWnd : hWnd);
        return 0;
    }

    return CallWindowProc(gpOldPreviewEditProc, hWnd, message, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------

VOID Pvw_OnSize(HWND hWnd, UINT state, INT cx, INT cy)
{
    RECT rect;

    (void)hWnd;
    (void)state;
    (void)cx;
    (void)cy;

    GetClientRect(ghPrevWnd, &rect);
    MoveWindow(ghTextWnd, rect.left, rect.top, rect.right, rect.bottom - rect.top, TRUE);

    if (ghTextWnd)
    {
        PreviewPageWrite(-1);
    }
}
//-------------------------------------------------------------------------------------------------

VOID Pvw_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;

    BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
}
//-------------------------------------------------------------------------------------------------

VOID Pvw_OnDestroy(HWND hWnd)
{
    RECT rect;

    if (ghPrevWnd)
    {
        GetWindowRect(ghPrevWnd, &rect);
        rect.right -= rect.left;
        rect.bottom -= rect.top;
        InitWindowPos(INIT_SAVE, WDP_PREVIEW, &rect);
    }

    ghPrevWnd = nullptr;
    ghTextWnd = nullptr;
    gpOldPreviewEditProc = nullptr;

    if (hWnd)
    {
        PostMessage(ghPrntWnd, WMP_PREVIEW_CLOSE, 0, 0);
    }
}
//-------------------------------------------------------------------------------------------------

HRESULT PreviewPageWrite(INT iViewPage)
{
    INT iPageCount;
    INT iFirstLine;
    INT iPage;
    SYSTEMTIME stTime;
    wstring wsPreview;

    if (!(ghTextWnd))
    {
        return E_HANDLE;
    }

    (void)iViewPage;
    iFirstLine = (INT)SendMessage(ghTextWnd, EM_GETFIRSTVISIBLELINE, 0, 0);

    GetLocalTime(&stTime);
    iPageCount = DocNowFilePageCount();

    for (iPage = 0; iPageCount > iPage; iPage++)
    {
        PreviewAppendPageText(wsPreview, iPage, stTime, (iPage + 1) < iPageCount);
    }

    if (wsPreview.empty())
    {
        wsPreview = TEXT("미리볼 내용이 없습니다.\r\n");
    }

    SetWindowText(ghTextWnd, wsPreview.c_str());

    if (0 < iFirstLine)
    {
        SendMessage(ghTextWnd, EM_LINESCROLL, 0, iFirstLine);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

VOID Pvw_OnKey(HWND hWnd, UINT vk, BOOL fDown, INT cRepeat, UINT flags)
{
    (void)cRepeat;
    (void)flags;

    if (fDown && VK_ESCAPE == vk)
    {
        DestroyWindow(hWnd);
    }
}
//-------------------------------------------------------------------------------------------------

static VOID PreviewAppendPageText(wstring &wsPreview, INT iPage, const SYSTEMTIME &stTime, BOOLEAN bHasNext)
{
    LPTSTR ptContent;
    TCHAR atHeader[BIG_STRING];

    ptContent = nullptr;
    if (0 >= DocPageTextGetAlloc(gitFileIt, iPage, D_UNI, (LPVOID *)(&ptContent), FALSE) || !(ptContent))
    {
        return;
    }

    StringCchPrintf(atHeader, BIG_STRING,
                    TEXT("%d : 익명의 참치 씨 (PREVIEW) : %04d-%02d-%02d(%s) %02d:%02d:%02d\r\n"),
                    iPage + 1,
                    stTime.wYear, stTime.wMonth, stTime.wDay,
                    gcptWeek[stTime.wDayOfWeek],
                    stTime.wHour, stTime.wMinute, stTime.wSecond);

    wsPreview += atHeader;
    wsPreview += TEXT("\r\n");
    wsPreview += ptContent;
    PreviewEnsureTrailingBreak(wsPreview);
    if (bHasNext)
    {
        wsPreview += TEXT("\r\n\r\n");
    }

    FREE(ptContent);
}
//-------------------------------------------------------------------------------------------------

static VOID PreviewEnsureTrailingBreak(wstring &wsPreview)
{
    if (wsPreview.empty())
    {
        return;
    }

    if (wsPreview.size() >= 2 &&
        TEXT('\r') == wsPreview[wsPreview.size() - 2] &&
        TEXT('\n') == wsPreview[wsPreview.size() - 1])
    {
        return;
    }

    wsPreview += TEXT("\r\n");
}
//-------------------------------------------------------------------------------------------------

static HBRUSH PreviewCtlColor(HDC hdc)
{
    SetTextColor(hdc, RGB(0x20, 0x20, 0x20));
    SetBkColor(hdc, RGB(0xEF, 0xEF, 0xEF));
    return ghPreviewBrush ? ghPreviewBrush : (HBRUSH)(COLOR_WINDOW + 1);
}
