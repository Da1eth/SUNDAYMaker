// 문서 데이터 접근과 기본 상태 계산을 담당한다.
#include "DocAppBridgeInternal.h"
#include "DocContextInternal.h"
#include "DocViewBridgeInternal.h"
//-------------------------------------------------------------------------------------------------

#define PAGE_LINE_MAX 80
#define LINE_MOZI_MAX 255
//-------------------------------------------------------------------------------------------------

EXTERNED list<ONEFILE> gltMultiFiles; // 複数ファイル保持

static LPARAM gdNextNumber; // 開いたファイルの通し番号・常にインクリ

EXTERNED FILES_ITR gitFileIt; // 今見てるファイルの本体

EXTERNED INT gixFocusPage; // 注目中のページ・とりあえず０・０インデックス

EXTERNED INT gixDropPage; // 投下ホット番号

extern UINT gbNoSjisSkipHangul; // 한글을 NOSJIS 색칠에서 제외할지 여부
extern UINT gbCrLfCode;    //    改行コード：０したらば・非０ＹＹ
//-------------------------------------------------------------------------------------------------

UINT CALLBACK DocPageLoad(LPTSTR, LPCTSTR, INT);
static UINT DocBadSpaceRefreshDirect(LPONELINE);
static UINT DocDelayPageLoadFast(FILES_ITR, INT);
static LPCTSTR DocFileDisplayNameGet(const ONEFILE &);
static INT DocConfirmSaveModified(HWND, const ONEFILE &, BOOLEAN);
static HRESULT DocSyncFocusedPage(INT, INT);
//-------------------------------------------------------------------------------------------------

static HRESULT DocSyncFocusedPage(INT dPageNum, INT iPrePage)
{
    gixFocusPage = dPageNum;
    DocCurrentFile().dNowPage = dPageNum;
    DocCurrentPage().bVisited = TRUE;

    DocDelayPageLoad(gitFileIt, dPageNum);

    return DocAppPageListSelectionChange(dPageNum, iPrePage);
}

//-------------------------------------------------------------------------------------------------

static LPCTSTR DocFileDisplayNameGet(const ONEFILE &stFile)
{
    if (stFile.atFileName[0] != TEXT('\0'))
    {
        return PathFindFileName(stFile.atFileName);
    }

    return stFile.atDummyName;
}

static INT DocConfirmSaveModified(HWND hWnd, const ONEFILE &stFile,
                                  BOOLEAN bClosing)
{
    TCHAR atMessage[BIG_STRING];

    StringCchPrintf(
        atMessage, ARRAYSIZE(atMessage),
        bClosing ? TEXT("\r\n[%s] 파일이 저장되지 않았습니다. \r\n지금 저장하고 종료할까요?")
                 : TEXT("\r\n%s 파일이 저장되지 않았습니다. 지금 저장할까요?"),
        DocFileDisplayNameGet(stFile));

    return MessageBox(hWnd, atMessage, TEXT("꺄아아아아아악"),
                      MB_YESNOCANCEL | MB_ICONQUESTION);
}

static UINT DocBadSpaceRefreshDirect(LPONELINE pstLine)
{
    UINT_PTR iCount;
    UINT iRslt;
    BOOLEAN bWarn;
    TCHAR ch, chn;

    if (!pstLine)
        return 0;

    iCount = pstLine->vcLine.size();
    pstLine->bBadSpace = FALSE;

    if (0 == iCount)
        return 0;

    iRslt = 0;
    bWarn = FALSE;

    for (UINT_PTR i = 0; iCount > i; i++)
    {
        ch = pstLine->vcLine.at(i).cchMozi;
        pstLine->vcLine.at(i).mzStyle &= ~CT_WARNING;

        if (0xFF < ch)
        {
            bWarn = FALSE;
            continue;
        }

        if (isspace(ch))
        {
            if (!(bWarn))
            {
                if ((i + 1) < iCount)
                {
                    chn = pstLine->vcLine.at(i + 1).cchMozi;
                    if (0xFF >= chn && isspace(chn))
                    {
                        pstLine->vcLine.at(i).mzStyle |= CT_WARNING;
                        bWarn = TRUE;
                        iRslt = 1;
                    }
                }
            }
            else
            {
                pstLine->vcLine.at(i).mzStyle |= CT_WARNING;
            }
        }
        else
        {
            bWarn = FALSE;
        }
    }

    if (iswspace(pstLine->vcLine.back().cchMozi))
    {
        iRslt = 1;
    }

    ch = pstLine->vcLine.front().cchMozi;
    if (0xFF >= ch && isspace(ch))
    {
        pstLine->vcLine.front().mzStyle |= CT_WARNING;
        iRslt = 1;
    }

    pstLine->bBadSpace = iRslt;

    return iRslt;
}

static UINT DocDelayPageLoadFast(FILES_ITR itFile, INT iPage)
{
    UINT_PTR cchSize, d;
    ONELINE stLine;
    LINE_ITR itLine;
    auto &stPage = itFile->vcCont.at(iPage);
    LPCTSTR ptRaw = stPage.ptRawData;

    if (!ptRaw)
        return 0;

    StringCchLength(ptRaw, STRSAFE_MAX_CCH, &cchSize);

    stPage.ltPage.clear();
    ZeroONELINE(&stLine);
    stPage.ltPage.push_back(stLine);

    stPage.dByteSz = 0;
    stPage.iMoziCnt = 0;
    stPage.iLineCnt = 1;

    itLine = stPage.ltPage.begin();

    GetHdcC();
    for (d = 0; cchSize > d; d++)
    {
        if (TEXT('\r') == ptRaw[d] && TEXT('\n') == ptRaw[d + 1])
        {
            if (gbCrLfCode)
                stPage.dByteSz += YY2_CRLF;
            else
                stPage.dByteSz += STRB_CRLF;

            ZeroONELINE(&stLine);
            stPage.ltPage.push_back(stLine);
            itLine = stPage.ltPage.end();
            itLine--;
            stPage.iLineCnt++;
            d++;
            continue;
        }

        if (CC_TAB == ptRaw[d])
            continue;

        LETTER stLetter;

        DocLetterDataCheck(&stLetter, ptRaw[d]);
        itLine->vcLine.push_back(stLetter);
        itLine->iDotCnt += stLetter.rdWidth;
        itLine->iByteSz += static_cast<INT>(stLetter.mzByte);

        stPage.dByteSz += static_cast<INT>(stLetter.mzByte);
        stPage.iMoziCnt++;
    }
    ReleaseHdcC();

    for (itLine = stPage.ltPage.begin(); itLine != stPage.ltPage.end();
         itLine++)
    {
        DocBadSpaceRefreshDirect(&(*itLine));
    }

    return 1;
}

