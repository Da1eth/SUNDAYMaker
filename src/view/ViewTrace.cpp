#include "Sunday.h"

#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>

#define TRC_SCROLLBAR

#define TRC_POSITION_RANGE 2000
#define TRC_POSITION_OFFSET 1000
#define TRC_POSITION_PAGE_X 11
#define TRC_POSITION_PAGE_Y 18

#define TRC_CONTRA_RANGE 510
#define TRC_CONTRA_OFFSET 255

#define TRC_GAMMA_RANGE 3000
#define TRC_GAMMA_OFFSET 1000
#define TRC_GAMMA_STEP 100

#define TRC_GRAYMOPH_RANGE 255

#define TRC_ZOOM_RANGE 375
#define TRC_ZOOM_OFFSET 25
#define TRC_ZOOM_DEFAULT 75

#define TRC_TURN_RANGE 359

static INT_PTR CALLBACK TraceCtrlDlgProc(HWND, UINT, WPARAM, LPARAM);
static UINT_PTR CALLBACK ImageOpenDlgHookProc(HWND, UINT, WPARAM, LPARAM);
HRESULT TraceImageFileOpen(HWND hDlg);
HRESULT TraceMoziColourChoice(HWND hDlg);
INT_PTR TraceOnScroll(HWND hDlg, HWND hWndCtl, UINT code, INT pos);
HRESULT TraceRedrawIamge(VOID);

struct TraceValueBinding
{
    UINT editID;
    UINT sliderID;
    double scale;
    INT offset;
};

static constexpr TraceValueBinding gTraceValueBindings[] = {
    {IDE_TRC_HRIZ_VALUE, IDSL_TRC_HRIZ_POS, 1.0, TRC_POSITION_OFFSET},
    {IDE_TRC_VART_VALUE, IDSL_TRC_VART_POS, 1.0, TRC_POSITION_OFFSET},
    {IDE_TRC_CONTRAST_VALUE, IDSL_TRC_CONTRAST, 1.0, TRC_CONTRA_OFFSET},
    {IDE_TRC_GAMMA_VALUE, IDSL_TRC_GAMMA, 1000.0, 0},
    {IDE_TRC_GRAYMOPH_VALUE, IDSL_TRC_GRAYMOPH, 1.0, 0},
    {IDE_TRC_ZOOM_VALUE, IDSL_TRC_ZOOM, 1.0, -TRC_ZOOM_OFFSET},
    {IDE_TRC_TURN_VALUE, IDSL_TRC_TURN, 1.0, 0},
};

static const TraceValueBinding *FindTraceValueBinding(UINT editID)
{
    for (const auto &binding : gTraceValueBindings)
    {
        if (binding.editID == editID)
            return &binding;
    }
    return nullptr;
}

static bool IsTraceScrollStepCode(UINT code)
{
    return code == TB_LINEUP || code == TB_LINEDOWN || code == TB_PAGEUP || code == TB_PAGEDOWN ||
           code == SB_LINELEFT || code == SB_LINERIGHT || code == SB_LINEUP ||
           code == SB_LINEDOWN || code == SB_PAGELEFT || code == SB_PAGERIGHT ||
           code == SB_PAGEUP || code == SB_PAGEDOWN;
}

static INT ApplyTraceScrollDelta(UINT ctlID, INT pos, UINT code)
{
    if (code == TB_LINEUP || code == SB_LINELEFT || code == SB_LINEUP)
    {
        if (ctlID == IDSL_TRC_GAMMA)
            return pos - TRC_GAMMA_STEP;
        return pos - 1;
    }

    if (code == TB_LINEDOWN || code == SB_LINERIGHT || code == SB_LINEDOWN)
    {
        if (ctlID == IDSL_TRC_GAMMA)
            return pos + TRC_GAMMA_STEP;
        return pos + 1;
    }

    if (code == TB_PAGEUP || code == SB_PAGELEFT || code == SB_PAGEUP)
    {
        switch (ctlID)
        {
        case IDSL_TRC_HRIZ_POS:
            return pos - TRC_POSITION_PAGE_X;
        case IDSL_TRC_VART_POS:
            return pos - TRC_POSITION_PAGE_Y;
        case IDSL_TRC_GAMMA:
            return pos - TRC_GAMMA_STEP;
        default:
            return pos - 5;
        }
    }

    if (code == TB_PAGEDOWN || code == SB_PAGERIGHT || code == SB_PAGEDOWN)
    {
        switch (ctlID)
        {
        case IDSL_TRC_HRIZ_POS:
            return pos + TRC_POSITION_PAGE_X;
        case IDSL_TRC_VART_POS:
            return pos + TRC_POSITION_PAGE_Y;
        case IDSL_TRC_GAMMA:
            return pos + TRC_GAMMA_STEP;
        default:
            return pos + 5;
        }
    }

    return pos;
}

namespace
{

    struct DIBDATA
    {
        BITMAPINFOHEADER bih;
        BYTE *pBits;
        DWORD cbStride;
        DWORD cbImage;
    };

    using HDIB = DIBDATA *;
    using ByteLut = std::array<BYTE, 256>;

    static HDIB ghImgDib = nullptr;
    static HDIB ghOrigDib = nullptr;
    static SIZE gstImgSize{};
    static HBRUSH gMoziClrBrush = nullptr;
    static HWND ghTraceDlg = nullptr;
    static HWND ghTraceImageOpenDlg = nullptr;
    static BOOLEAN gbThumbUse = TRUE;
    static TRACEPARAM gstTrcPrm{};
    static BOOLEAN gbOnView = TRUE;

    static DWORD CalcStride24(LONG width)
    {
        return ((width * 3) + 3) & ~3;
    }

    static LONG GetDIBHeight(HDIB hDIB)
    {
        return hDIB ? abs(hDIB->bih.biHeight) : 0;
    }

    static BYTE *GetDIBRow(HDIB hDIB, LONG y)
    {
        const LONG height = GetDIBHeight(hDIB);
        if (!hDIB || y < 0 || y >= height)
            return nullptr;
        return hDIB->pBits + static_cast<size_t>(y) * hDIB->cbStride;
    }

    static const BYTE *GetConstDIBRow(HDIB hDIB, LONG y)
    {
        return GetDIBRow(hDIB, y);
    }

