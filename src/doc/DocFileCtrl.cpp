// 문서 파일 열기, 저장, 백업을 담당한다.
#include "DocAppBridgeInternal.h"
#include "DocContextInternal.h"
#include "DocViewBridgeInternal.h"
//-------------------------------------------------------------------------------------------------

extern list<ONEFILE> gltMultiFiles; // 複数ファイル保持
extern FILES_ITR gitFileIt;         // 今見てるファイルの本体
extern INT gixFocusPage;            // 注目中のページ・とりあえず０・０インデックス

extern UINT gbAutoBUmsg; //        自動バックアップメッセージ出すか？

extern UINT gbSaveMsgOn; //        保存メッセージ出すか？

static TCHAR gatBackUpDirty[MAX_PATH];

//-------------------------------------------------------------------------------------------------

INT DocAstSeparatorGetAlloc(FILES_ITR, INT, UINT, LPVOID *);
UINT DocMltSeparatorGetAlloc(LPVOID *);

INT DocUnicode2UTF8(LPVOID *);
//-------------------------------------------------------------------------------------------------

// 該当するファイルは開き済か
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
//-------------------------------------------------------------------------------------------------

// ファイルから読み込む
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
    //    stOpenFile.lpstrInitialDir =
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
//-------------------------------------------------------------------------------------------------