// なんか初期化
HRESULT DocInitialise(UINT dMode)
{
    if (dMode) //    作成時
    {
        gdNextNumber = 1;
    }
    else
    {
        for (auto &stFile : gltMultiFiles)
        {
            for (auto &stPage : stFile.vcCont)
            {
                FREE(stPage.ptRawData);
            }
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 変更したか
HRESULT DocModifyContent(UINT dMode)
{
    if (dMode)
    {
        if (DocCurrentFile().dModify)
            return S_FALSE;
        //    変更のとき、已に変更の処理してたら何もしなくて良い

        MainStatusBarSetText(SB_MODIFY, MODIFY_MSG);
    }
    else
    {
        MainStatusBarSetText(SB_MODIFY, TEXT(""));
    }

    DocMultiFileModify(dMode);

    DocCurrentFile().dModify = dMode; //    ここで記録しておく

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 新しいファイル置き場を作ってフォーカスする・ファイルコア函数
LPARAM DocMultiFileCreate(LPTSTR ptDmyName)
{
    ONEFILE stFile;
    FILES_ITR itNew;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        ZeroMemory(stFile.atFileName, sizeof(stFile.atFileName));
        stFile.dModify = FALSE;
        stFile.dNowPage = 0;
        stFile.dUnique = gdNextNumber++;
        stFile.stCaret.x = 0;
        stFile.stCaret.y = 0;

        ZeroMemory(stFile.atDummyName, sizeof(stFile.atDummyName));
        StringCchPrintf(stFile.atDummyName, MAX_PATH, TEXT("%s%d.%s"),
                        NAME_DUMMY_NAME, stFile.dUnique, NAME_DUMMY_EXT);

        if (ptDmyName != nullptr)
        {
            StringCchCopy(ptDmyName, MAX_PATH, stFile.atDummyName);
        }

        stFile.vcCont.clear();

        gltMultiFiles.push_back(stFile);

        //    新規作成の準備
        gixFocusPage = -1;

        DocAppPageListClear();

        itNew = gltMultiFiles.end();
        itNew--; //    末端に追加したからこれでおｋ

        gitFileIt = itNew; //    ファイルなう

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

    return stFile.dUnique;
}
//-------------------------------------------------------------------------------------------------

// 起動時の完全新規作成・開くファイルが全く無い場合の処理
HRESULT DocActivateEmptyCreate(LPTSTR ptFile)
{
    INT iNewPage;

    DocMultiFileCreate(
        ptFile);                  //    新しいファイル置き場の準備・ここで返り血は要らない
    iNewPage = DocPageCreate(-1); //    ページ作っておく
    DocAppPageListInsert(iNewPage);
    DocPageChange(iNewPage);      //    その頁にフォーカスを合わせる
    DocAppMultiFileTabFirst(ptFile);
    DocAppTitleChange(ptFile);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 内容を変更したらタブのファイル名に[変更]つける
HRESULT DocMultiFileModify(UINT dMode)
{
    TCHAR atFile[MAX_PATH]; // ファイル名

    StringCchCopy(atFile, MAX_PATH, DocCurrentFile().atFileName);
    if (0 == atFile[0])
    {
        StringCchCopy(atFile, MAX_PATH, DocCurrentFile().atDummyName);
    }

    PathStripPath(atFile);

    if (dMode)
    {
        StringCchCat(atFile, MAX_PATH, MODIFY_MSG);
    }

    DocAppMultiFileTabRename(DocCurrentFile().dUnique, atFile);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ファイルタブを選択した・ファイルコア函数
HRESULT DocMultiFileSelect(LPARAM uqNumber)
{
    POINT stCaret;

    const auto itNow = std::find_if(
        gltMultiFiles.begin(), gltMultiFiles.end(),
        [uqNumber](const ONEFILE &stFile)
        { return uqNumber == stFile.dUnique; });

    if (itNow == gltMultiFiles.end())
        return E_OUTOFMEMORY;

    DocViewClearSelection();

    DocAppPageListClear();

    gitFileIt = itNow; //    ファイルなう

    DocAppPageListBuild(nullptr);

    DocAppTitleChange(itNow->atFileName);

    DocModifyContent(itNow->dModify); //    変更したかどうか

    DocCaretPosMemory(INIT_LOAD,
                      &stCaret); //    先に読み出さないと次でクルヤーされる

    DocSyncFocusedPage(itNow->dNowPage,
                       -1); //    全部読み込んだのでラストページを表示する

    DocViewResetCaret(stCaret.x, stCaret.y);

    DocAppBackgroundPageLoadRestart(nullptr);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 全内容を破棄・ファイルコア函数
HRESULT DocMultiFileCloseAll(VOID)
{
    for (auto &stFile : gltMultiFiles)
    {
        for (auto &stPage : stFile.vcCont)
        {
            for (auto &stLine : stPage.ltPage)
            {
                stLine.vcLine.clear();
            }
            stPage.ltPage.clear();

            SqnFreeAll(&(stPage.stUndoLog));
        }
        stFile.vcCont.clear();
    }

    gltMultiFiles.clear();

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ファイルタブを閉じるとき・最後の一つは閉じれないようにするか・ファイルコア函数
LPARAM DocMultiFileClose(HWND hWnd, LPARAM uqNumber)
{
    INT iRslt;
    UINT_PTR i, iPage;
    UINT_PTR iCount;
    LPARAM dNowNum, dPrevi;
    FILES_ITR itNow;
    LINE_ITR itLine;

    //    一つしか開いてないなら閉じない
    iCount = gltMultiFiles.size();
    if (1 >= iCount)
        return 0;

    //    対象は今のファイル以外かもしれない。そういうときはそのファイルに移動して処理する。
    //    閉じたら、元ファイルにフォーカスする。開いているファイルを閉じたら、隣のファイルにフォーカスする

    dNowNum = gitFileIt->dUnique; //    今開いてるヤツの番号

    itNow = gltMultiFiles.begin();
    itNow++; //    次のやつの通し番号を確保しておく。
    dPrevi = itNow->dUnique;

    //    閉じたいファイルイテレータを探す
    for (itNow = gltMultiFiles.begin(); itNow != gltMultiFiles.end(); itNow++)
    {
        if (uqNumber == itNow->dUnique)
            break;
        dPrevi = itNow->dUnique;
    }
    if (itNow == gltMultiFiles.end())
        return 0;
    //    もし削除対象が先頭なら、dPreviは次のやつのまま、次以降なら、直前のが入ってるはず
    //    この時点で、itNow は削除するファイルである

    if (dNowNum != uqNumber) //    開いてるファイルと閉じたいファイルが異なるなら
    {
        gixFocusPage = -1;
        DocMultiFileSelect(uqNumber); //    閉じる予定ファイルを開く
        dPrevi = dNowNum;             //    元に戻さにゃ
    }

    //    もし変更が残ってるなら注意を促す
    if (gitFileIt->dModify)
    {
        iRslt = DocConfirmSaveModified(hWnd, *gitFileIt, TRUE);
        if (IDCANCEL == iRslt)
        {
            return 0;
        }

        if (IDYES == iRslt)
        {
            DocFileSave(hWnd, D_SJIS);
        }
    }

    //    DocContentsObliterate内のやつ

    iPage = itNow->vcCont.size();
    for (i = 0; iPage > i; i++)
    {
        for (itLine = itNow->vcCont.at(i).ltPage.begin();
             itLine != itNow->vcCont.at(i).ltPage.end(); itLine++)
        {
            itLine->vcLine.clear(); //    各行の中身全消し
        }
        itNow->vcCont.at(i).ltPage.clear(); //    行を全消し

        FREE(itNow->vcCont.at(i).ptRawData);

        SqnFreeAll(&(itNow->vcCont.at(i).stUndoLog));
    }
    itNow->vcCont.clear(); //    ページを全消し

    gltMultiFiles.erase(itNow); //    本体を消し

    gixFocusPage = -1;
    DocMultiFileSelect(dPrevi); //    元ファイルもしくは隣ファイルを開き直す

    return dPrevi;
}
//-------------------------------------------------------------------------------------------------

// 開いてるタブをもってくる
INT DocMultiFileFetch(INT iTgt, LPTSTR ptFile, LPTSTR ptIniPath)
{
    TCHAR atKeyName[MIN_STRING];
    INT iCount;

    assert(ptIniPath);

    iCount = GetPrivateProfileInt(TEXT("MultiOpen"), TEXT("Count"), 0, ptIniPath);
    if (0 > iTgt)
        return iCount;

    assert(ptFile);

    if (iCount <= iTgt)
    {
        ptFile[0] = TEXT('\0');
        return iCount;
    }
    //    オーバーしてたら無効にして終了

    StringCchPrintf(atKeyName, MIN_STRING, TEXT("Item%u"), iTgt);

    GetPrivateProfileString(TEXT("MultiOpen"), atKeyName, TEXT(""), ptFile,
                            MAX_PATH, ptIniPath);

    return iCount;
}
//-------------------------------------------------------------------------------------------------

// 開いてるタブを記録する・ファイルコア函数
HRESULT DocMultiFileStore(LPTSTR ptIniPath)
{
    TCHAR atKeyName[MIN_STRING], atBuff[MIN_STRING];
    UINT i;
    FILES_ITR itNow;

    assert(ptIniPath);

    //    一旦セクションを空にする
    ZeroMemory(atBuff, sizeof(atBuff));
    WritePrivateProfileSection(TEXT("MultiOpen"), atBuff, ptIniPath);

    //    ファイルを順次記録
    i = 0;
    for (itNow = gltMultiFiles.begin(); itNow != gltMultiFiles.end(); itNow++)
    {
        if (itNow->atFileName[0] != TEXT('\0'))
        {
            StringCchPrintf(atKeyName, MIN_STRING, TEXT("Item%u"), i);
            WritePrivateProfileString(TEXT("MultiOpen"), atKeyName, itNow->atFileName,
                                      ptIniPath);
            i++;
        }
    }

    //    個数を記録
    StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), i);
    WritePrivateProfileString(TEXT("MultiOpen"), TEXT("Count"), atBuff,
                              ptIniPath);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 対象ファイルの名前をゲッツ！する
LPTSTR DocMultiFileNameGet(INT tabNum)
{
    INT i;
    FILES_ITR itNow;

    //    ヒットするまでサーチ
    for (i = 0, itNow = gltMultiFiles.begin(); itNow != gltMultiFiles.end();
         i++, itNow++)
    {
        if (tabNum == i)
            break;
    }
    if (itNow == gltMultiFiles.end())
        return nullptr; //    ヒット無し・アリエナーイ

    //    名無しならダミー名
    if (itNow->atFileName[0] == TEXT('\0'))
    {
        return itNow->atDummyName;
    }

    return itNow->atFileName; //    ファイル名戻す
}
//-------------------------------------------------------------------------------------------------

// Caret位置を常時記録・ファイル切り替えたときに意味がある
VOID DocCaretPosMemory(UINT dMode, LPPOINT pstPos)
{
    if (dMode) //    ロード
    {
        pstPos->x = gitFileIt->stCaret.x;
        pstPos->y = gitFileIt->stCaret.y;
    }
    else //    セーブ
    {
        gitFileIt->stCaret.x = pstPos->x;
        gitFileIt->stCaret.y = pstPos->y;
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// 新しいファイルを開く
HRESULT DocOpenFromNull(HWND hWnd)
{
    LPARAM dNumber;

    TCHAR atDummyName[MAX_PATH];
    //    複数ファイル扱うなら、破棄は不要、新しいファイルインスタンス作って対応

    //    新しいファイル置き場の準備
    dNumber = DocMultiFileCreate(atDummyName); //    ファイルを新規作成するとき

    DocAppMultiFileTabAppend(dNumber,
                       DocCurrentFile().atDummyName); //    ファイルの新規作成した

    DocAppTitleChange(atDummyName);

    gixFocusPage = DocPageCreate(-1);
    DocAppPageListInsert(gixFocusPage);
    DocPageChange(0);

    DocViewRefreshAll();

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ファイル閉じる前に変更を確認
INT DocFileCloseCheck(HWND hWnd)
{
    INT rslt;
    FILES_ITR itFiles;

    //    未保存のファイルをチェキ
    for (itFiles = gltMultiFiles.begin(); itFiles != gltMultiFiles.end();
         itFiles++)
    {
        if (itFiles->dModify)
        {
            rslt = DocConfirmSaveModified(hWnd, *itFiles, FALSE);
            if (IDCANCEL == rslt)
            {
                return 0;
            } //    キャンセルなら終わること自体とりやめ
            if (IDYES == rslt)
            {
                DocFileSave(hWnd, D_SJIS);
            } //    保存するならセーブを呼ぶ
            //    NOなら何もせず次を確認
        }
    }

    return 1;
}
//-------------------------------------------------------------------------------------------------

// ファイルを確保
LPARAM DocFileInflate(LPTSTR ptFileName)
{
    HANDLE hFile;
    DWORD readed;

    LPVOID pBuffer; //    文字列バッファ用ポインター
    INT iByteSize;

    LPTSTR ptString;
    UINT_PTR cchSize;

    LPARAM dNumber;

    // TCHAR    atBuff[10];
    // ZeroMemory( atBuff, sizeof(atBuff) );
    assert(ptFileName); //    ファイル開けないのはバグ

    //    ファイル名が空っぽだったら自動的にアウツ！
    if (0 == ptFileName[0])
    {
        return 0;
    }

    //    レッツオーポン
    hFile = CreateFile(ptFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return 0;
    }

    // InitLastOpen( INIT_SAVE, ptFileName );    //    複数ファイルでは意味が無い

    //    処理順番入替

    iByteSize = GetFileSize(hFile, nullptr);
    pBuffer = malloc(iByteSize + 4); //    バッファは少し大きめに取る
    ZeroMemory(pBuffer, iByteSize + 4);

    SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
    ReadFile(hFile, pBuffer, iByteSize, &readed, nullptr);
    CloseHandle(hFile); //    内容全部取り込んだから開放

    ptString = TextDecodeAutoAlloc(pBuffer, iByteSize, nullptr);
    FREE(pBuffer);
    if (!(ptString))
        return 0;
    pBuffer = ptString;

    StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSize);

    //    新しいファイル置き場の準備    2014/05/28↑にあったのを移動した
    dNumber = DocMultiFileCreate(nullptr); //    実際のファイルを開くとき
    if (0 >= dNumber)
        return 0;

    StringCchCopy(DocCurrentFile().atFileName, MAX_PATH, ptFileName);

    //    もしASTなら、先頭は[AA]になってるはず・分割は中でやる
    if (StrCmpN(AST_SEPARATERW, ptString, 4))
    {
        DocStringSplitMLT(ptString, cchSize, DocPageLoad);
    }
    else
    {
        DocStringSplitAST(ptString, cchSize, DocPageLoad);
    }

    //    ファイル開いたらキャレットとかスクロールをリセットする
    DocViewResetEditState();

    FREE(pBuffer); //    ＝ptString

    DocPageChange(0);           //    全部読み込んだので最初のページを表示する
    DocAppPageListSelectionChange(-1, -1);

    DocAppBackgroundPageLoadRestart(nullptr);

    DocAppTitleChange(ptFileName);

    return dNumber;
}
//-------------------------------------------------------------------------------------------------

// 頁を作って内容をぶち込む
UINT CALLBACK DocPageLoad(LPTSTR ptName, LPCTSTR ptCont, INT cchSize)
{
    gixFocusPage = DocPageCreate(-1); //    頁を作成
    DocAppPageListInsert(gixFocusPage);

    //    新しく作ったページにうつる

    if (ptName)
    {
        DocPageNameSet(ptName);
    } //    名前をセットしておく
    DocCurrentPage().ptRawData = (LPTSTR)malloc((cchSize + 2) * sizeof(TCHAR));
    ZeroMemory(DocCurrentPage().ptRawData, (cchSize + 2) * sizeof(TCHAR));

    // HRESULT hRslt =
    StringCchCopy(DocCurrentPage().ptRawData, (cchSize + 2), ptCont);

    //    バッファに文字列を保存だけしておく

    DocPageParamGet(nullptr, nullptr); //    再計算しちゃう・遅延読込ヒット

    return 1;
}
//-------------------------------------------------------------------------------------------------

// ＭＬＴもしくはＴＸＴなユニコード文字列を受け取って分解しつつページに入れる
UINT DocStringSplitMLT(LPTSTR ptStr, INT cchSize, PAGELOAD pfPageLoad)
{
    LPTSTR ptCaret; //    読込開始・現在位置
    LPTSTR ptEnd;   //    ページの末端位置・セパレータの直前
    UINT iNumber;   //    通し番号カウント
    UINT cchItem;
    BOOLEAN bLast = FALSE;

    ptCaret = ptStr; //    まずは最初から

    iNumber = 0; //    通し番号０インデックス
                 //    始点にはセパレータ無いものとみなす。連続するセパレータは、空白内容として扱う

    do
    {
        ptEnd = StrStr(ptCaret, MLT_SEPARATERW); //    セパレータを探す
        if (!ptEnd)                              //    見つからなかったら＝これが最後なら＝nullptr
        {
            ptEnd = ptStr + cchSize; //    WCHARサイズで計算おｋ？
            bLast = TRUE;
        }
        cchItem = ptEnd - ptCaret; //    WCHAR単位なので計算結果は文字数のようだ

        //    最終頁でない場合は末端の改行分引く
        if (!(bLast) && 0 < cchItem)
        {
            cchItem -= CH_CRLF_CCH;
            ptCaret[cchItem] = 0;
        }

        pfPageLoad(nullptr, ptCaret, cchItem);

        iNumber++;

        ptCaret = NextLineW(ptEnd); //    セパレータの次の行からが本体

    } while (*ptCaret); //    データ有る限りループで探す

    return iNumber;
}
//-------------------------------------------------------------------------------------------------

// ＡＳＴなユニコード文字列を受け取って分解しつつページに入れる
UINT DocStringSplitAST(LPTSTR ptStr, INT cchSize, PAGELOAD pfPageLoad)
{
    LPTSTR ptCaret; //    読込開始・現在位置
    LPTSTR ptStart; //    セパレータの直前
    LPTSTR ptEnd;
    UINT iNumber; //    通し番号カウント
    UINT cchItem;
    BOOLEAN bLast;
    TCHAR atName[MAX_PATH];

    ptCaret = ptStr; //    まずは最初から

    iNumber = 0; //    通し番号０インデックス

    bLast = FALSE;

    do //    とりあえず一番最初はptCaretは[AA]になってる
    {
        ptStart = NextLineW(ptCaret); //    次の行からが本番

        ptCaret += 5;                //    [AA][
        cchItem = ptStart - ptCaret; //    名前部分の文字数
        cchItem -= 3;                //    ]rn

        ZeroMemory(atName, sizeof(atName)); //    名前確保
        if (0 < cchItem)
            StringCchCopyN(atName, MAX_PATH, ptCaret, cchItem);

        ptCaret = ptStart; //    本体部分

        ptEnd = StrStr(ptCaret, AST_SEPARATERW); //    セパレータを探す
        //    この時点でptEndは次の[AA]をさしてる・もしくはnullptr(最後のコマ)
        if (!ptEnd) //    見つからなかったら＝これが最後なら＝nullptr
        {
            ptEnd = ptStr + cchSize; //    WCHARサイズで計算おｋ？
            bLast = TRUE;
        }
        cchItem = ptEnd - ptCaret; //    WCHAR単位なので計算結果は文字数のようだ

        if (!(bLast) && 0 < cchItem) //    最終頁でない場合は末端の改行分引く
        {
            cchItem -= CH_CRLF_CCH;
            ptCaret[cchItem] = 0;
        }

        pfPageLoad(atName, ptCaret, cchItem);

        iNumber++;

        ptCaret = ptEnd;

    } while (*ptCaret); //    データ有る限りループで探す

    return iNumber;
}
//-------------------------------------------------------------------------------------------------

// ＡＳＤなＳＪＩＳ文字列を受け取って分解しつつページに入れる
UINT DocImportSplitASD(LPSTR pcStr, INT cbSize, PAGELOAD pfPageLoad)
{
    // ASDなら、SJISのままで0x01,0x01、0x02,0x02を対応する必要がある
    // 0x01,0x01が改行、0x02,0x02が説明の区切り、アイテム区切りが改行

    LPSTR pcCaret; //    読込開始・現在位置
    LPSTR pcEnd, pcDesc;
    UINT iNumber; //    通し番号カウント
    UINT cbItem, d;
    BOOLEAN bLast;

    LPTSTR ptName, ptCont;
    UINT_PTR cchItem;

    pcCaret = pcStr; //    まずは最初から

    iNumber = 0; //    通し番号０インデックス

    bLast = FALSE;

    do //    とりやえず実行
    {
        pcEnd = NextLineA(pcCaret); //    次の行までで１アイテム
        cbItem = pcEnd - pcCaret;   //    壱行の文字数

        pcDesc = nullptr;
        ptName = nullptr;
        ptCont = nullptr;

        for (d = 0; cbItem > d; d++)
        {
            if ((0x0D == pcCaret[d]) && (0x0A == pcCaret[d + 1])) //    末端であるか
            {
                pcCaret[d] = 0x00; //    末端なのでnullptrにする
                pcCaret[d + 1] = 0x00;

                if (pcDesc)
                {
                    ptName = LegacyEncodedTextDecodeAlloc(pcDesc);
                }

                break;
            }

            //    処理順番注意
            if ((0x01 == pcCaret[d]) && (0x01 == pcCaret[d + 1])) //    改行であるか
            {
                pcCaret[d] = 0x0D;     //    ￥ｒ
                pcCaret[d + 1] = 0x0A; //    ￥ｎ
                d++;                   //    変換したので次に進めるのがよい
            }

            if ((0x02 == pcCaret[d]) &&
                (0x02 == pcCaret[d + 1])) //    アイテムと説明の区切り
            {
                pcDesc = &(pcCaret[d + 2]); //    説明開始位置

                pcCaret[d] = 0x00; //    仕切りなのでnullptrにする
                pcCaret[d + 1] = 0x00;
                d++; //    変換したので次に進めるのがよい
            }
        }

        ptCont = LegacyEncodedTextDecodeAlloc(
            pcCaret); //    호환 텍스트를 유니코드로 바꿔 둔다
        StringCchLength(ptCont, STRSAFE_MAX_CCH, &cchItem);

        pfPageLoad(ptName, ptCont, cchItem);

        iNumber++;

        FREE(ptCont);
        FREE(ptName);

        pcCaret = pcEnd;

    } while (*pcCaret); //    データ有る限りループで探す

    return iNumber;
}
//-------------------------------------------------------------------------------------------------

// 頁名をセットする・ファイルコア函数
HRESULT DocPageNameSet(LPTSTR ptName)
{
    StringCchCopy((*gitFileIt).vcCont.at(gixFocusPage).atPageName, SUB_STRING,
                  ptName);

    DocAppPageListNameSet(gixFocusPage, ptName);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ページ追加処理・ファイルコア函数
INT DocPageCreate(INT iAdding)
{
    INT_PTR iTotal, iNext;
    UINT_PTR iAddPage = 0;
    INT i;

    ONELINE stLine;
    ONEPAGE stPage;

    PAGE_ITR itPage;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        ZeroONELINE(&stLine); //    新規作成したら、壱行目が０文字な枠を作る

        //    こっちもZeroONEPAGEとかにまとめるか
        ZeroMemory(stPage.atPageName, sizeof(stPage.atPageName));
        //    stPage.dDotCnt = 0;
        stPage.dByteSz = 0;
        stPage.ltPage.clear();
        stPage.ltPage.push_back(stLine); //    １頁の枠を作って
        stPage.dSelLineTop = -1;         //    無効は－１を注意
        stPage.dSelLineBottom = -1;      //
        stPage.bVisited = FALSE;
        stPage.ptRawData = nullptr;
        SqnInitialise(&(stPage.stUndoLog));

        //    今の頁の次に作成
        iTotal = DocNowFilePageCount();

        if (0 <= iAdding)
        {
            iNext = iAdding + 1; //    次の頁
            if (iTotal <= iNext)
            {
                iNext = -1;
            } //    全頁より多いなら末端指定
        }
        else
        {
            iNext = -1;
        }

        if (0 > iNext)
        {
            (*gitFileIt).vcCont.push_back(stPage); //    ファイル構造体に追加

            iAddPage = DocNowFilePageCount();
            iAddPage--; //    末端に追加したんだから、個数数えて－１したら０インデックス番号
        }
        else
        {
            itPage = (*gitFileIt).vcCont.begin();
            for (i = 0; iNext > i; i++)
            {
                itPage++;
            }
            (*gitFileIt).vcCont.insert(itPage, stPage);

            iAddPage = iNext;
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

    return iAddPage; //    追加したページ番号
}
//-------------------------------------------------------------------------------------------------

// 頁を削除・ファイルコア函数
HRESULT DocPageDelete(INT iPage, INT iBack)
{
    INT i, iNew;
    PAGE_ITR itPage;

    if (1 >= DocNowFilePageCount())
        return E_ACCESSDENIED;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        //    ここでバックアップを？

        //    街頭位置までイテレータをもっていく
        itPage = (*gitFileIt).vcCont.begin();
        for (i = 0; iPage > i; i++)
        {
            itPage++;
        }

        FREE(itPage->ptRawData);

        SqnFreeAll(&(itPage->stUndoLog));  //    アンドゥログ削除
        (*gitFileIt).vcCont.erase(itPage); //    さっくり削除
        gixFocusPage = -1;                 //    頁選択無効にする

        DocAppPageListDelete(iPage);

        if (0 <= iBack) //    戻り先指定
        {
            iNew = iBack;
        }
        else
        {
            iNew = iPage - 1; //    削除したら一つ前の頁へ
            if (0 > iNew)
                iNew = 0;
        }

        DocPageChange(iNew); //    削除したら頁移動

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return ETC_MSG(err.what(), E_FAIL);
    }
    catch (...)
    {
        return ETC_MSG(("etc error"), E_FAIL);
    }
#endif

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ディレイしてる頁の読込
UINT DocDelayPageLoad(FILES_ITR itFile, INT iPage)
{
    if (itFile->vcCont.at(iPage).ptRawData)
    {
        TRACE(TEXT("지연 로딩 중입니다. [%d]"), iPage);

        DocDelayPageLoadFast(itFile, iPage);

        //    アンドゥは一旦リセットすべき＜頁開けただけなので
        //    変更もONなってたら解除

        //    DocPageParamGet( nullptr, nullptr );    //
        //    再計算しちゃう＜文字追加でやってるので問題無い

        FREE(itFile->vcCont.at(iPage).ptRawData); //    nullptrか否かを見るので注意
        itFile->vcCont.at(iPage).ptRawData = nullptr;

        if (itFile == gitFileIt)
        {
            DocAppPageListRewrite(iPage);
        }

    }
    else
    {
        return 0;
    }

    return 1;
}
//-------------------------------------------------------------------------------------------------

// ページを変更・ファイルコア函数
HRESULT DocPageChange(INT dPageNum)
{
    INT iPrePage;

    //    今の表示内容破棄とかいろいろある
#ifdef DO_TRY_CATCH
    try
    {
#endif

        DocViewClearSelection();

        iPrePage = gixFocusPage;
        DocSyncFocusedPage(dPageNum, iPrePage);

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

// 頁一覧に表示する内容を送る。
HRESULT DocPageInfoRenew(INT dPage, UINT bMode)
{
    UINT_PTR dLines;
    UINT dBytes;
    //    TCHAR        atBuff[SUB_STRING];

    if (0 > dPage)
    {
        dPage = gixFocusPage;
    }

    dBytes = gitFileIt->vcCont.at(dPage).dByteSz;

    if (bMode) //    ステータスバーにバイト数を表示する
    {
        DocAppMainStatusSetByteCount(dBytes);
    }

    if (gitFileIt->vcCont.at(dPage).ptRawData)
    {
        dLines = gitFileIt->vcCont.at(dPage).iLineCnt;
    }
    else
    {
        dLines = gitFileIt->vcCont.at(dPage).ltPage.size();
    }

    DocAppPageListInfoSet(dPage, dBytes, dLines);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 指定行のテキストを指定した文字数からゲットする
INT DocLineDataGetAlloc(INT rdLine, INT iStart, LPLETTER *pstTexts, PINT pchLen,
                        PUINT pdFlag)
{
    INT iSize, i = 0, j, dotCnt;
    INT_PTR iCount, iLines;

    //    始点と終点を使えるようにする    //    －１なら末端

    LINE_ITR itLine;

    iLines = DocNowFilePageLineCount();
    if (iLines <= rdLine)
        return -1;

    itLine = DocCurrentLine(rdLine);

    iCount = itLine->vcLine.size();
    *pdFlag = 0;

    if (0 == iCount) //    文字列の中身がない
    {
        *pchLen = 0;
        dotCnt = 0;
    }
    else
    {
        if (iStart >= iCount)
            return 0; //    通り過ぎた

        iSize = iCount - iStart; //    文字数を入れる
        //    色換えの必要があるところまでとか、一塊ずつで面倒見るように
        *pchLen = iSize;
        iSize++; //    nullptrターミネータの為に増やす

        if (!pstTexts)
            return 0; //    入れるところないならここで終わる

        *pstTexts = (LPLETTER)malloc(iSize * sizeof(LETTER));
        if (!(*pstTexts))
        {
            TRACE(TEXT("malloc error"));
            return 0;
        }

        ZeroMemory(*pstTexts, iSize * sizeof(LETTER));

        dotCnt = 0;
        for (i = iStart, j = 0; iCount > i; i++, j++)
        {
            (*pstTexts)[j].cchMozi = itLine->vcLine.at(i).cchMozi;
            (*pstTexts)[j].rdWidth = itLine->vcLine.at(i).rdWidth;
            (*pstTexts)[j].mzStyle = itLine->vcLine.at(i).mzStyle;

            dotCnt += itLine->vcLine.at(i).rdWidth;
        }

        //    末端がspaceかどうか確認
        if (iswspace(itLine->vcLine.at(iCount - 1).cchMozi))
        {
            *pdFlag |= CT_LASTSP;
        }
    }

    if (iLines - 1 > rdLine)
        *pdFlag |= CT_RETURN; //    次の行があるなら改行
    else
        *pdFlag |= CT_EOF; //    ないならこの行末端がEOF

    //    改行の状態を確保
    *pdFlag |= itLine->dStyle;

    return dotCnt;
}
//-------------------------------------------------------------------------------------------------

// ページ全体を確保する・freeは呼んだ方でやる
INT DocPageGetAlloc(UINT bStyle, LPVOID *pText)
{
    return DocPageTextGetAlloc(gitFileIt, gixFocusPage, bStyle, pText, FALSE);
}
//-------------------------------------------------------------------------------------------------

// ページ全体を文字列で確保する・freeは呼んだ方でやる
INT DocPageTextGetAlloc(FILES_ITR itFile, INT dPage, UINT bStyle, LPVOID *pText,
                        BOOLEAN bCrLf)
{
    UINT_PTR iLines, iLetters, j, iSize;
    UINT_PTR i;
    UINT_PTR cchSize;
    TCHAR atEntity[16];
    LPTSTR ptCaret;

    LPSTR pcStr;

    string srString;
    wstring wsString;

    LINE_ITR itLines;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        srString.clear();
        wsString.clear();

        //    デフォ的な
        if (0 > dPage)
        {
            dPage = gixFocusPage;
        }

        if (itFile->vcCont.at(dPage).ptRawData) //    生データ状態なら
        {
            if (bStyle & D_UNI) //    ユニコードである
            {
                if (bStyle & D_ENTITY)
                {
                    ptCaret = itFile->vcCont.at(dPage).ptRawData;
                    while (*ptCaret)
                    {
                        if (DocBuildCopyEntityText(*ptCaret, atEntity,
                                                   ARRAYSIZE(atEntity)))
                        {
                            wsString += atEntity;
                        }
                        else
                        {
                            wsString += *ptCaret;
                        }
                        ptCaret++;
                    }
                }
                else
                {
                    wsString = wstring(itFile->vcCont.at(dPage).ptRawData);
                }
                if (bCrLf)
                {
                    wsString += wstring(CH_CRLFW);
                }
            }
            else //    ShiftJISである
            {
                pcStr = LegacyEncodedTextEncodeAlloc(
                    itFile->vcCont.at(dPage)
                        .ptRawData); //    페이지 전체를 호환 텍스트로 확보
                if (pcStr)
                {
                    srString = string(pcStr);
                    if (bCrLf)
                    {
                        srString += string(CH_CRLFA);
                    }

                    FREE(pcStr);
                }
            }
        }
        else //    ロード済みなら
        {
            //    全文字を頂く
            iLines = itFile->vcCont.at(dPage).ltPage.size();

            for (itLines = itFile->vcCont.at(dPage).ltPage.begin(), i = 0;
                 itLines != itFile->vcCont.at(dPage).ltPage.end(); itLines++, i++)
            {
                iLetters = itLines->vcLine.size();

                for (j = 0; iLetters > j; j++)
                {
                    srString += string(itLines->vcLine.at(j).acNoSjisText);
                    if ((bStyle & D_UNI) && (bStyle & D_ENTITY) &&
                        DocBuildCopyEntityText(itLines->vcLine.at(j).cchMozi, atEntity,
                                               ARRAYSIZE(atEntity)))
                    {
                        wsString += atEntity;
                    }
                    else
                    {
                        wsString += itLines->vcLine.at(j).cchMozi;
                    }
                }

                if (!(1 == iLines && 0 == iLetters)) //    壱行かつ零文字は空である
                {
                    if (iLines >
                        (i + 1)) //    とりあえずファイル末端改行はここでは付けない
                    {
                        srString += string(CH_CRLFA);
                        wsString += wstring(CH_CRLFW);
                    }
                }
            }

            if (bCrLf)
            {
                srString += string(CH_CRLFA);
                wsString += wstring(CH_CRLFW);
            }
        }

        if (bStyle & D_UNI)
        {
            cchSize = wsString.size() + 1; //    nullptrターミネータ
            iSize =
                cchSize * sizeof(TCHAR); //    ユニコードなのでバイト数は２倍である

            if (pText)
            {
                *pText = (LPTSTR)malloc(iSize);
                ZeroMemory(*pText, iSize);
                StringCchCopy((LPTSTR)(*pText), cchSize, wsString.c_str());
            }
        }
        else
        {
            iSize = srString.size() + 1; //    nullptrターミネータ

            if (pText)
            {
                *pText = (LPSTR)malloc(iSize);
                ZeroMemory(*pText, iSize);
                StringCchCopyA((LPSTR)(*pText), iSize, srString.c_str());
            }
        }

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        return (INT)ETC_MSG(err.what(), 0);
    }
    catch (...)
    {
        return (INT)ETC_MSG(("etc error"), 0);
    }
#endif

    return (INT)iSize;
}
//-------------------------------------------------------------------------------------------------

// 現在頁の指定行を文字列で確保する・freeは呼んだ方でやる・ファイルと頁指定できたほうがいいか
INT DocLineTextGetAlloc(FILES_ITR itFile, INT dPage, UINT bStyle, UINT dTarget,
                        LPVOID *pText)
{
    UINT_PTR dLines;
    UINT_PTR dLetters, j, iSize;
    UINT_PTR cchSize;

    string srString;
    wstring wsString;

    LINE_ITR itLines;

    //    位置確認
    dLines = itFile->vcCont.at(dPage).ltPage.size();
    if (dLines <= dTarget)
    {
        return 0;
    }

    itLines = itFile->vcCont.at(dPage).ltPage.begin();
    std::advance(itLines, dTarget);

    dLetters = itLines->vcLine.size();
    for (j = 0; dLetters > j; j++)
    {
        srString += string(itLines->vcLine.at(j).acNoSjisText);
        wsString += itLines->vcLine.at(j).cchMozi;
    }

    if (bStyle & D_UNI)
    {
        cchSize = wsString.size() + 1;   //    nullptrターミネータ
        iSize = cchSize * sizeof(TCHAR); //    ユニコードなのでバイト数は２倍である

        if (pText)
        {
            *pText = (LPTSTR)malloc(iSize);
            ZeroMemory(*pText, iSize);
            StringCchCopy((LPTSTR)(*pText), cchSize, wsString.c_str());
        }
    }
    else
    {
        iSize = srString.size() + 1; //    nullptrターミネータ

        if (pText)
        {
            *pText = (LPSTR)malloc(iSize);
            ZeroMemory(*pText, iSize);
            StringCchCopyA((LPSTR)(*pText), iSize, srString.c_str());
        }
    }
    return (INT)iSize;
}
//-------------------------------------------------------------------------------------------------

// 한글 문자의 CT_NOSJIS 플래그를 설정에 맞게 갱신한다.
HRESULT HangulNoSjisToggle(VOID)
{
    INT_PTR iPage, iLine, iMozi, dP, dL, dM;

    LINE_ITR itLine;

    iPage = DocNowFilePageCount();

    for (dP = 0; iPage > dP; dP++)
    {
        if ((*gitFileIt).vcCont.at(dP).ptRawData)
            continue;

        iLine = (*gitFileIt).vcCont.at(dP).ltPage.size();

        itLine = (*gitFileIt).vcCont.at(dP).ltPage.begin();
        for (dL = 0; iLine > dL; dL++, itLine++)
        {
            iMozi = itLine->vcLine.size();

            for (dM = 0; iMozi > dM; dM++)
            {
                TCHAR ch = itLine->vcLine.at(dM).cchMozi;
                if (IsHangulChar(ch))
                {
                    if (gbNoSjisSkipHangul)
                        itLine->vcLine.at(dM).mzStyle &= ~CT_NOSJIS;
                    else
                        itLine->vcLine.at(dM).mzStyle |= CT_NOSJIS;
                }
            }
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ホットキーによる投下の調整
HRESULT DocThreadDropCopy(VOID)
{
    CHAR acBuf[260];
    TCHAR atTitle[64], atInfo[256];
    INT cbSize, maxPage; //, dFocusBuf;
    LPVOID pcString = nullptr;

    cbSize =
        DocPageTextGetAlloc(gitFileIt, gixDropPage, D_SJIS, &pcString, FALSE);

    TRACE(TEXT("%d 페이지를 복사"), gixDropPage);

    DocClipboardDataSet(pcString, cbSize, D_SJIS);

    ZeroMemory(acBuf, sizeof(acBuf));
    StringCchCopyNA(acBuf, 260, (LPCSTR)pcString, 250);
    ZeroMemory(atInfo, sizeof(atInfo));
    MultiByteToWideChar(CP_ACP, 0, acBuf, (INT)strlen(acBuf), atInfo, 256);

    StringCchPrintf(atTitle, 64, TEXT("%d 페이지를 복사했습니다."),
                    gixDropPage + 1);

    NotifyBalloonExist(atInfo, atTitle, NIIF_INFO);

    FREE(pcString);

    gixDropPage++; //    次の頁へ

    maxPage = DocNowFilePageCount();
    if (maxPage <= gixDropPage)
        gixDropPage = 0;
    //    最終頁までイッたら先頭に戻る

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