    static BYTE ClampByte(int value)
    {
        return static_cast<BYTE>(std::clamp(value, 0, 255));
    }

    static ByteLut BuildContrastLut(short strength)
    {
        ByteLut lut{};
        const double factor = (259.0 * (strength + 255.0)) / (255.0 * (259.0 - strength));
        for (int i = 0; i < 256; i++)
        {
            lut[i] = ClampByte(static_cast<int>(factor * (i - 128) + 128));
        }
        return lut;
    }

    static ByteLut BuildGammaLut(WORD gamma)
    {
        ByteLut lut{};
        const double gammaValue = (gamma > 0) ? (gamma / 1000.0) : 0.1;
        for (int i = 0; i < 256; i++)
        {
            const int value = static_cast<int>(std::pow(i / 255.0, 1.0 / gammaValue) * 255.0 + 0.5);
            lut[i] = static_cast<BYTE>(value < 255 ? value : 255);
        }
        return lut;
    }

    static void ApplyLutDIB(HDIB hDIB, const ByteLut &lutB, const ByteLut &lutG, const ByteLut &lutR)
    {
        const LONG width = hDIB->bih.biWidth;
        const LONG height = GetDIBHeight(hDIB);
        for (LONG y = 0; y < height; y++)
        {
            BYTE *pixel = GetDIBRow(hDIB, y);
            BYTE *const end = pixel + static_cast<size_t>(width) * 3;
            for (; pixel < end; pixel += 3)
            {
                pixel[0] = lutB[pixel[0]];
                pixel[1] = lutG[pixel[1]];
                pixel[2] = lutR[pixel[2]];
            }
        }
    }

    static HDIB AllocDIB24(LONG width, LONG height)
    {
        if (width <= 0 || height <= 0)
            return nullptr;

        auto *pDib = static_cast<DIBDATA *>(calloc(1, sizeof(DIBDATA)));
        if (!pDib)
            return nullptr;

        pDib->bih.biSize = sizeof(BITMAPINFOHEADER);
        pDib->bih.biWidth = width;
        pDib->bih.biHeight = -height;
        pDib->bih.biPlanes = 1;
        pDib->bih.biBitCount = 24;
        pDib->bih.biCompression = BI_RGB;
        pDib->cbStride = CalcStride24(width);
        pDib->cbImage = pDib->cbStride * height;
        pDib->bih.biSizeImage = pDib->cbImage;
        pDib->pBits = static_cast<BYTE *>(calloc(1, pDib->cbImage));
        if (!pDib->pBits)
        {
            free(pDib);
            return nullptr;
        }

        return pDib;
    }

    static HDIB CreateDIBFromTopDownBGR24(const BYTE *pBgr, LONG width, LONG height, DWORD cbStride)
    {
        if (!pBgr)
            return nullptr;

        HDIB hDIB = AllocDIB24(width, height);
        if (!hDIB)
            return nullptr;

        for (LONG y = 0; y < height; y++)
        {
            memcpy(GetDIBRow(hDIB, y), pBgr + static_cast<size_t>(y) * cbStride, static_cast<size_t>(width) * 3);
        }

        return hDIB;
    }

    static BOOL DeleteDIB(HDIB hDIB)
    {
        if (!hDIB)
            return FALSE;
        free(hDIB->pBits);
        free(hDIB);
        return TRUE;
    }

    static BOOL HeadDIB(HDIB hDIB, LPBITMAPINFOHEADER pbmih)
    {
        if (!hDIB || !pbmih)
            return FALSE;
        *pbmih = hDIB->bih;
        pbmih->biHeight = GetDIBHeight(hDIB);
        return TRUE;
    }

    static HDIB CopyDIB(HDIB hDIB)
    {
        if (!hDIB)
            return nullptr;

        auto *dst = static_cast<DIBDATA *>(calloc(1, sizeof(DIBDATA)));
        if (!dst)
            return nullptr;

        *dst = *hDIB;
        dst->pBits = static_cast<BYTE *>(malloc(hDIB->cbImage));
        if (!dst->pBits)
        {
            free(dst);
            return nullptr;
        }

        memcpy(dst->pBits, hDIB->pBits, hDIB->cbImage);
        return dst;
    }

    static BOOL ContrastDIB(HDIB hDIB, short rStr, short gStr, short bStr)
    {
        if (!hDIB)
            return FALSE;
        if (rStr == 0 && gStr == 0 && bStr == 0)
            return TRUE;
        ApplyLutDIB(hDIB, BuildContrastLut(bStr), BuildContrastLut(gStr), BuildContrastLut(rStr));
        return TRUE;
    }

    static BOOL GammaDIB(HDIB hDIB, WORD rGma, WORD gGma, WORD bGma)
    {
        if (!hDIB)
            return FALSE;
        if (rGma == 1000 && gGma == 1000 && bGma == 1000)
            return TRUE;
        ApplyLutDIB(hDIB, BuildGammaLut(bGma), BuildGammaLut(gGma), BuildGammaLut(rGma));
        return TRUE;
    }

    static BOOL GrayDIB(HDIB hDIB, WORD wStrength)
    {
        if (!hDIB)
            return FALSE;
        if (wStrength == 0)
            return TRUE;

        const LONG width = hDIB->bih.biWidth;
        const LONG height = GetDIBHeight(hDIB);
        const int strength = std::clamp<int>(wStrength, 0, 255);

        for (LONG y = 0; y < height; y++)
        {
            BYTE *pixel = GetDIBRow(hDIB, y);
            BYTE *const end = pixel + static_cast<size_t>(width) * 3;
            for (; pixel < end; pixel += 3)
            {
                const int b = pixel[0];
                const int g = pixel[1];
                const int r = pixel[2];
                const int gray = (77 * r + 150 * g + 29 * b) >> 8;
                pixel[0] = static_cast<BYTE>(b + ((gray - b) * strength) / 255);
                pixel[1] = static_cast<BYTE>(g + ((gray - g) * strength) / 255);
                pixel[2] = static_cast<BYTE>(r + ((gray - r) * strength) / 255);
            }
        }

        return TRUE;
    }

