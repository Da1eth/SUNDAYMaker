#include "Sunday.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4996)
#include "stb_image_write.h"
#pragma warning(pop)

#include <memory>

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

    static HDIB CreateDIB(LONG width, LONG height)
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

    static BOOL DeleteDIB(HDIB hDIB)
    {
        if (!hDIB)
            return FALSE;
        free(hDIB->pBits);
        free(hDIB);
        return TRUE;
    }

    static HDIB DCtoDIB(HDC hDC, long x1, long y1, long x2, long y2)
    {
        HDC hMemDC = nullptr;
        HBITMAP hBitmap = nullptr;
        HGDIOBJ hOldBmp = nullptr;
        BITMAPINFO bmi{};
        HDIB hDib = nullptr;
        const LONG width = x2 - x1;
        const LONG height = y2 - y1;

        if (width <= 0 || height <= 0)
            return nullptr;

        hDib = CreateDIB(width, height);
        if (!hDib)
            return nullptr;

        hMemDC = CreateCompatibleDC(hDC);
        if (!hMemDC)
            goto cleanup;

        hBitmap = CreateCompatibleBitmap(hDC, width, height);
        if (!hBitmap)
            goto cleanup;

        hOldBmp = SelectObject(hMemDC, hBitmap);
        BitBlt(hMemDC, 0, 0, width, height, hDC, x1, y1, SRCCOPY);

        bmi.bmiHeader = hDib->bih;
        if (!GetDIBits(hMemDC, hBitmap, 0, height, hDib->pBits, &bmi, DIB_RGB_COLORS))
            goto cleanup;

        SelectObject(hMemDC, hOldBmp);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        return hDib;

    cleanup:
        if (hOldBmp)
            SelectObject(hMemDC, hOldBmp);
        if (hBitmap)
            DeleteObject(hBitmap);
        if (hMemDC)
            DeleteDC(hMemDC);
        if (hDib)
            DeleteDIB(hDib);
        return nullptr;
    }

    static BOOL WriteDIBWithStb(HDIB hDIB, LPCSTR lpFileName, bool isPng)
    {
        if (!hDIB || !lpFileName)
            return FALSE;

        const LONG width = hDIB->bih.biWidth;
        const LONG height = GetDIBHeight(hDIB);
        const int outStride = width * 3;
        auto rgb = std::make_unique<unsigned char[]>(static_cast<size_t>(outStride) * height);
        if (!rgb)
            return FALSE;

        for (LONG y = 0; y < height; y++)
        {
            const BYTE *src = GetDIBRow(hDIB, y);
            unsigned char *dst = rgb.get() + static_cast<size_t>(y) * outStride;
            for (LONG x = 0; x < width; x++)
            {
                dst[x * 3 + 0] = src[x * 3 + 2];
                dst[x * 3 + 1] = src[x * 3 + 1];
                dst[x * 3 + 2] = src[x * 3 + 0];
            }
        }

        const int result = isPng
                               ? stbi_write_png(lpFileName, width, height, 3, rgb.get(), outStride)
                               : stbi_write_bmp(lpFileName, width, height, 3, rgb.get());
        return result != 0;
    }

    static BOOL DIBtoBMP(HDIB hDIB, LPCSTR lpFileName)
    {
        return WriteDIBWithStb(hDIB, lpFileName, false);
    }

    static BOOL DIBtoPNG(HDIB hDIB, LPCSTR lpFileName)
    {
        return WriteDIBWithStb(hDIB, lpFileName, true);
    }

    static HRESULT SaveDCRect(HDC hDC, const RECT &rect, const CHAR *pcMode, const CHAR *pcFilePath)
    {
        HRESULT hr = E_FAIL;
        HDIB hDib = DCtoDIB(hDC, rect.left, rect.top, rect.right, rect.bottom);
        if (!hDib)
            return E_OUTOFMEMORY;

        if (lstrcmpiA(pcMode, "png") == 0)
        {
            hr = DIBtoPNG(hDib, pcFilePath) ? S_OK : E_FAIL;
        }
        else
        {
            hr = DIBtoBMP(hDib, pcFilePath) ? S_OK : E_FAIL;
        }

        DeleteDIB(hDib);
        return hr;
    }

} // namespace

HRESULT ImageFileSaveDC(HDC hDC, LPTSTR ptFilePath, INT bType)
{
    BITMAP bitmap{};
    RECT rect{};
    HGDIOBJ hBitmap = GetCurrentObject(hDC, OBJ_BITMAP);
    CHAR acFilePath[MAX_PATH]{};
    const CHAR *pcMode = (bType == ISAVE_BMP) ? "bmp" : "png";

    if (!hBitmap)
        return E_HANDLE;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bitmap))
        return E_FAIL;

    rect.left = 0;
    rect.top = 0;
    rect.right = bitmap.bmWidth;
    rect.bottom = bitmap.bmHeight;

    WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, ptFilePath, -1, acFilePath, MAX_PATH, nullptr, nullptr);

    return SaveDCRect(hDC, rect, pcMode, acFilePath);
}
