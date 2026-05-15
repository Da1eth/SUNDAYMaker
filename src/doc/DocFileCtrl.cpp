// 문서 파일 열기, 저장, 백업을 담당한다.
#include "DocAppBridgeInternal.h"
#include "DocContextInternal.h"
#include "DocViewBridgeInternal.h"

extern list<ONEFILE> gltMultiFiles; // 열린 파일 목록
extern FILES_ITR gitFileIt;         // 현재 보고 있는 파일
extern INT gixFocusPage;            // 현재 선택된 페이지 인덱스

extern UINT gbAutoBUmsg; // 자동 백업 알림 표시 여부

extern UINT gbSaveMsgOn; // 저장 알림 표시 여부

static TCHAR gatBackUpDirty[MAX_PATH];

INT DocAstSeparatorGetAlloc(FILES_ITR, INT, UINT, LPVOID *);
UINT DocMltSeparatorGetAlloc(LPVOID *);

INT DocUnicode2UTF8(LPVOID *);

namespace
{

    enum DocSaveExtensionIndex
    {
        DOC_EXT_AST = 0,
        DOC_EXT_MLT = 1,
        DOC_EXT_TXT = 2,
    };

    constexpr TCHAR gatDocSaveExtensions[3][5] = {
        TEXT(".ast"),
        TEXT(".mlt"),
        TEXT(".txt"),
    };

    INT DocImageSaveTypeFromFilter(UINT dFilterIndex)
    {
        if (dFilterIndex == 2)
            return ISAVE_BMP;

        return ISAVE_PNG;
    }

    LPCTSTR DocImageExtensionFromType(INT bType)
    {
        if (bType == ISAVE_BMP)
            return TEXT(".bmp");

        return TEXT(".png");
    }

    VOID DocAppendExtensionIfMissing(LPTSTR ptFilePath, size_t cchFilePath,
                                     LPCTSTR ptExtension)
    {
        if (!ptFilePath || !ptExtension)
            return;

        if (PathFindExtension(ptFilePath)[0] != TEXT('\0'))
            return;

        StringCchCat(ptFilePath, cchFilePath, ptExtension);
    }

} // namespace

// 이미 열린 파일인지 확인한다.
LPARAM DocOpendFileCheck(LPTSTR ptFile)
{
    for (const auto &stFile : gltMultiFiles)
    {
        if (StrCmp(stFile.atFileName, ptFile) == 0)
        {
            return stFile.dUnique;
        }
    }

    return -1;
}

// 파일 열기 대화상자를 띄운다.
HRESULT DocFileOpen(HWND hWnd)
{
    OPENFILENAME stOpenFile;
    BOOLEAN bOpened;
    TCHAR atFilePath[MAX_PATH], atFileName[MAX_STRING];

    ZeroMemory(&stOpenFile, sizeof(OPENFILENAME));

    ZeroMemory(atFilePath, sizeof(atFilePath));
    ZeroMemory(atFileName, sizeof(atFileName));

    stOpenFile.lStructSize = sizeof(OPENFILENAME);
    stOpenFile.hwndOwner = hWnd;
    stOpenFile.lpstrFilter = TEXT(
        "아스키 아트 파일 ( mlt, ast, txt "
        ")\0*.mlt;*.ast;*.txt\0全ての形式(*.*)\0*.*\0\0");
    stOpenFile.nFilterIndex = 1;
    stOpenFile.lpstrFile = atFilePath;
    stOpenFile.nMaxFile = MAX_PATH;
    stOpenFile.lpstrFileTitle = atFileName;
    stOpenFile.nMaxFileTitle = MAX_STRING;
    stOpenFile.lpstrTitle = TEXT("어떤 파일을 불러올까요?");
    stOpenFile.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
    stOpenFile.lpstrDefExt = TEXT("mlt");

    bOpened = GetOpenFileName(&stOpenFile);

    DocViewFocusEditor();

    if (!(bOpened))
        return E_ABORT;

    DocDoOpenFile(hWnd, atFilePath);

    return S_OK;
}