    static BOOL SampleBilinear24(HDIB hDIB, double sx, double sy, BYTE *dstPixel)
    {
        const LONG width = hDIB->bih.biWidth;
        const LONG height = GetDIBHeight(hDIB);
        if (sx < 0.0 || sy < 0.0 || sx > (width - 1.0) || sy > (height - 1.0))
            return FALSE;

        const LONG x0 = static_cast<LONG>(std::floor(sx));
        const LONG y0 = static_cast<LONG>(std::floor(sy));
        const LONG x1 = (std::min)(x0 + 1, width - 1);
        const LONG y1 = (std::min)(y0 + 1, height - 1);
        const double tx = sx - x0;
        const double ty = sy - y0;

        const BYTE *row0 = GetConstDIBRow(hDIB, y0);
        const BYTE *row1 = GetConstDIBRow(hDIB, y1);
        const BYTE *p00 = row0 + x0 * 3;
        const BYTE *p10 = row0 + x1 * 3;
        const BYTE *p01 = row1 + x0 * 3;
        const BYTE *p11 = row1 + x1 * 3;

        for (int c = 0; c < 3; c++)
        {
            const double top = p00[c] + (p10[c] - p00[c]) * tx;
            const double bottom = p01[c] + (p11[c] - p01[c]) * tx;
            dstPixel[c] = ClampByte(static_cast<int>(top + (bottom - top) * ty + 0.5));
        }

        return TRUE;
    }

    static BOOL TurnDIBBilinear(HDIB hDIB, long lAngle, COLORREF clrBack)
    {
        if (!hDIB)
            return FALSE;
        if (lAngle == 0)
            return TRUE;

        const LONG oldW = hDIB->bih.biWidth;
        const LONG oldH = GetDIBHeight(hDIB);
        const double rad = (lAngle / 1000.0) * std::numbers::pi_v<double> / 180.0;
        const double cosA = std::cos(rad);
        const double sinA = std::sin(rad);
        const double cx = oldW / 2.0;
        const double cy = oldH / 2.0;

        const double corners[4][2] = {
            {0.0, 0.0},
            {static_cast<double>(oldW), 0.0},
            {static_cast<double>(oldW), static_cast<double>(oldH)},
            {0.0, static_cast<double>(oldH)}};

        double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
        for (const auto &corner : corners)
        {
            const double rx = (corner[0] - cx) * cosA - (corner[1] - cy) * sinA;
            const double ry = (corner[0] - cx) * sinA + (corner[1] - cy) * cosA;
            minX = (std::min)(minX, rx);
            maxX = (std::max)(maxX, rx);
            minY = (std::min)(minY, ry);
            maxY = (std::max)(maxY, ry);
        }

        const LONG newW = static_cast<LONG>(maxX - minX + 0.5);
        const LONG newH = static_cast<LONG>(maxY - minY + 0.5);
        if (newW <= 0 || newH <= 0)
            return FALSE;

        const DWORD newStride = CalcStride24(newW);
        const DWORD newCbImage = newStride * newH;
        auto newBits = std::unique_ptr<BYTE, decltype(&free)>(static_cast<BYTE *>(malloc(newCbImage)), &free);
        if (!newBits)
            return FALSE;

        const BYTE bgB = GetBValue(clrBack);
        const BYTE bgG = GetGValue(clrBack);
        const BYTE bgR = GetRValue(clrBack);

        auto fillRow = std::make_unique<BYTE[]>(newStride);
        for (LONG x = 0; x < newW; x++)
        {
            fillRow[x * 3 + 0] = bgB;
            fillRow[x * 3 + 1] = bgG;
            fillRow[x * 3 + 2] = bgR;
        }
        for (LONG y = 0; y < newH; y++)
        {
            memcpy(newBits.get() + static_cast<size_t>(y) * newStride, fillRow.get(), newStride);
        }

        const double newCx = newW / 2.0;
        const double newCy = newH / 2.0;

        for (LONG dy = 0; dy < newH; dy++)
        {
            BYTE *dstRow = newBits.get() + static_cast<size_t>(dy) * newStride;
            for (LONG dx = 0; dx < newW; dx++)
            {
                const double sx = (dx - newCx) * cosA + (dy - newCy) * sinA + cx;
                const double sy = -(dx - newCx) * sinA + (dy - newCy) * cosA + cy;
                SampleBilinear24(hDIB, sx, sy, dstRow + dx * 3);
            }
        }

        BYTE *oldBits = hDIB->pBits;
        hDIB->pBits = newBits.release();
        free(oldBits);
        hDIB->bih.biWidth = newW;
        hDIB->bih.biHeight = -newH;
        hDIB->cbStride = newStride;
        hDIB->cbImage = newCbImage;
        hDIB->bih.biSizeImage = newCbImage;
        return TRUE;
    }

    static BOOL DIBtoDCex(HDC hdc, int nXDest, int nYDest, int nDestWidth, int nDestHeight,
                          HDIB hDIB, int nXSrc, int nYSrc, int nSrcWidth, int nSrcHeight, DWORD dwRop)
    {
        if (!hDIB)
            return FALSE;

        int srcW = nSrcWidth ? nSrcWidth : hDIB->bih.biWidth;
        int srcH = nSrcHeight ? nSrcHeight : GetDIBHeight(hDIB);
        int dstX = nXDest, dstY = nYDest;
        int dstW = nDestWidth, dstH = nDestHeight;

        if (srcW < 0)
        {
            dstX += dstW;
            dstW = -dstW;
            srcW = -srcW;
        }
        if (srcH < 0)
        {
            dstY += dstH;
            dstH = -dstH;
            srcH = -srcH;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader = hDIB->bih;

        StretchDIBits(hdc, dstX, dstY, dstW, dstH,
                      nXSrc, nYSrc, srcW, srcH,
                      hDIB->pBits, &bmi, DIB_RGB_COLORS, dwRop);

        return TRUE;
    }

    static HDIB LoadImageDIB(LPCTSTR ptFileName)
    {
        if (!ptFileName)
            return nullptr;

        HDIB hDib = nullptr;
        HRESULT hr;
        BOOL bCoInit = FALSE;
        IWICImagingFactory *pFactory = nullptr;
        IWICBitmapDecoder *pDecoder = nullptr;
        IWICBitmapFrameDecode *pFrame = nullptr;
        IWICFormatConverter *pConverter = nullptr;
        UINT width = 0, height = 0;
        UINT cbStride = 0, cbImage = 0;
        BYTE *pBits = nullptr;

        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr))
            bCoInit = TRUE;
        else if (hr != RPC_E_CHANGED_MODE)
            return nullptr;

        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                              IID_IWICImagingFactory, (LPVOID *)&pFactory);
        if (FAILED(hr))
            goto cleanup;

        hr = pFactory->CreateDecoderFromFilename(ptFileName, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
        if (FAILED(hr))
            goto cleanup;

        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr))
            goto cleanup;

        hr = pFactory->CreateFormatConverter(&pConverter);
        if (FAILED(hr))
            goto cleanup;

        hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat24bppBGR,
                                    WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
            goto cleanup;

        hr = pConverter->GetSize(&width, &height);
        if (FAILED(hr))
            goto cleanup;

        cbStride = CalcStride24(static_cast<LONG>(width));
        cbImage = cbStride * height;
        pBits = static_cast<BYTE *>(malloc(cbImage));
        if (!pBits)
            goto cleanup;

        hr = pConverter->CopyPixels(nullptr, cbStride, cbImage, pBits);
        if (FAILED(hr))
            goto cleanup;

        hDib = CreateDIBFromTopDownBGR24(pBits, static_cast<LONG>(width), static_cast<LONG>(height), cbStride);

    cleanup:
        free(pBits);
        if (pConverter)
            pConverter->Release();
        if (pFrame)
            pFrame->Release();
        if (pDecoder)
            pDecoder->Release();
        if (pFactory)
            pFactory->Release();
        if (bCoInit)
            CoUninitialize();
        return hDib;
    }

} // namespace