// ファイル名を受けて、オーポン処理する
HRESULT DocDoOpenFile(HWND hWnd, LPTSTR ptFile)
{
    LPARAM dNumber;

    dNumber = DocOpendFileCheck(ptFile);
    if (1 <= dNumber)
    {
        if (SUCCEEDED(DocAppMultiFileTabSelect(dNumber)))
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

    DocAppMultiFileTabAppend(dNumber, ptFile);
    DocAppOpenHistoryLogging(hWnd, ptFile);

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

// インターバルで自動保存
HRESULT DocFileBackup(HWND hWnd)
{
    CONST TCHAR aatExte[3][5] = {{TEXT(".ast")}, {TEXT(".mlt")}, {TEXT(".txt")}};

    TCHAR atFilePath[MAX_PATH], atFileName[MAX_STRING];
    TCHAR atBuffer[MAX_PATH];

    HANDLE hFile;
    DWORD wrote;

    LPTSTR ptExten; //    ファイル名の拡張子
    TCHAR atExBuf[10];

    LPVOID pBuffer; //    文字列バッファ用ポインター
    INT iByteSize, iNullTmt, iCrLf;

    LPVOID pbSplit;
    UINT cbSplSz;
    UINT cbWriteSz;

    INT isAST, isMLT, idExten;

    UINT_PTR iPages, i; //    頁数

    FILES_ITR itFile;

    ZeroMemory(atFilePath, sizeof(atFilePath));
    ZeroMemory(atFileName, sizeof(atFileName));
    ZeroMemory(atBuffer, sizeof(atBuffer));

    // 複数ファイル、各ファイルをセーブするには？
    for (itFile = gltMultiFiles.begin(); itFile != gltMultiFiles.end();
         itFile++)
    {
        iPages = itFile->vcCont.size(); //    総頁数

        if (1 >= iPages)
            isMLT = FALSE;
        else
            isMLT = TRUE;

        isAST = DocAppPageListHasNamedPages(itFile);

        if (isAST)
        {
            idExten = 0;
        } //    AST
        else if (isMLT)
        {
            idExten = 1;
        } //    MLT
        else
        {
            idExten = 2;
        } //    TXT

        StringCchCopy(atBuffer, MAX_PATH, itFile->atFileName);

        if (atBuffer[0] == TEXT('\0')) //    名称未設定状態
        {
            StringCchCopy(atFileName, MAX_STRING, itFile->atDummyName);
        }
        else
        {
            PathStripPath(atBuffer);
            StringCchCopy(atFileName, MAX_STRING, atBuffer);
        }

        //    拡張子を確認・ドット込みだよ～ん
        ptExten = PathFindExtension(
            atFileName); //    拡張子が無いならnullptr、というか末端になる
        if (0 == *ptExten)
        {
            //    拡張子指定がないならそのまま対応のをくっつける
            StringCchCopy(ptExten, 5, aatExte[idExten]);
        }
        else //    既存の拡張子があったら
        {
            StringCchCopy(atExBuf, 10, ptExten);
            CharLower(atExBuf); //    比較のために小文字にしちゃう

            if (isAST) //    ASTは優先的に適用
            {
                if (StrCmp(atExBuf, aatExte[0])) //    もしASTじゃなかったら変更
                {
                    StringCchCopy(ptExten, 5, aatExte[0]);
                }
            }
            else if (isMLT) //    名前無いけど複数頁ならMLTじゃないとダメ
            {
                if (StrCmp(atExBuf, aatExte[1])) //    もしMLTじゃなかったら変更
                {
                    StringCchCopy(ptExten, 5, aatExte[1]);
                }
            }
            //    一枚なら、TXTでもMLTでも気にしなくてよかばい
        }

        StringCchCopy(atFilePath, MAX_PATH, gatBackUpDirty);
        PathAppend(atFilePath, atFileName); //    Backupファイル名

        hFile = CreateFile(atFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            NotifyBalloonExist(TEXT("파일 백업에 실패했습니다."),
                               TEXT("꺄아아아아아악!"), NIIF_ERROR);
            //    gbAutoBUmsg    バックアップ出来なかったメッセージは常に表示がいいか
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

        for (i = 0; iPages > i; i++) //    全頁保存
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
            //    最終頁の末端の改行は不要のはず
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

// ファイルに保存する
HRESULT DocFileSave(HWND hWnd, UINT bStyle)
{
    CONST TCHAR aatExte[3][5] = {{TEXT(".ast")}, {TEXT(".mlt")}, {TEXT(".txt")}};

    SYSTEMTIME stSysTile;
    OPENFILENAME stSaveFile;

    BOOLEAN bOpened;

    TCHAR atFilePath[MAX_PATH], atFileName[MAX_STRING];
    TCHAR atBuffer[MAX_STRING];

    HANDLE hFile;
    DWORD wrote;

    LPTSTR ptExten; //    ファイル名の拡張子
    TCHAR atExBuf[10];

    LPVOID pBuffer; //    文字列バッファ用ポインター
    INT iByteSize, iNullTmt, iCrLf;

    LPVOID pbSplit;
    UINT cbSplSz;
    UINT cbWriteSz;

    INT isAST, isMLT, idExten, mbRslt;
    BOOLEAN bExtChg = FALSE, bLastChg = FALSE;
    BOOLEAN bForceMLT = FALSE;
    BOOLEAN bNoName = FALSE;

    BOOLEAN bUtf8 = TRUE;  //    기본 저장은 UTF-8 무 BOM
    BOOLEAN bUnic = FALSE; //    ユニコードで保存セヨ

    UINT_PTR iPages, i; //    頁数

    ZeroMemory(&stSaveFile, sizeof(OPENFILENAME));

    ZeroMemory(atFilePath, sizeof(atFilePath));
    ZeroMemory(atFileName, sizeof(atFileName));
    ZeroMemory(atBuffer, sizeof(atBuffer));

    //    保存時は常に選択しているファイルを保存

    iPages = DocNowFilePageCount(); //    総頁数
    if (1 >= iPages)
        isMLT = FALSE;
    else
        isMLT = TRUE;

    // 既存の拡張子がASTなら、それを優先する

    isAST = DocAppPageListHasNamedPages(gitFileIt);

    if (isAST)
    {
        idExten = 0;
    } //    AST
    else
    {
        idExten = 1;
    } //    MLT

    GetLocalTime(&stSysTile);

    StringCchCopy(atFilePath, MAX_PATH, (*gitFileIt).atFileName);

    if (0 == (*gitFileIt).atFileName[0])
        bNoName = TRUE;

    //    リネームか、ファイル名が無かったら保存ダイヤログ開く
    if ((bStyle & D_RENAME) || bNoName)
    {
        // ここで FileSaveDialogue を出す
        stSaveFile.lStructSize = sizeof(OPENFILENAME);
        stSaveFile.hwndOwner = hWnd;
        stSaveFile.lpstrFilter =
            TEXT("[UTF8]아스키 아트 파일 ( mlt, ast, txt )\0*.mlt;*.ast;*.txt\0\0");
        stSaveFile.nFilterIndex = 1; //    デフォのフィルタ選択肢
        stSaveFile.lpstrFile = atFilePath;
        stSaveFile.nMaxFile = MAX_PATH;
        stSaveFile.lpstrFileTitle = atFileName;
        stSaveFile.nMaxFileTitle = MAX_STRING;
        //        stSaveFile.lpstrInitialDir =
        stSaveFile.lpstrTitle = TEXT("어떤 이름으로 저장할까요?");
        stSaveFile.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

        bOpened = GetSaveFileName(&stSaveFile);

        DocViewFocusEditor();

        if (!(bOpened))
        {
            return E_ABORT;
        }
        //    キャンセルしてたら何もしない

        bLastChg = TRUE;
    }

    //    拡張子を確認・ドット込みだよ～ん・拡張子の位置のポインタ確保
    ptExten = PathFindExtension(atFilePath); //    拡張子が無いなら末端になる
    if (0 == *ptExten)
    {
        //    拡張子指定がないならそのまま対応のをくっつける
        StringCchCopy(ptExten, 5, aatExte[idExten]);
        bExtChg = TRUE;
    }
    else //    既存の拡張子があったら
    {
        StringCchCopy(atExBuf, 10, ptExten);
        CharLower(atExBuf); //    比較のために小文字にしちゃう

        //    既存の拡張子が、ASTならそれを優先する
        if (!(StrCmp(atExBuf, aatExte[0]))) //    ASTであるなら
        {
            //    AST形式を維持する
            isAST = TRUE;
            isMLT = FALSE;
            idExten = 0;
        }

        //    保存する拡張子がMLTで、既存のASTからリネームなら確認
        if (!(StrCmp(atExBuf, aatExte[1])))
        {
            if (isAST && (bStyle & D_RENAME)) //    既存ASTかつリネームなら
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
                idExten = 1;
                bForceMLT = TRUE;
            }
        }

        if (isAST) //    ASTは優先的に適用
        {
            if (StrCmp(atExBuf, aatExte[0])) //    もしASTじゃなかったら変更
            {
                StringCchCopy(ptExten, 5, aatExte[0]);
                bExtChg = TRUE;
            }
        }
        else if (isMLT) //    名前無いけど複数頁ならMLTじゃないとダメ
        {
            if (StrCmp(atExBuf, aatExte[1])) //    もしMLTじゃなかったら変更
            {
                StringCchCopy(ptExten, 5, aatExte[1]);
                bExtChg = TRUE;
            }
        }
        //    一枚なら、TXTでもMLTでも気にしなくてよかばい
    }

    //    上書きなら直前の状態のバックアップとか取るべき
    //    同じ名前のファイルがあれば、ってことで

    //    オリジナルファイル名に注意
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

    //    本文の取込はユニコードでやる必要がある
    if (bUnic || bUtf8)
    {
        bStyle |= D_UNI;
    }

    for (i = 0; iPages > i; i++) //    全頁保存
    {
        if (isAST) //    ＡＳＴの場合は、頁先頭にタイトルが入ってる
        {
            //    返り値の確保バイト数にはＮＵＬＬターミネータ含んでるので注意
            cbSplSz = DocAstSeparatorGetAlloc(gitFileIt, i, bStyle, &pbSplit);

            if (bUtf8)
            {
                cbSplSz = DocUnicode2UTF8(&pbSplit);
            }
            //    pbSplitの中身を付け替える

            WriteFile(hFile, pbSplit, (cbSplSz - iNullTmt), &wrote, nullptr);
            FREE(pbSplit);
        }
        else //    MLTの場合は、二つ目以降で区切りが必要
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
        //    pBufferの中身を付け替える

        if ((i + 1) == iPages)
        {
            iByteSize -= iCrLf;
        } //    最終頁の末端の改行は不要のはず
        WriteFile(hFile, pBuffer, iByteSize - iNullTmt, &wrote, nullptr);

        FREE(pBuffer);
    }

    SetEndOfFile(hFile);
    CloseHandle(hFile);

    FREE(pbSplit);

    DocModifyContent(FALSE);

    //    なんかメッセージ
    if (bExtChg) //    拡張子変更した場合
    {
        // InitLastOpen( INIT_SAVE, atFilePath );    //    ラストオーポンを書換
        DocAppMultiFileTabRename((*gitFileIt).dUnique, atFilePath);
        DocAppTitleChange(atFilePath);
        StringCchPrintf(atBuffer, MAX_STRING,
                        TEXT("%s 확장자로 파일을 저장했습니다."), aatExte[idExten]);
        NotifyBalloonExist(atBuffer, TEXT("일했꼬꼬"), NIIF_INFO);

        DocAppOpenHistoryLogging(hWnd,
                           atFilePath); //    ファイル名変更したので記録取り直し
    }
    else
    {
        //    20110713    新規かリネームしてたらラストオーポンを書換
        if (bLastChg)
        {
            // InitLastOpen( INIT_SAVE, atFilePath );
            DocAppMultiFileTabRename((*gitFileIt).dUnique, atFilePath);
            DocAppTitleChange(atFilePath);

            DocAppOpenHistoryLogging(hWnd,
                               atFilePath); //    ファイル名変更したので記録取り直し
        }

        if (gbSaveMsgOn)
        {
            NotifyBalloonExist(TEXT("파일을 UTF-8로 저장했습니다."), TEXT("일했꼬꼬"),
                               NIIF_INFO);
        }
    }

    //    頁一覧の書き直し
    if (bForceMLT)
    {
        DocAppPageListRewrite(-1);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ユニコード文字列を受け取って、ＵＴＦ８のアドレスを確保してもどす。
INT DocUnicode2UTF8(LPVOID *pText)
{
    UINT_PTR cchSz;   //    ユニコード用
    INT cbSize, rslt; //    UTF8用
    LPVOID pUtf8;     //    確保

    StringCchLength((LPTSTR)(*pText), STRSAFE_MAX_CCH, &cchSz);

    //    必要バイト数確認
    cbSize = WideCharToMultiByte(CP_UTF8, 0, (LPTSTR)(*pText), -1, nullptr, 0,
                                 nullptr, nullptr);
    TRACE(TEXT("cbSize[%d]"), cbSize);
    pUtf8 = (LPSTR)malloc(cbSize);
    ZeroMemory(pUtf8, cbSize);
    rslt = WideCharToMultiByte(CP_UTF8, 0, (LPTSTR)(*pText), -1, (LPSTR)(pUtf8),
                               cbSize, nullptr, nullptr);
    TRACE(TEXT("rslt[%d]"), rslt);

    FREE(*pText); //    ユニコード文字列のほうは破壊する

    *pText = pUtf8; //    ＵＴＦ８のほうに付け替える

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

// ページ名前をAST区切り付きで確保する・freeは呼んだ方でやる
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
        cbSize = (cchSize + 1) * sizeof(TCHAR); //    nullptrターミネータ

        *pText = (LPTSTR)malloc(cbSize);
        ZeroMemory(*pText, cbSize);
        StringCchCopy((LPTSTR)(*pText), (cchSize + 1), atBuffer);
    }
    else
    {
        cbSize = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, atBuffer,
                                     cchSize, nullptr, 0, nullptr, nullptr);
        cbSize++; //    nullptrターミネータ
        *pText = (LPSTR)malloc(cbSize);
        ZeroMemory(*pText, cbSize);
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, atBuffer, cchSize,
                            (LPSTR)(*pText), cbSize, nullptr, nullptr);
    }

    return cbSize;
}
//-------------------------------------------------------------------------------------------------Yippee-ki-yay!

// 画像で頁を保存・BMPかPNG、JPEGは向いてない
HRESULT DocImageSave(HWND hWnd, UINT bStyle, HFONT hFont)
{
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

    //    とりあえづダミー名前でファイル
    StringCchCopy(atOutName, MAX_PATH, gitFileIt->atFileName);
    //    拡張子より選択を優先するようにしちゃう
    PathRemoveExtension(atOutName); //    拡張子あぼ～ん

    StringCchPrintf(atPart, MIN_STRING, TEXT("_Page%d"), gixFocusPage);
    StringCchCat(atOutName, MAX_PATH, atPart);

    ZeroMemory(&stSaveFile, sizeof(OPENFILENAME));
    stSaveFile.lStructSize = sizeof(OPENFILENAME);
    stSaveFile.hwndOwner = hWnd;
    stSaveFile.lpstrFilter =
        TEXT("PNG 파일 ( *.png )\0*.png\0BMP 파일 ( *.bmp )\0*.bmp\0\0");
    stSaveFile.nFilterIndex = 1; //    デフォのフィルタ選択肢
    stSaveFile.lpstrFile = atOutName;
    stSaveFile.nMaxFile = MAX_PATH;
    stSaveFile.lpstrFileTitle = atFileName;
    stSaveFile.nMaxFileTitle = MAX_STRING;
    //        stSaveFile.lpstrInitialDir =
    stSaveFile.lpstrTitle = TEXT("어떤 이름과 확장자로 저장할까요?");
    stSaveFile.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    bOpened = GetSaveFileName(&stSaveFile);

    DocViewFocusEditor();

    if (!(bOpened))
    {
        return E_ABORT;
    }

    // 파일 저장할 때 PNG를 기본값으로
    switch (stSaveFile.nFilterIndex)
    {
    default:
        bType = ISAVE_PNG;
        break;
    case 2:
        bType = ISAVE_BMP;
        break;
    }

    dLines = DocNowFilePageLineCount(); // DocPageParamGet( nullptr , nullptr );
                                        // //    要るのは行数
    iDotX = DocPageMaxDotGet(-1, -1);
    iDotY = dLines * LINE_HEIGHT;
    //    ちゅっと余裕いれとく
    iDotX += 8;
    iDotY += 8;

    SetRect(&rect, 4, 4, iDotX - 4, iDotY - 4);

    TRACE(TEXT("해상도 %d x %d"), iDotX, iDotY);

    iByteSize =
        DocPageTextGetAlloc(gitFileIt, gixFocusPage, D_UNI, &pBuffer, TRUE);
    ptText = (LPTSTR)pBuffer;
    StringCchLength(ptText, STRSAFE_MAX_CCH, &cchSize);

    //    描画用ビットマップ作成
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
    //    文字列全体を見ていく
    for (caret = 0; cchSize > caret;)
    {
        if (TEXT('\r') == ptText[caret]) //    壱行の終わり
        {
            TextOut(hMemDC, 0, iLine, &(ptText[start]), cchLen);
            cchLen = 0;    //    文字数リセット
            caret += 2;    //    次の行の開始位置
            start = caret; //    開始位置確認

            iLine += LINE_HEIGHT; //    描画Ｙ位置
        }
        else
        {
            cchLen++;
            caret++;
        }
    }
    //    最後の行描画
    TextOut(hMemDC, 0, iLine, &(ptText[start]), cchLen);

    FREE(pBuffer);

    if (SUCCEEDED(ImageFileSaveDC(hMemDC, atOutName, bType)))
    {
        //    せいこう
        TRACE(TEXT("%s 를 저장했어요."), atOutName);
    }
    else
    {
        //    しっぱい
        TRACE(TEXT("%s 저장에 실패했어요."), atOutName);
    }

    SelectBitmap(hMemDC, hOldBmp);
    DeleteBitmap(hBitmap);

    SelectFont(hMemDC, hOldFont);

    DeleteDC(hMemDC);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
