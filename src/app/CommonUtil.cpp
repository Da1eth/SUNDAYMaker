#include "Sunday.h"

typedef struct tagFINDPATTERN
{
    TCHAR cchMozi;
    INT iDistance;

} FINDPATTERN, *LPFINDPATTERN;

VOID WndTagSet(HWND hWnd, LONG_PTR tag)
{
    SetWindowLongPtr(hWnd, GWLP_USERDATA, tag);
}

LONG_PTR WndTagGet(HWND hWnd) { return GetWindowLongPtr(hWnd, GWLP_USERDATA); }

BOOLEAN FileExtensionCheck(LPTSTR ptFile, LPTSTR ptExte)
{
    TCHAR atExBuf[10];
    LPTSTR ptExten;

    ptExten = PathFindExtension(ptFile);
    if (0 == *ptExten)
    {
        return 0;
    }

    StringCchCopy(atExBuf, 10, ptExten);
    CharLower(atExBuf);

    if (StrCmp(atExBuf, ptExte))
    {
        return 0;
    }

    return 1;
}

INT TextViewSizeGet(LPCTSTR ptText, PINT piLine)
{
    UINT_PTR cchSize, i;
    INT xDot, yLine, dMaxDot;

    wstring wString;

    StringCchLength(ptText, STRSAFE_MAX_CCH, &cchSize);

    yLine = 1;
    dMaxDot = 0;
    for (i = 0; cchSize > i; i++)
    {
        if (CC_CR == ptText[i] && CC_LF == ptText[i + 1])
        {
            xDot = ViewStringWidthGet(wString.c_str());
            if (dMaxDot < xDot)
                dMaxDot = xDot;

            wString.clear();
            i++;
            yLine++;
        }
        else if (CC_TAB == ptText[i])
        {
        }
        else
        {
            wString += ptText[i];
        }
    }

    if (1 <= wString.size())
    {
        xDot = ViewStringWidthGet(wString.c_str());
        if (dMaxDot < xDot)
            dMaxDot = xDot;
    }

    if (piLine)
        *piLine = yLine;
    return dMaxDot;
}

LPCTSTR NextLineW(LPCTSTR pt)
{
    while (*pt && *pt != 0x000D)
    {
        pt++;
    }

    if (0x000D == *pt)
    {
        pt++;
        if (0x000A == *pt)
        {
            pt++;
        }
    }

    return pt;
}

LPTSTR NextLineW(LPTSTR pt)
{
    while (*pt && *pt != 0x000D)
    {
        pt++;
    }

    if (0x000D == *pt)
    {
        pt++;
        if (0x000A == *pt)
        {
            pt++;
        }
    }

    return pt;
}

LPSTR NextLineA(LPSTR pt)
{
    while (*pt && *pt != 0x0D)
    {
        pt++;
    }

    if (0x0D == *pt)
    {
        pt++;
        if (0x0A == *pt)
        {
            pt++;
        }
    }

    return pt;
}

LPFINDPATTERN FindTableMake(LPCTSTR ptPattern)
{
    UINT i;
    UINT_PTR dLength;
    LPFINDPATTERN pstPtrn;

    StringCchLength(ptPattern, STRSAFE_MAX_CCH, &dLength);
    pstPtrn = (LPFINDPATTERN)malloc((dLength + 1) * sizeof(FINDPATTERN));
    ZeroMemory(pstPtrn, (dLength + 1) * sizeof(FINDPATTERN));

    for (i = 0; dLength >= i; i++)
    {
        pstPtrn[i].iDistance = dLength;
    }

    while (dLength > 0)
    {
        i = 0;
        while (pstPtrn[i].cchMozi)
        {
            if (pstPtrn[i].cchMozi == *ptPattern)
            {
                break;
            }
            i++;
        }
        pstPtrn[i].cchMozi = *ptPattern;
        pstPtrn[i].iDistance = --dLength;

        ptPattern++;
    }

    return pstPtrn;
}

LPTSTR FindStringProc(LPTSTR ptText, LPTSTR ptPattern, LPINT pdCch)
{
    UINT_PTR dPtrnLen, dLength;
    LPTSTR ptTextEnd;
    INT i, j, k, jump, cch;

    LPFINDPATTERN pstPattern;

    StringCchLength(ptText, STRSAFE_MAX_CCH, &dLength);

    StringCchLength(ptPattern, STRSAFE_MAX_CCH, &dPtrnLen);
    dPtrnLen--;

    ptTextEnd = ptText + dLength - dPtrnLen;

    pstPattern = FindTableMake(ptPattern);

    cch = 0;
    while (ptText < ptTextEnd)
    {
        for (i = dPtrnLen; i >= 0; i--)
        {
            if (ptText[i] != ptPattern[i])
            {
                break;
            }
        }

        if (i < 0)
        {
            FREE(pstPattern);
            *pdCch = cch;
            return (ptText);
        }

        k = 0;
        while (pstPattern[k].cchMozi)
        {
            if (pstPattern[k].cchMozi == ptText[i])
            {
                break;
            }
            k++;
        }
        j = pstPattern[k].iDistance - (dPtrnLen - i);
        jump = (0 < j) ? j : 2;
        ptText += jump;
        cch += jump;
    }

    FREE(pstPattern);

    *pdCch = 0;

    return (nullptr);
}

LRESULT ExceptionMessage(LPCSTR pcExpMsg, LPCSTR pcFuncName, UINT dLine,
                         LPARAM lReturn)
{
    CHAR acMessage[BIG_STRING];

    StringCchPrintfA(acMessage, BIG_STRING,
                     ("오류 발생＜%s＞[%s:%u]\r\n프로그램을 종료합니다."),
                     pcExpMsg, pcFuncName, dLine);

    MessageBoxA(GetDesktopWindow(), acMessage, ("치명적인 오류 발생"), MB_OK);

    return lReturn;
}