INT TraceInitialise(HWND hWnd, UINT bMode)
{
    UNREFERENCED_PARAMETER(hWnd);

    if (bMode)
    {
        ghTraceDlg = nullptr;
        ghImgDib = nullptr;
        ghOrigDib = nullptr;
        gstTrcPrm.stOffsetPt.x = TRC_POSITION_OFFSET;
        gstTrcPrm.stOffsetPt.y = TRC_POSITION_OFFSET;
        gstTrcPrm.dContrast = TRC_CONTRA_OFFSET;
        gstTrcPrm.dGamma = TRC_GAMMA_OFFSET;
        gstTrcPrm.dGrayMoph = 0;
        gstTrcPrm.dZooming = TRC_ZOOM_DEFAULT;
        gstTrcPrm.dTurning = 0;
        gstTrcPrm.bUpset = BST_UNCHECKED;
        gstTrcPrm.bMirror = BST_UNCHECKED;
        gstTrcPrm.dMoziColour = ViewMoziColourGet(nullptr);
        InitTraceValue(INIT_LOAD, &gstTrcPrm);
        gbOnView = TRUE;
        gbThumbUse = TRUE;
    }
    else
    {
        if (ghImgDib)
        {
            DeleteDIB(ghImgDib);
            ghImgDib = nullptr;
        }
        if (ghOrigDib)
        {
            DeleteDIB(ghOrigDib);
            ghOrigDib = nullptr;
        }
        if (ghTraceDlg)
            DestroyWindow(ghTraceDlg);
    }

    return 0;
}

HRESULT TraceDialogueOpen(HINSTANCE hInst, HWND hWnd)
{
    HWND hDktpWnd;
    LONG x;
    RECT rect, dtRect, trRect;

    if (ghTraceDlg)
    {
        PostMessage(ghTraceDlg, WM_CLOSE, 0, 0);
        return S_OK;
    }

    GetWindowRect(hWnd, &rect);
#ifdef TRC_SCROLLBAR
    ghTraceDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_TRACEADJUST_DLG2), hWnd, TraceCtrlDlgProc, 0);
#else
    ghTraceDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_TRACEADJUST_DLG), hWnd, TraceCtrlDlgProc, 0);
#endif
    GetClientRect(ghTraceDlg, &trRect);

    hDktpWnd = GetDesktopWindow();
    GetWindowRect(hDktpWnd, &dtRect);
    x = dtRect.right - rect.right;
    if (trRect.right > x)
        rect.right = dtRect.right - trRect.right;

    SetWindowPos(ghTraceDlg, HWND_TOP, rect.right, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
    if (ghOrigDib)
        ViewRedrawSetLine(-1);
    MenuItemCheckOnOff(IDM_TRACE_MODE_ON, TRUE);
    AppTitleTrace(TRUE);
    return S_OK;
}

// 에딧 컨트롤에서 직접 입력한 값을 파싱하여 슬라이더에 반영
static void TraceApplyEditValue(HWND hDlg, UINT editID)
{
    const TraceValueBinding *pBinding = FindTraceValueBinding(editID);
    if (!pBinding)
        return;

    TCHAR atBuf[SUB_STRING]{};
    GetDlgItemText(hDlg, editID, atBuf, SUB_STRING);

    // 숫자 및 소수점, 부호, '%' 허용 파싱
    double dVal = 0.0;

    // "25 ％" 또는 "25%" 등 처리: '%' 및 전각 '%' 제거
    for (TCHAR *p = atBuf; *p; ++p)
    {
        if (*p == TEXT('%') || *p == TEXT('\uFF05'))
            *p = TEXT('\0');
    }
    dVal = _tcstod(atBuf, nullptr);

    const INT rawPos = static_cast<INT>(dVal * pBinding->scale) + pBinding->offset;
    TraceOnScroll(hDlg, GetDlgItem(hDlg, pBinding->sliderID), TB_THUMBPOSITION, rawPos);
}

static LRESULT CALLBACK TraceValueEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                                   UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

static void TraceSubclassValueEdits(HWND hDlg)
{
    for (const auto &binding : gTraceValueBindings)
    {
        SetWindowSubclass(GetDlgItem(hDlg, binding.editID), TraceValueEditSubclassProc, binding.editID, 0);
    }
}

#ifdef TRC_SCROLLBAR
static void TraceInitScrollControl(HWND hDlg, UINT sliderID, INT rangeMax, INT value, SCROLLINFO *pScrollInfo)
{
    HWND hSlider = GetDlgItem(hDlg, sliderID);
    pScrollInfo->nMax = rangeMax;
    SetScrollInfo(hSlider, SB_CTL, pScrollInfo, TRUE);
    TraceOnScroll(hDlg, hSlider, TB_THUMBPOSITION, value);
}
#else
static void TraceInitScrollControl(HWND hDlg, UINT sliderID, INT rangeMax, INT value, void *)
{
    UNREFERENCED_PARAMETER(rangeMax);
    TraceOnScroll(hDlg, GetDlgItem(hDlg, sliderID), TB_THUMBPOSITION, value);
}
#endif