// 파일 경로를 받아 실제 열기 처리를 수행한다.
HRESULT DocDoOpenFile(HWND hWnd, LPTSTR ptFile)
{
    LPARAM dNumber;
    DOC_APP_SHELL_SYNC_REQUEST stSync{};

    dNumber = DocOpendFileCheck(ptFile);
    if (1 <= dNumber)
    {
        stSync.dFlags = DOC_APP_SYNC_FILE_TAB_SELECT;
        stSync.dFileNumber = dNumber;
        if (SUCCEEDED(DocAppShellSync(stSync)))
        {
            DocMultiFileSelect(dNumber);
            return S_OK;
        }
    }

    dNumber = DocFileInflate(ptFile);
    if (!(dNumber))
    {
        MessageBox(hWnd, TEXT("불러오기에 실패했습니다."), TEXT("꺄아아아아아악!"),
                   MB_OK | MB_ICONERROR);
        return E_HANDLE;
    }

    stSync = {};
    stSync.dFlags = DOC_APP_SYNC_FILE_TAB_APPEND |
                    DOC_APP_SYNC_OPEN_HISTORY;
    stSync.dFileNumber = dNumber;
    stSync.hWindow = hWnd;
    stSync.ptText = ptFile;
    DocAppShellSync(stSync);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// バックアップディレクトリーを確保
VOID DocBackupDirectoryInit(LPTSTR ptCurrent)
{
    StringCchCopy(gatBackUpDirty, MAX_PATH, ptCurrent);
    PathAppend(gatBackUpDirty, BACKUP_DIR);
    CreateDirectory(gatBackUpDirty, nullptr);

    return;
}
//-------------------------------------------------------------------------------------------------

// 자동 백업을 수행한다.
HRESULT DocFileBackup(HWND hWnd)
{
    TCHAR atFilePath[MAX_PATH], atFileName[MAX_STRING];
    TCHAR atBuffer[MAX_PATH];

    HANDLE hFile;
    DWORD wrote;

    LPTSTR ptExten;
    TCHAR atExBuf[10];

    LPVOID pBuffer;
    INT iByteSize, iNullTmt, iCrLf;

    LPVOID pbSplit;
    UINT cbSplSz;
    UINT cbWriteSz;

    INT isAST, isMLT, idExten;

    UINT_PTR iPages, i;

    FILES_ITR itFile;

    ZeroMemory(atFilePath, sizeof(atFilePath));
    ZeroMemory(atFileName, sizeof(atFileName));
    ZeroMemory(atBuffer, sizeof(atBuffer));

    for (itFile = gltMultiFiles.begin(); itFile != gltMultiFiles.end();
         itFile++)
    {
        iPages = itFile->vcCont.size();

        if (1 >= iPages)
            isMLT = FALSE;
        else
            isMLT = TRUE;

        isAST = DocAppPageListHasNamedPages(itFile);

        if (isAST)
        {
            idExten = DOC_EXT_AST;
        }
        else if (isMLT)
        {
            idExten = DOC_EXT_MLT;
        }
        else
        {
            idExten = DOC_EXT_TXT;
        }

        StringCchCopy(atBuffer, MAX_PATH, itFile->atFileName);

        if (atBuffer[0] == TEXT('\0'))
        {
            StringCchCopy(atFileName, MAX_STRING, itFile->atDummyName);
        }
        else
        {
            PathStripPath(atBuffer);
            StringCchCopy(atFileName, MAX_STRING, atBuffer);
        }

        ptExten = PathFindExtension(atFileName);
        if (0 == *ptExten)
        {
            StringCchCopy(ptExten, 5, gatDocSaveExtensions[idExten]);
        }
        else
        {
            StringCchCopy(atExBuf, 10, ptExten);
            CharLower(atExBuf);

            if (isAST)
            {
                if (StrCmp(atExBuf, gatDocSaveExtensions[DOC_EXT_AST]))
                {
                    StringCchCopy(ptExten, 5, gatDocSaveExtensions[DOC_EXT_AST]);
                }
            }
            else if (isMLT)
            {
                if (StrCmp(atExBuf, gatDocSaveExtensions[DOC_EXT_MLT]))
                {
                    StringCchCopy(ptExten, 5, gatDocSaveExtensions[DOC_EXT_MLT]);
                }
            }
        }

        StringCchCopy(atFilePath, MAX_PATH, gatBackUpDirty);
        PathAppend(atFilePath, atFileName);

        hFile = CreateFile(atFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            NotifyBalloonExist(TEXT("파일 백업에 실패했습니다."),
                               TEXT("꺄아아아아아악!"), NIIF_ERROR);
            return E_HANDLE;
        }

        iNullTmt = 1;
        iCrLf = CH_CRLF_CCH;
        SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

        if (isAST)
        {
            pbSplit = nullptr;
        }
        else
        {
            cbSplSz = DocMltSeparatorGetAlloc(&pbSplit);
        }

        for (i = 0; iPages > i; i++)
        {
            if (isAST)
            {
                cbSplSz = DocAstSeparatorGetAlloc(itFile, i, D_UNI, &pbSplit);

                cbSplSz = DocUnicode2UTF8(&pbSplit);

                WriteFile(hFile, pbSplit, (cbSplSz - iNullTmt), &wrote, nullptr);
                FREE(pbSplit);
            }
            else
            {
                if (1 <= i)
                {
                    LPVOID pUtf8Split = malloc(cbSplSz);
                    CopyMemory(pUtf8Split, pbSplit, cbSplSz);
                    cbWriteSz = DocUnicode2UTF8(&pUtf8Split);
                    WriteFile(hFile, pUtf8Split, cbWriteSz - 1, &wrote, nullptr);
                    FREE(pUtf8Split);
                }
            }

            iByteSize = DocPageTextGetAlloc(itFile, i, D_UNI, &pBuffer, TRUE);

            iByteSize = DocUnicode2UTF8(&pBuffer);

            if ((i + 1) == iPages)
            {
                iByteSize -= iCrLf;
            }
            WriteFile(hFile, pBuffer, iByteSize - iNullTmt, &wrote, nullptr);

            FREE(pBuffer);
        }

        SetEndOfFile(hFile);
        CloseHandle(hFile);

        FREE(pbSplit);
    }

    if (gbAutoBUmsg)
    {
        NotifyBalloonExist(TEXT("작업 중인 파일을 백업했습니다."),
                           TEXT("백업 완료 안내"), NIIF_INFO);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 현재 선택된 파일을 저장한다.
HRESULT DocFileSave(HWND hWnd, UINT bStyle)
{
    OPENFILENAME stSaveFile;

    BOOLEAN bOpened;

    TCHAR atFilePath[MAX_PATH], atFileName[MAX_STRING];
    TCHAR atBuffer[MAX_STRING];

    HANDLE hFile;
    DWORD wrote;

    LPTSTR ptExten;
    TCHAR atExBuf[10];

    LPVOID pBuffer;
    INT iByteSize, iNullTmt, iCrLf;

    LPVOID pbSplit;
    UINT cbSplSz;
    UINT cbWriteSz;

    INT isAST, isMLT, idExten, mbRslt;
    BOOLEAN bExtChg = FALSE, bLastChg = FALSE;
    BOOLEAN bForceMLT = FALSE;
    BOOLEAN bNoName = FALSE;

    BOOLEAN bUtf8 = TRUE;  // 기본 저장은 UTF-8 무 BOM이다.
    BOOLEAN bUnic = FALSE;

    UINT_PTR iPages, i;

    ZeroMemory(&stSaveFile, sizeof(OPENFILENAME));

    ZeroMemory(atFilePath, sizeof(atFilePath));
    ZeroMemory(atFileName, sizeof(atFileName));
    ZeroMemory(atBuffer, sizeof(atBuffer));

    iPages = DocNowFilePageCount();
    if (1 >= iPages)
        isMLT = FALSE;
    else
        isMLT = TRUE;

    isAST = DocAppPageListHasNamedPages(gitFileIt);

    if (isAST)
    {
        idExten = DOC_EXT_AST;
    }
    else
    {
        idExten = DOC_EXT_MLT;
    }

    StringCchCopy(atFilePath, MAX_PATH, (*gitFileIt).atFileName);

    if (0 == (*gitFileIt).atFileName[0])
        bNoName = TRUE;

    // 이름이 없거나 다른 이름 저장이면 저장 대화상자를 연다.
    if ((bStyle & D_RENAME) || bNoName)
    {
        stSaveFile.lStructSize = sizeof(OPENFILENAME);
        stSaveFile.hwndOwner = hWnd;
        stSaveFile.lpstrFilter =
            TEXT("[UTF8]아스키 아트 파일 ( mlt, ast, txt )\0*.mlt;*.ast;*.txt\0\0");
        stSaveFile.nFilterIndex = 1;
        stSaveFile.lpstrFile = atFilePath;
        stSaveFile.nMaxFile = MAX_PATH;
        stSaveFile.lpstrFileTitle = atFileName;
        stSaveFile.nMaxFileTitle = MAX_STRING;
        stSaveFile.lpstrTitle = TEXT("어떤 이름으로 저장할까요?");
        stSaveFile.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

        bOpened = GetSaveFileName(&stSaveFile);

        DocViewFocusEditor();

        if (!(bOpened))
        {
            return E_ABORT;
        }

        bLastChg = TRUE;
    }

    // 현재 저장 형식에 맞게 확장자를 보정한다.
    ptExten = PathFindExtension(atFilePath);
    if (0 == *ptExten)
    {
        StringCchCopy(ptExten, 5, gatDocSaveExtensions[idExten]);
        bExtChg = TRUE;
    }
    else
    {
        StringCchCopy(atExBuf, 10, ptExten);
        CharLower(atExBuf);

        // 페이지 이름이 있는 파일은 AST를 우선 유지한다.
        if (!(StrCmp(atExBuf, gatDocSaveExtensions[DOC_EXT_AST])))
        {
            isAST = TRUE;
            isMLT = FALSE;
            idExten = DOC_EXT_AST;
        }

        if (!(StrCmp(atExBuf, gatDocSaveExtensions[DOC_EXT_MLT])))
        {
            if (isAST && (bStyle & D_RENAME))
            {
                mbRslt =
                    MessageBox(hWnd,
                               TEXT("MLT 확장자로 저장하면 페이지 이름이 "
                                    "사라집니다.\r\n이대로 저장할까요?"),
                               TEXT("데이터 소실 안내"), MB_OKCANCEL | MB_ICONQUESTION);
                if (IDOK != mbRslt)
                    return E_ABORT;

                isMLT = TRUE;
                isAST = FALSE;
                idExten = DOC_EXT_MLT;
                bForceMLT = TRUE;
            }
        }

        if (isAST)
        {
            if (StrCmp(atExBuf, gatDocSaveExtensions[DOC_EXT_AST]))
            {
                StringCchCopy(ptExten, 5, gatDocSaveExtensions[DOC_EXT_AST]);
                bExtChg = TRUE;
            }
        }
        else if (isMLT)
        {
            if (StrCmp(atExBuf, gatDocSaveExtensions[DOC_EXT_MLT]))
            {
                StringCchCopy(ptExten, 5, gatDocSaveExtensions[DOC_EXT_MLT]);
                bExtChg = TRUE;
            }
        }
    }

    StringCchCopy((*gitFileIt).atFileName, MAX_PATH, atFilePath);

    hFile = CreateFile(atFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        MessageBox(hWnd, TEXT("파일 저장에 실패했습니다."), TEXT("꺄아아아아아악!"),
                   MB_OK | MB_ICONERROR);
        return E_HANDLE;
    }

    iNullTmt = 1;
    iCrLf = CH_CRLF_CCH;
    SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

    if (isAST)
    {
        pbSplit = nullptr;
    }
    else
    {
        cbSplSz = DocMltSeparatorGetAlloc(&pbSplit);
    }

    // UTF-8 저장은 유니코드로 읽어 온 뒤 변환한다.
    if (bUnic || bUtf8)
    {
        bStyle |= D_UNI;
    }

    for (i = 0; iPages > i; i++)
    {
        if (isAST)
        {
            cbSplSz = DocAstSeparatorGetAlloc(gitFileIt, i, bStyle, &pbSplit);

            if (bUtf8)
            {
                cbSplSz = DocUnicode2UTF8(&pbSplit);
            }

            WriteFile(hFile, pbSplit, (cbSplSz - iNullTmt), &wrote, nullptr);
            FREE(pbSplit);
        }
        else
        {
            if (1 <= i)
            {
                LPVOID pUtf8Split = malloc(cbSplSz);
                CopyMemory(pUtf8Split, pbSplit, cbSplSz);
                cbWriteSz = DocUnicode2UTF8(&pUtf8Split);
                WriteFile(hFile, pUtf8Split, cbWriteSz - 1, &wrote, nullptr);
                FREE(pUtf8Split);
            }
            if (bForceMLT)
            {
                DocAstSeparatorGetAlloc(gitFileIt, i, 0, nullptr);
            }
        }

        iByteSize = DocPageTextGetAlloc(gitFileIt, i, bStyle, &pBuffer, TRUE);

        if (bUtf8)
        {
            iByteSize = DocUnicode2UTF8(&pBuffer);
        }

        if ((i + 1) == iPages)
        {
            iByteSize -= iCrLf;
        }
        WriteFile(hFile, pBuffer, iByteSize - iNullTmt, &wrote, nullptr);

        FREE(pBuffer);
    }

    SetEndOfFile(hFile);
    CloseHandle(hFile);

    FREE(pbSplit);

    DocModifyContent(FALSE);

    // 파일명이나 확장자가 바뀌었으면 UI 상태도 함께 갱신한다.
    if (bExtChg)
    {
        DOC_APP_SHELL_SYNC_REQUEST stSync{};
        stSync.dFlags = DOC_APP_SYNC_FILE_TAB_RENAME |
                        DOC_APP_SYNC_TITLE |
                        DOC_APP_SYNC_OPEN_HISTORY;
        stSync.dFileNumber = (*gitFileIt).dUnique;
        stSync.hWindow = hWnd;
        stSync.ptText = atFilePath;
        DocAppShellSync(stSync);

        StringCchPrintf(atBuffer, MAX_STRING,
                        TEXT("%s 확장자로 파일을 저장했습니다."), gatDocSaveExtensions[idExten]);
        NotifyBalloonExist(atBuffer, TEXT("일했꼬꼬"), NIIF_INFO);
    }
    else
    {
        if (bLastChg)
        {
            DOC_APP_SHELL_SYNC_REQUEST stSync{};
            stSync.dFlags = DOC_APP_SYNC_FILE_TAB_RENAME |
                            DOC_APP_SYNC_TITLE |
                            DOC_APP_SYNC_OPEN_HISTORY;
            stSync.dFileNumber = (*gitFileIt).dUnique;
            stSync.hWindow = hWnd;
            stSync.ptText = atFilePath;
            DocAppShellSync(stSync);
        }

        if (gbSaveMsgOn)
        {
            NotifyBalloonExist(TEXT("파일을 저장했습니다."), TEXT("일했꼬꼬"),
                               NIIF_INFO);
        }
    }

    if (bForceMLT)
    {
        DOC_APP_SHELL_SYNC_REQUEST stSync{};
        stSync.dFlags = DOC_APP_SYNC_PAGE_LIST_REWRITE;
        stSync.iPage = -1;
        DocAppShellSync(stSync);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 유니코드 문자열을 UTF-8 버퍼로 바꿔 돌려준다.
INT DocUnicode2UTF8(LPVOID *pText)
{
    UINT_PTR cchSz;
    INT cbSize, rslt;
    LPVOID pUtf8;

    StringCchLength((LPTSTR)(*pText), STRSAFE_MAX_CCH, &cchSz);

    // 필요한 버퍼 크기를 먼저 구한다.
    cbSize = WideCharToMultiByte(CP_UTF8, 0, (LPTSTR)(*pText), -1, nullptr, 0,
                                 nullptr, nullptr);
    pUtf8 = (LPSTR)malloc(cbSize);
    ZeroMemory(pUtf8, cbSize);
    rslt = WideCharToMultiByte(CP_UTF8, 0, (LPTSTR)(*pText), -1, (LPSTR)(pUtf8),
                               cbSize, nullptr, nullptr);

    FREE(*pText);

    *pText = pUtf8;

    return cbSize;
}
//-------------------------------------------------------------------------------------------------

UINT DocMltSeparatorGetAlloc(LPVOID *pText)
{
    const UINT cchSize = MLT_SPRT_CCH + CH_CRLF_CCH + 1;
    const UINT cbSize = cchSize * sizeof(TCHAR);

    if (!(pText))
    {
        return 0;
    }

    *pText = malloc(cbSize);
    ZeroMemory(*pText, cbSize);
    StringCchPrintf((LPTSTR)(*pText), cchSize, TEXT("%s%s"), MLT_SEPARATERW,
                    CH_CRLFW);

    return cbSize;
}
//-------------------------------------------------------------------------------------------------

// 페이지 이름을 AST 구분자와 함께 할당해서 돌려준다.
INT DocAstSeparatorGetAlloc(FILES_ITR itFile, INT dPage, UINT bStyle,
                            LPVOID *pText)
{
    UINT cchSize, cbSize;
    TCHAR atBuffer[MAX_STRING];

    StringCchPrintf(atBuffer, MAX_STRING, TEXT("[AA][%s]\r\n"),
                    itFile->vcCont.at(dPage).atPageName);
    StringCchLength(atBuffer, MAX_STRING, &cchSize);

    if (!(pText))
    {
        ZeroMemory(itFile->vcCont.at(dPage).atPageName, SUB_STRING * sizeof(TCHAR));
        return 0;
    }

    if (bStyle & D_UNI)
    {
        cbSize = (cchSize + 1) * sizeof(TCHAR);

        *pText = (LPTSTR)malloc(cbSize);
        ZeroMemory(*pText, cbSize);
        StringCchCopy((LPTSTR)(*pText), (cchSize + 1), atBuffer);
    }
    else
    {
        cbSize = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, atBuffer,
                                     cchSize, nullptr, 0, nullptr, nullptr);
        cbSize++;
        *pText = (LPSTR)malloc(cbSize);
        ZeroMemory(*pText, cbSize);
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, atBuffer, cchSize,
                            (LPSTR)(*pText), cbSize, nullptr, nullptr);
    }

    return cbSize;
}

// 현재 페이지를 이미지로 저장한다.
HRESULT DocImageSave(HWND hWnd, UINT bStyle, HFONT hFont)
{
    UNREFERENCED_PARAMETER(bStyle);

    LPVOID pBuffer;
    LPTSTR ptText;
    UINT dLines;
    INT iDotX, iDotY, iByteSize, bType;
    UINT_PTR cchSize;
    RECT rect;

    INT iLine;
    UINT_PTR cchLen, start, caret = 0;

    BOOL bOpened;
    OPENFILENAME stSaveFile;

    TCHAR atOutName[MAX_PATH], atFileName[MAX_STRING];
    TCHAR atPart[MIN_STRING];

    HDC hdc, hMemDC;
    HBITMAP hBitmap, hOldBmp;
    HFONT hOldFont;

    // 현재 파일명과 페이지 번호를 기본 저장 이름으로 사용한다.
    StringCchCopy(atOutName, MAX_PATH, gitFileIt->atFileName);
    PathRemoveExtension(atOutName);

    StringCchPrintf(atPart, MIN_STRING, TEXT("_Page%d"), gixFocusPage);
    StringCchCat(atOutName, MAX_PATH, atPart);

    ZeroMemory(&stSaveFile, sizeof(OPENFILENAME));
    stSaveFile.lStructSize = sizeof(OPENFILENAME);
    stSaveFile.hwndOwner = hWnd;
    stSaveFile.lpstrFilter =
        TEXT("PNG 파일 ( *.png )\0*.png\0BMP 파일 ( *.bmp )\0*.bmp\0\0");
    stSaveFile.nFilterIndex = 1;
    stSaveFile.lpstrFile = atOutName;
    stSaveFile.nMaxFile = MAX_PATH;
    stSaveFile.lpstrFileTitle = atFileName;
    stSaveFile.nMaxFileTitle = MAX_STRING;
    stSaveFile.lpstrDefExt = TEXT("png");
    stSaveFile.lpstrTitle = TEXT("어떤 이름과 확장자로 저장할까요?");
    stSaveFile.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    bOpened = GetSaveFileName(&stSaveFile);

    DocViewFocusEditor();

    if (!(bOpened))
    {
        return E_ABORT;
    }

    bType = DocImageSaveTypeFromFilter(stSaveFile.nFilterIndex);
    DocAppendExtensionIfMissing(atOutName, MAX_PATH,
                                DocImageExtensionFromType(bType));

    dLines = DocNowFilePageLineCount();
    iDotX = DocPageMaxDotGet(-1, -1);
    iDotY = dLines * LINE_HEIGHT;

    // 렌더링 여백을 조금 더 준다.
    iDotX += 8;
    iDotY += 8;

    SetRect(&rect, 4, 4, iDotX - 4, iDotY - 4);

    iByteSize =
        DocPageTextGetAlloc(gitFileIt, gixFocusPage, D_UNI, &pBuffer, TRUE);
    ptText = (LPTSTR)pBuffer;
    StringCchLength(ptText, STRSAFE_MAX_CCH, &cchSize);

    // 메모리 DC에 흰 배경으로 페이지를 먼저 그린다.
    hdc = GetDC(hWnd);

    hBitmap = CreateCompatibleBitmap(hdc, iDotX, iDotY);
    hMemDC = CreateCompatibleDC(hdc);

    hOldBmp = SelectBitmap(hMemDC, hBitmap);
    hOldFont = SelectFont(hMemDC, hFont);

    PatBlt(hMemDC, 0, 0, iDotX, iDotY, WHITENESS);

    ReleaseDC(hWnd, hdc);

    iLine = 0;
    cchLen = 0;
    start = 0;
    for (caret = 0; cchSize > caret;)
    {
        if (TEXT('\r') == ptText[caret])
        {
            TextOut(hMemDC, 0, iLine, &(ptText[start]), cchLen);
            cchLen = 0;
            caret += 2;
            start = caret;

            iLine += LINE_HEIGHT;
        }
        else
        {
            cchLen++;
            caret++;
        }
    }
    TextOut(hMemDC, 0, iLine, &(ptText[start]), cchLen);

    FREE(pBuffer);

    if (SUCCEEDED(ImageFileSaveDC(hMemDC, atOutName, bType)))
    {
    }
    else
    {
    }

    SelectBitmap(hMemDC, hOldBmp);
    DeleteBitmap(hBitmap);

    SelectFont(hMemDC, hOldFont);

    DeleteDC(hMemDC);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
