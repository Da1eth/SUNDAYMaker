#include "AppModuleInternal.h"

HFONT ghNameFont;

static CONST TCHAR gcatUiFace[] = TEXT("돋움");

static LOGFONT gstBaseFont = {FONTSZ_NORMAL,
                              0,
                              0,
                              0,
                              FW_NORMAL,
                              FALSE,
                              FALSE,
                              FALSE,
                              DEFAULT_CHARSET,
                              OUT_OUTLINE_PRECIS,
                              CLIP_DEFAULT_PRECIS,
                              PROOF_QUALITY,
                              FIXED_PITCH,
                              TEXT("HeadKasen")};

static HANDLE ghHeadKasenFontHandle;
static HFONT ghDropDownFont;

static BOOL CALLBACK AppUiFontEnumProc(HWND hWnd, LPARAM lParam)
{
    SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

static HFONT AppUiCreateFont(INT dHeight, UINT dPoint)
{
    NONCLIENTMETRICS stNcm;
    LOGFONT stFont;
    HDC hdcScreen;

    ZeroMemory(&stNcm, sizeof(NONCLIENTMETRICS));
    stNcm.cbSize = sizeof(NONCLIENTMETRICS);

    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),
                             &stNcm, 0))
    {
        stFont = stNcm.lfMessageFont;
    }
    else
    {
        ZeroMemory(&stFont, sizeof(LOGFONT));
        stFont.lfCharSet = DEFAULT_CHARSET;
        stFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
        stFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        stFont.lfQuality = DEFAULT_QUALITY;
        stFont.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    }

    StringCchCopy(stFont.lfFaceName, LF_FACESIZE, gcatUiFace);
    if (0 != dHeight)
    {
        stFont.lfHeight = dHeight;
    }
    else
    {
        hdcScreen = GetDC(nullptr);
        if (hdcScreen)
        {
            stFont.lfHeight =
                -MulDiv(dPoint, GetDeviceCaps(hdcScreen, LOGPIXELSY), 72);
            ReleaseDC(nullptr, hdcScreen);
        }
        else
        {
            stFont.lfHeight = -MulDiv(dPoint, 96, 72);
        }
    }

    return CreateFontIndirect(&stFont);
}

static HRESULT AppHeadKasenFontInitialise(HINSTANCE hInstance)
{
    HRSRC hFontRc;
    HGLOBAL hFontMem;
    PVOID pFontData;
    DWORD dFontBytes;
    DWORD dFontsAdded;

    ghHeadKasenFontHandle = nullptr;

    hFontRc =
        FindResource(hInstance, MAKEINTRESOURCE(IDR_HEADKASEN_TTF), RT_RCDATA);
    if (!(hFontRc))
        return HRESULT_FROM_WIN32(GetLastError());

    hFontMem = LoadResource(hInstance, hFontRc);
    if (!(hFontMem))
        return HRESULT_FROM_WIN32(GetLastError());

    pFontData = LockResource(hFontMem);
    if (!(pFontData))
        return E_FAIL;

    dFontBytes = SizeofResource(hInstance, hFontRc);
    if (!(dFontBytes))
        return E_FAIL;

    dFontsAdded = 0;
    ghHeadKasenFontHandle =
        AddFontMemResourceEx(pFontData, dFontBytes, nullptr, &dFontsAdded);
    if (!(ghHeadKasenFontHandle) || !(dFontsAdded))
        return E_FAIL;

    // AddFontMemResourceEx로 로드한 폰트는 프로세스 전용이므로
    // HWND_BROADCAST로 WM_FONTCHANGE를 보낼 필요 없음.
    // SendMessage(HWND_BROADCAST)는 응답 없는 창이 있으면 무기한 블로킹됨.
    return S_OK;
}

static VOID AppHeadKasenFontFinalise(VOID)
{
    if (ghHeadKasenFontHandle)
    {
        RemoveFontMemResourceEx(ghHeadKasenFontHandle);
        ghHeadKasenFontHandle = nullptr;
    }
}

HRESULT AppUiResourcesInitialise(HINSTANCE hInstance)
{
    return AppHeadKasenFontInitialise(hInstance);
}

VOID AppUiResourcesFinalise(VOID) { AppHeadKasenFontFinalise(); }

HRESULT AppUiFontsInitialise(VOID)
{
    ghNameFont = AppUiCreateFont(0, 9);
    ghDropDownFont = AppUiCreateFont(0, 9);

    if (!(ghNameFont))
    {
        ghNameFont =
            CreateFont(0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                       DEFAULT_PITCH | FF_SWISS, gcatUiFace);
    }

    if (!(ghDropDownFont))
    {
        ghDropDownFont = ghNameFont;
    }

    return ghNameFont ? S_OK : E_FAIL;
}

VOID AppUiFontsFinalise(VOID)
{
    if (ghDropDownFont && ghDropDownFont != ghNameFont)
    {
        DeleteFont(ghDropDownFont);
    }

    if (ghNameFont)
    {
        DeleteFont(ghNameFont);
    }

    ghDropDownFont = nullptr;
    ghNameFont = nullptr;
}

HFONT AppUiFontGet(VOID) { return ghNameFont; }

HFONT AppUiDropDownFontGet(VOID)
{
    return ghDropDownFont ? ghDropDownFont : ghNameFont;
}

HRESULT AppUiFontApply(HWND hWnd, BOOL bApplyChildren)
{
    if (!(hWnd) || !(ghNameFont))
        return E_INVALIDARG;

    SendMessage(hWnd, WM_SETFONT, (WPARAM)ghNameFont, TRUE);
    if (bApplyChildren)
    {
        EnumChildWindows(hWnd, AppUiFontEnumProc, (LPARAM)ghNameFont);
    }

    return S_OK;
}

HRESULT ViewingFontGet(LPLOGFONT pstLogFont)
{
    ZeroMemory(pstLogFont, sizeof(LOGFONT));
    *pstLogFont = gstBaseFont;
    return S_OK;
}