static void TraceResetDialogState(HWND hDlg)
{
    if (ghImgDib)
    {
        DeleteDIB(ghImgDib);
        ghImgDib = nullptr;
    }
    if (ghOrigDib)
    {
        ghImgDib = CopyDIB(ghOrigDib);
    }

    gstTrcPrm.stOffsetPt.x = TRC_POSITION_OFFSET;
    gstTrcPrm.stOffsetPt.y = TRC_POSITION_OFFSET;
    gstTrcPrm.dContrast = TRC_CONTRA_OFFSET;
    gstTrcPrm.dGamma = TRC_GAMMA_OFFSET;
    gstTrcPrm.dGrayMoph = 0;
    gstTrcPrm.dZooming = TRC_ZOOM_DEFAULT;
    gstTrcPrm.dTurning = 0;
    gstTrcPrm.bUpset = BST_UNCHECKED;
    gstTrcPrm.bMirror = BST_UNCHECKED;

    CheckDlgButton(hDlg, IDCB_TRC_IMG_UPSET, BST_UNCHECKED);
    CheckDlgButton(hDlg, IDCB_TRC_IMG_MIRROR, BST_UNCHECKED);

    TraceOnScroll(hDlg, GetDlgItem(hDlg, IDSL_TRC_HRIZ_POS), TB_THUMBPOSITION, gstTrcPrm.stOffsetPt.x);
    TraceOnScroll(hDlg, GetDlgItem(hDlg, IDSL_TRC_VART_POS), TB_THUMBPOSITION, gstTrcPrm.stOffsetPt.y);
    TraceOnScroll(hDlg, GetDlgItem(hDlg, IDSL_TRC_CONTRAST), TB_THUMBPOSITION, gstTrcPrm.dContrast);
    TraceOnScroll(hDlg, GetDlgItem(hDlg, IDSL_TRC_GAMMA), TB_THUMBPOSITION, gstTrcPrm.dGamma);
    TraceOnScroll(hDlg, GetDlgItem(hDlg, IDSL_TRC_GRAYMOPH), TB_THUMBPOSITION, gstTrcPrm.dGrayMoph);
    TraceOnScroll(hDlg, GetDlgItem(hDlg, IDSL_TRC_ZOOM), TB_THUMBPOSITION, gstTrcPrm.dZooming);
    TraceOnScroll(hDlg, GetDlgItem(hDlg, IDSL_TRC_TURN), TB_THUMBPOSITION, gstTrcPrm.dTurning);
}

static LRESULT CALLBACK TraceValueEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                                   UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    UNREFERENCED_PARAMETER(dwRefData);

    switch (message)
    {
    case WM_GETDLGCODE:
        if (lParam)
        {
            const MSG *pMsg = reinterpret_cast<const MSG *>(lParam);
            if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
            {
                return DefSubclassProc(hWnd, message, wParam, lParam) | DLGC_WANTMESSAGE;
            }
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_RETURN)
        {
            SetFocus(GetParent(hWnd));
            return 0;
        }
        break;

    case WM_CHAR:
        if (wParam == VK_RETURN)
            return 0;
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, TraceValueEditSubclassProc, uIdSubclass);
        break;

    default:
        break;
    }

    return DefSubclassProc(hWnd, message, wParam, lParam);
}

static INT_PTR CALLBACK TraceCtrlDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT id;
    HDC hdc;
    HWND hWndChild;
    RECT rect;
    COLORREF caretColour;
#ifdef TRC_SCROLLBAR
    SCROLLINFO stSclInfo;
#endif

    switch (message)
    {
    default:
        break;

    case WM_LBUTTONDOWN:
        // 다이얼로그 빈 공간 클릭 시 에딧 컨트롤 포커스 해제
        SetFocus(hDlg);
        return static_cast<INT_PTR>(TRUE);

    case WM_INITDIALOG:
        gMoziClrBrush = CreateSolidBrush(gstTrcPrm.dMoziColour);
        TraceSubclassValueEdits(hDlg);

#ifdef TRC_SCROLLBAR
        ZeroMemory(&stSclInfo, sizeof(SCROLLINFO));
        stSclInfo.cbSize = sizeof(SCROLLINFO);
        stSclInfo.fMask = SIF_RANGE;
#endif
        TraceInitScrollControl(hDlg, IDSL_TRC_HRIZ_POS, TRC_POSITION_RANGE, gstTrcPrm.stOffsetPt.x, &stSclInfo);
        TraceInitScrollControl(hDlg, IDSL_TRC_VART_POS, TRC_POSITION_RANGE, gstTrcPrm.stOffsetPt.y, &stSclInfo);
        TraceInitScrollControl(hDlg, IDSL_TRC_CONTRAST, TRC_CONTRA_RANGE, gstTrcPrm.dContrast, &stSclInfo);
        TraceInitScrollControl(hDlg, IDSL_TRC_GAMMA, TRC_GAMMA_RANGE, gstTrcPrm.dGamma, &stSclInfo);
        TraceInitScrollControl(hDlg, IDSL_TRC_GRAYMOPH, TRC_GRAYMOPH_RANGE, gstTrcPrm.dGrayMoph, &stSclInfo);
        TraceInitScrollControl(hDlg, IDSL_TRC_ZOOM, TRC_ZOOM_RANGE, gstTrcPrm.dZooming, &stSclInfo);
        TraceInitScrollControl(hDlg, IDSL_TRC_TURN, TRC_TURN_RANGE, gstTrcPrm.dTurning, &stSclInfo);

        CheckDlgButton(hDlg, IDCB_TRC_IMG_UPSET, gstTrcPrm.bUpset);
        CheckDlgButton(hDlg, IDCB_TRC_IMG_MIRROR, gstTrcPrm.bMirror);
        ViewRedrawSetLine(-1);
        return static_cast<INT_PTR>(TRUE);

    case WM_CTLCOLORSTATIC:
        hdc = (HDC)wParam;
        hWndChild = (HWND)lParam;
        id = GetDlgCtrlID(hWndChild);
        if (IDPL_TRC_MOZICOLOUR == id)
        {
            GetClientRect(hWndChild, &rect);
            FillRect(hdc, &rect, gMoziClrBrush);
            return static_cast<INT_PTR>(TRUE);
        }
        break;

    case WM_VSCROLL:
    case WM_HSCROLL:
        return TraceOnScroll(hDlg, (HWND)lParam, static_cast<UINT>(LOWORD(wParam)), static_cast<INT>(static_cast<SHORT>(HIWORD(wParam))));

    case WM_COMMAND:
        id = LOWORD(wParam);
        if (HIWORD(wParam) == EN_KILLFOCUS)
        {
            if (FindTraceValueBinding(id))
            {
                TraceApplyEditValue(hDlg, id);
                return static_cast<INT_PTR>(TRUE);
            }
        }
        switch (id)
        {
        case IDB_TRC_IMAGEOPEN:
            TraceImageFileOpen(hDlg);
            return static_cast<INT_PTR>(TRUE);
        case IDPL_TRC_MOZICOLOUR:
            TraceMoziColourChoice(hDlg);
            return static_cast<INT_PTR>(TRUE);
        case IDCB_TRC_IMG_UPSET:
            gstTrcPrm.bUpset = IsDlgButtonChecked(hDlg, IDCB_TRC_IMG_UPSET);
            ViewRedrawSetLine(-1);
            break;
        case IDCB_TRC_IMG_MIRROR:
            gstTrcPrm.bMirror = IsDlgButtonChecked(hDlg, IDCB_TRC_IMG_MIRROR);
            ViewRedrawSetLine(-1);
            break;
        case IDB_TRC_VIEWTOGGLE:
            gbOnView = gbOnView ? FALSE : TRUE;
            SetDlgItemText(hDlg, IDB_TRC_VIEWTOGGLE, gbOnView ? TEXT("숨기기") : TEXT("보이기"));
            ViewRedrawSetLine(-1);
            ViewFocusSet();
            break;
        case IDB_TRC_RESET:
            TraceResetDialogState(hDlg);
            gbOnView = TRUE;
            ViewRedrawSetLine(-1);
            ViewFocusSet();
            break;
        default:
            break;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        ghTraceDlg = nullptr;
        ViewMoziColourGet(&caretColour);
        ViewCaretReColour(caretColour);
        ViewRedrawSetLine(-1);
        DeleteBrush(gMoziClrBrush);
        MenuItemCheckOnOff(IDM_TRACE_MODE_ON, FALSE);
        return static_cast<INT_PTR>(TRUE);

    case WM_DESTROY:
        InitTraceValue(INIT_SAVE, &gstTrcPrm);
        ViewFocusSet();
        AppTitleTrace(FALSE);
        return static_cast<INT_PTR>(TRUE);
    }

    return static_cast<INT_PTR>(FALSE);
}

HRESULT TraceImageFileOpen(HWND hDlg)
{
    BITMAPINFOHEADER stBIH{};
    OPENFILENAME stOpenFile{};
    BOOLEAN bOpened;
    TCHAR atFilePath[MAX_PATH]{};
    TCHAR atFileName[MAX_STRING]{};

    if (ghTraceImageOpenDlg && IsWindow(ghTraceImageOpenDlg))
    {
        SetForegroundWindow(ghTraceImageOpenDlg);
        return S_FALSE;
    }

    stOpenFile.lStructSize = sizeof(OPENFILENAME);
    stOpenFile.hInstance = GetModuleHandle(nullptr);
    stOpenFile.hwndOwner = hDlg;
    stOpenFile.lpstrFilter = TEXT("이미지 파일 ( bmp, png, jpg, gif, webp )\0*.bmp;*.png;*.jpg;*.jpeg;*.jpe;*.gif;*.webp\0\0");
    stOpenFile.lpstrFile = atFilePath;
    stOpenFile.nMaxFile = MAX_PATH;
    stOpenFile.lpstrFileTitle = atFileName;
    stOpenFile.nMaxFileTitle = MAX_STRING;
    stOpenFile.lpstrTitle = TEXT("이미지 파일을 선택해주세요");
    stOpenFile.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_ENABLESIZING;
    stOpenFile.lpfnHook = ImageOpenDlgHookProc;
    stOpenFile.lpTemplateName = MAKEINTRESOURCE(IDD_IMAGE_OPEN_DLG);

    bOpened = GetOpenFileName(&stOpenFile);
    if (!bOpened)
        return E_ABORT;

    if (ghImgDib)
    {
        DeleteDIB(ghImgDib);
        ghImgDib = nullptr;
    }
    if (ghOrigDib)
    {
        DeleteDIB(ghOrigDib);
        ghOrigDib = nullptr;
    }

    ghOrigDib = LoadImageDIB(atFilePath);
    if (!ghOrigDib)
        return E_HANDLE;

    ghImgDib = CopyDIB(ghOrigDib);
    stBIH.biSize = sizeof(BITMAPINFOHEADER);
    HeadDIB(ghImgDib, &stBIH);
    gstImgSize.cx = stBIH.biWidth;
    gstImgSize.cy = stBIH.biHeight;
    TraceRedrawIamge();
    UNREFERENCED_PARAMETER(hDlg);
    return S_OK;
}

HRESULT TraceMoziColourChoice(HWND hDlg)
{
    CHOOSECOLOR stChsColour{};
    COLORREF adColours[16]{};

    adColours[0] = gstTrcPrm.dMoziColour;
    stChsColour.lStructSize = sizeof(CHOOSECOLOR);
    stChsColour.hwndOwner = hDlg;
    stChsColour.rgbResult = gstTrcPrm.dMoziColour;
    stChsColour.lpCustColors = adColours;
    stChsColour.Flags = CC_RGBINIT;

    if (ChooseColor(&stChsColour))
    {
        gstTrcPrm.dMoziColour = stChsColour.rgbResult;
        DeleteBrush(gMoziClrBrush);
        gMoziClrBrush = CreateSolidBrush(gstTrcPrm.dMoziColour);
        ViewCaretReColour(gstTrcPrm.dMoziColour);
        InvalidateRect(GetDlgItem(hDlg, IDPL_TRC_MOZICOLOUR), nullptr, TRUE);
        ViewRedrawSetLine(-1);
        return S_OK;
    }
    return E_ABORT;
}

INT_PTR TraceOnScroll(HWND hDlg, HWND hWndCtl, UINT code, INT pos)
{
    TCHAR atBuffer[SUB_STRING]{};
    const UINT ctlID = GetDlgCtrlID(hWndCtl);
    INT dDigi, dSyou;
    const bool isStepCode = IsTraceScrollStepCode(code);
    const bool isThumbCode = (code == TB_THUMBPOSITION || code == SB_THUMBPOSITION || code == SB_THUMBTRACK);
    const bool isEndCode = (code == TB_ENDTRACK || code == SB_ENDSCROLL);

    if (isStepCode || isEndCode)
    {
#ifdef TRC_SCROLLBAR
        pos = SendMessage(hWndCtl, SBM_GETPOS, 0, 0);
#else
        pos = SendMessage(hWndCtl, TBM_GETPOS, 0, 0);
#endif
    }

    pos = ApplyTraceScrollDelta(ctlID, pos, code);

    if (ghOrigDib && (isStepCode || isThumbCode || isEndCode))
    {
        if (ghImgDib)
            DeleteDIB(ghImgDib);
        ghImgDib = CopyDIB(ghOrigDib);
    }

    switch (ctlID)
    {
    case IDSL_TRC_HRIZ_POS:
        gstTrcPrm.stOffsetPt.x = std::clamp(pos, 0, TRC_POSITION_RANGE);
        StringCchPrintf(atBuffer, SUB_STRING, TEXT("%d"), gstTrcPrm.stOffsetPt.x - TRC_POSITION_OFFSET);
        Edit_SetText(GetDlgItem(hDlg, IDE_TRC_HRIZ_VALUE), atBuffer);
        break;
    case IDSL_TRC_VART_POS:
        gstTrcPrm.stOffsetPt.y = std::clamp(pos, 0, TRC_POSITION_RANGE);
        StringCchPrintf(atBuffer, SUB_STRING, TEXT("%d"), gstTrcPrm.stOffsetPt.y - TRC_POSITION_OFFSET);
        Edit_SetText(GetDlgItem(hDlg, IDE_TRC_VART_VALUE), atBuffer);
        break;
    case IDSL_TRC_CONTRAST:
        gstTrcPrm.dContrast = std::clamp(pos, 0, TRC_CONTRA_RANGE);
        StringCchPrintf(atBuffer, SUB_STRING, TEXT("%d"), gstTrcPrm.dContrast - TRC_CONTRA_OFFSET);
        Edit_SetText(GetDlgItem(hDlg, IDE_TRC_CONTRAST_VALUE), atBuffer);
        break;
    case IDSL_TRC_GAMMA:
        gstTrcPrm.dGamma = std::clamp(((pos + (TRC_GAMMA_STEP / 2)) / TRC_GAMMA_STEP) * TRC_GAMMA_STEP, 0, TRC_GAMMA_RANGE);
        dDigi = gstTrcPrm.dGamma / 1000;
        dSyou = (gstTrcPrm.dGamma % 1000) / 100;
        StringCchPrintf(atBuffer, SUB_STRING, TEXT("%d.%01d"), dDigi, dSyou);
        Edit_SetText(GetDlgItem(hDlg, IDE_TRC_GAMMA_VALUE), atBuffer);
        pos = gstTrcPrm.dGamma;
        break;
    case IDSL_TRC_GRAYMOPH:
        gstTrcPrm.dGrayMoph = std::clamp(pos, 0, TRC_GRAYMOPH_RANGE);
        StringCchPrintf(atBuffer, SUB_STRING, TEXT("%d"), gstTrcPrm.dGrayMoph);
        Edit_SetText(GetDlgItem(hDlg, IDE_TRC_GRAYMOPH_VALUE), atBuffer);
        break;
    case IDSL_TRC_ZOOM:
        gstTrcPrm.dZooming = std::clamp(pos, 0, TRC_ZOOM_RANGE);
        StringCchPrintf(atBuffer, SUB_STRING, TEXT("%d ％"), gstTrcPrm.dZooming + TRC_ZOOM_OFFSET);
        Edit_SetText(GetDlgItem(hDlg, IDE_TRC_ZOOM_VALUE), atBuffer);
        break;
    case IDSL_TRC_TURN:
        gstTrcPrm.dTurning = std::clamp(pos, 0, TRC_TURN_RANGE);
        StringCchPrintf(atBuffer, SUB_STRING, TEXT("%d"), gstTrcPrm.dTurning);
        Edit_SetText(GetDlgItem(hDlg, IDE_TRC_TURN_VALUE), atBuffer);
        break;
    default:
        return static_cast<INT_PTR>(FALSE);
    }

#ifdef TRC_SCROLLBAR
    SendMessage(hWndCtl, SBM_SETPOS, pos, TRUE);
#else
    SendMessage(hWndCtl, TBM_SETPOS, TRUE, pos);
#endif

    if ((isStepCode || isEndCode || isThumbCode) && ghImgDib)
    {
        TraceRedrawIamge();
    }

    if (isEndCode)
        ViewFocusSet();
    return static_cast<INT_PTR>(TRUE);
}

HRESULT TraceRedrawIamge(VOID)
{
    BITMAPINFOHEADER stBIH{};
    const SHORT dBuff = gstTrcPrm.dContrast - TRC_CONTRA_OFFSET;
    ContrastDIB(ghImgDib, dBuff, dBuff, dBuff);
    GammaDIB(ghImgDib, gstTrcPrm.dGamma, gstTrcPrm.dGamma, gstTrcPrm.dGamma);
    GrayDIB(ghImgDib, gstTrcPrm.dGrayMoph);
    TurnDIBBilinear(ghImgDib, gstTrcPrm.dTurning * 1000, 0x00EEEEEE);
    stBIH.biSize = sizeof(BITMAPINFOHEADER);
    HeadDIB(ghImgDib, &stBIH);
    gstImgSize.cx = stBIH.biWidth;
    gstImgSize.cy = stBIH.biHeight;
    ViewRedrawSetLine(-1);
    return S_OK;
}

HRESULT TraceImgViewTglExt(VOID)
{
    if (!ghTraceDlg)
        return E_HANDLE;
    gbOnView = gbOnView ? FALSE : TRUE;
    SetDlgItemText(ghTraceDlg, IDB_TRC_VIEWTOGGLE, gbOnView ? TEXT("숨기기") : TEXT("보이기"));
    ViewRedrawSetLine(-1);
    return S_OK;
}

UINT TraceMoziColourGet(LPCOLORREF pColour)
{
    if (!ghTraceDlg || !pColour || !gbOnView)
        return FALSE;
    *pColour = gstTrcPrm.dMoziColour;
    return TRUE;
}

UINT TraceImageAppear(HDC hdc, INT iScrlX, INT iScrlY)
{
    POINT stBegin;
    SIZE stStretch, stReverse;

    if (!(ghTraceDlg && ghImgDib) || !gbOnView)
        return FALSE;

    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);

    stBegin.x = LINENUM_WID + (gstTrcPrm.stOffsetPt.x - TRC_POSITION_OFFSET) - iScrlX;
    stBegin.y = RULER_AREA + (gstTrcPrm.stOffsetPt.y - TRC_POSITION_OFFSET) - iScrlY;
    stStretch = gstImgSize;
    stStretch.cx *= (gstTrcPrm.dZooming + TRC_ZOOM_OFFSET);
    stStretch.cx /= 100;
    stStretch.cy *= (gstTrcPrm.dZooming + TRC_ZOOM_OFFSET);
    stStretch.cy /= 100;
    stReverse = gstImgSize;
    if (BST_CHECKED == gstTrcPrm.bUpset)
        stReverse.cy *= -1;
    if (BST_CHECKED == gstTrcPrm.bMirror)
        stReverse.cx *= -1;

    DIBtoDCex(hdc, stBegin.x, stBegin.y, stStretch.cx, stStretch.cy,
              ghImgDib, 0, 0, stReverse.cx, stReverse.cy, SRCCOPY);
    return TRUE;
}

static UINT_PTR CALLBACK ImageOpenDlgHookProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND chUseCbxWnd;
    static HWND chPanelWnd;
    static HDIB chThumbDib;

    HWND hWndCtl, hWndChild, hWnd;
    HDC hDC;
    INT idCtrl, id, cx, cy;
    UINT codeNotify, state;
    TCHAR atFile[MAX_PATH], atSpec[MAX_PATH];
    LPOFNOTIFY pstOfNty;
    RECT rect, dlgRect;
    POINT stPoint;
    SIZE stSize;

    switch (message)
    {
    case WM_INITDIALOG:
        ghTraceImageOpenDlg = GetParent(hDlg);
        chThumbDib = nullptr;
        chUseCbxWnd = GetDlgItem(hDlg, IDCB_TRC_DLG_USETHUMB);
        Button_SetCheck(chUseCbxWnd, gbThumbUse ? BST_CHECKED : BST_UNCHECKED);
        chPanelWnd = GetDlgItem(hDlg, IDS_TRC_DLG_THUMBFRAME);
        return static_cast<INT_PTR>(TRUE);

    case WM_CTLCOLORSTATIC:
        hDC = (HDC)wParam;
        hWndChild = (HWND)lParam;
        if (hWndChild == chPanelWnd)
        {
            GetClientRect(chPanelWnd, &rect);
            if (chThumbDib && gbThumbUse)
            {
                SetStretchBltMode(hDC, HALFTONE);
                SetBrushOrgEx(hDC, 0, 0, nullptr);
                DIBtoDCex(hDC, 0, 0, rect.right, rect.bottom, chThumbDib, 0, 0, 0, 0, SRCCOPY);
            }
            else
            {
                FillRect(hDC, &rect, GetStockBrush(WHITE_BRUSH));
            }
        }
        return static_cast<INT_PTR>(FALSE);

    case WM_COMMAND:
        id = LOWORD(wParam);
        hWndCtl = (HWND)lParam;
        codeNotify = HIWORD(wParam);
        UNREFERENCED_PARAMETER(hWndCtl);
        UNREFERENCED_PARAMETER(codeNotify);
        if (IDCB_TRC_DLG_USETHUMB == id)
            gbThumbUse = (BST_CHECKED == Button_GetCheck(chUseCbxWnd));
        return static_cast<INT_PTR>(TRUE);

    case WM_SIZE:
        state = (UINT)wParam;
        cx = (INT)(SHORT)LOWORD(lParam);
        cy = (INT)(SHORT)HIWORD(lParam);
        UNREFERENCED_PARAMETER(state);
        UNREFERENCED_PARAMETER(cx);
        UNREFERENCED_PARAMETER(cy);
        hWnd = GetParent(hDlg);
        GetClientRect(hWnd, &dlgRect);
        GetWindowRect(chPanelWnd, &rect);
        stPoint.x = rect.left;
        stPoint.y = rect.top;
        ScreenToClient(hDlg, &stPoint);
        stSize.cx = (dlgRect.right - stPoint.x) - 8;
        stSize.cy = (dlgRect.bottom - stPoint.y) - 40;
        SetWindowPos(chPanelWnd, HWND_TOP, 0, 0, stSize.cx, stSize.cy, SWP_NOMOVE | SWP_NOZORDER);
        InvalidateRect(chPanelWnd, nullptr, TRUE);
        return static_cast<INT_PTR>(TRUE);

    case WM_NOTIFY:
        idCtrl = (int)wParam;
        pstOfNty = (LPOFNOTIFY)lParam;
        UNREFERENCED_PARAMETER(idCtrl);
        if (CDN_SELCHANGE == pstOfNty->hdr.code)
        {
            CommDlg_OpenSave_GetSpec(pstOfNty->hdr.hwndFrom, atSpec, MAX_PATH);
            CommDlg_OpenSave_GetFilePath(pstOfNty->hdr.hwndFrom, atFile, MAX_PATH);
            if (chThumbDib)
            {
                DeleteDIB(chThumbDib);
                chThumbDib = nullptr;
            }
            chThumbDib = LoadImageDIB(atFile);
            InvalidateRect(chPanelWnd, nullptr, TRUE);
        }
        return static_cast<INT_PTR>(TRUE);

    case WM_DESTROY:
        if (ghTraceImageOpenDlg == GetParent(hDlg))
            ghTraceImageOpenDlg = nullptr;
        if (chThumbDib)
        {
            DeleteDIB(chThumbDib);
            chThumbDib = nullptr;
        }
        return static_cast<INT_PTR>(TRUE);
    }

    return static_cast<INT_PTR>(FALSE);
}