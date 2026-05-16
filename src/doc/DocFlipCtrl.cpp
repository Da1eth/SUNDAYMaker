// 문서 좌우/상하 반전과 치환 파츠 로딩을 담당한다.
#include "DocAppBridgeInternal.h"
#include "DocContextInternal.h"
#include "DocViewBridgeInternal.h"
//-------------------------------------------------------------------------------------------------

// 반전용 파츠는 필요할 때만 로드한다.

#define FLIP_HORIZONTAL 1
#define FLIP_VERTICAL 0

typedef struct tagFLIPREPLACEMENT
{
    TCHAR atSrcStr[MIN_STRING];  // 원본 문자열
    TCHAR atDestStr[MIN_STRING]; // 치환 문자열

} FLIPREPLACEMENT, *LPFLIPREPLACEMENT;
//-------------------------------------------------------------------------------------------------

extern list<ONEFILE> gltMultiFiles; //    複数ファイル保持
extern FILES_ITR gitFileIt;         //    今見てるファイルの本体
extern INT gixFocusPage;            //    注目中のページ・とりあえず０・０インデックス

static vector<FLIPREPLACEMENT> gvcHorizontalFlipParts; // 좌우 반전 치환표
static vector<FLIPREPLACEMENT> gvcVerticalFlipParts;   // 상하 반전 치환표
typedef vector<FLIPREPLACEMENT>::iterator FLIP_REPLACEMENT_ITR;
//-------------------------------------------------------------------------------------------------

HRESULT DocMirrorTranceLine(INT, INT);
HRESULT DocMirrorTranceBox(INT, INT);

HRESULT DocUpsetTranceLine(INT, INT);
HRESULT DocUpsetTranceBox(INT, INT);

LPTSTR SeledTextAlloc(LINE_ITR, PINT, PINT);

HRESULT FlipReplacementLoad(UINT);
UINT FlipReplacementApply(UINT, LPCTSTR, LPTSTR, UINT_PTR);
static vector<FLIPREPLACEMENT> *FlipReplacementTableGet(UINT);
static HRESULT FlipReplacementAppendSwapPair(list<FLIPREPLACEMENT> *, const JSON_FLIP_ENTRY &);
//-------------------------------------------------------------------------------------------------

static vector<FLIPREPLACEMENT> *FlipReplacementTableGet(UINT dMode)
{
    return dMode ? &gvcHorizontalFlipParts : &gvcVerticalFlipParts;
}

static HRESULT FlipReplacementAppendSwapPair(list<FLIPREPLACEMENT> *pParts, const JSON_FLIP_ENTRY &stEntry)
{
    FLIPREPLACEMENT stData;

    if (!(pParts))
        return E_INVALIDARG;

    ZeroMemory(&stData, sizeof(FLIPREPLACEMENT));
    StringCchCopy(stData.atSrcStr, MIN_STRING, stEntry.wsSource.c_str());
    StringCchCopy(stData.atDestStr, MIN_STRING, stEntry.wsTarget.c_str());
    pParts->push_back(stData);

    ZeroMemory(&stData, sizeof(FLIPREPLACEMENT));
    StringCchCopy(stData.atSrcStr, MIN_STRING, stEntry.wsTarget.c_str());
    StringCchCopy(stData.atDestStr, MIN_STRING, stEntry.wsSource.c_str());
    pParts->push_back(stData);

    return S_OK;
}

// 初期化と破棄
HRESULT DocInverseInit(UINT dMode)
{

    if (dMode)
    {
    }
    else
    {
        gvcHorizontalFlipParts.clear();
        gvcVerticalFlipParts.clear();
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// Flip.json에서 반전 치환표를 로드한다.
HRESULT FlipReplacementLoad(UINT dMode)
{
    vector<JSON_FLIP_ENTRY> vcEntries;
    UINT cchSize, cchLen;
    TCHAR atFileName[MAX_PATH];
    vector<FLIPREPLACEMENT> *pLoadedParts;

    UINT_PTR loop;
    list<FLIPREPLACEMENT> ltParts;                      // 치환표 로드 버퍼
    list<FLIPREPLACEMENT>::iterator itParts, itPtPos; // 버퍼 이터레이터

    StringCchCopy(atFileName, MAX_PATH, ExePathGet());
    PathAppend(atFileName, TEMPLATE_DIR);
    PathAppend(atFileName, FLIP_FILE);

    pLoadedParts = FlipReplacementTableGet(dMode);
    if (!(pLoadedParts))
        return E_INVALIDARG;

    if (!(pLoadedParts->empty()))
        return S_FALSE;

    if (FAILED(AppLoadFlipEntries(atFileName, dMode, &vcEntries)))
        return E_HANDLE;
    for (size_t i = 0; vcEntries.size() > i; i++)
    {
        FlipReplacementAppendSwapPair(&ltParts, vcEntries[i]);
    }

    //    文字数の順番に並べ直すべし
    cchSize = 0;

    loop = ltParts.size();
    while (loop) //    全体をみないかん
    {
        itParts = ltParts.begin();                                //    とりあえず１個目
        StringCchLength(itParts->atSrcStr, MIN_STRING, &cchSize); //    文字数確認

        for (itPtPos = ltParts.begin(); ltParts.end() != itPtPos; itPtPos++)
        {
            StringCchLength(itPtPos->atSrcStr, MIN_STRING, &cchLen);
            if (cchSize < cchLen)
            {
                itParts = itPtPos;
            } //    文字数多かったら変更
        }

        FLIPREPLACEMENT stData{};
        StringCchCopy(stData.atSrcStr, MIN_STRING, itParts->atSrcStr);
        StringCchCopy(stData.atDestStr, MIN_STRING, itParts->atDestStr);
        pLoadedParts->push_back(stData);

        ltParts.erase(itParts); //    記録したらそれは消す

        loop = ltParts.size(); //    残りがあるか
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 指定行の選択範囲をテキストで確保する・freeは呼んだ方でやる
LPTSTR SeledTextAlloc(LINE_ITR itLine, PINT piDot, PINT piMozi)
{
    UINT_PTR j, dLetters;
    INT iDot, iMozi;
    INT iSelDot;
    LPTSTR ptString = nullptr;

    UINT_PTR cchSz;

    wstring wsSrcBuff;

    //    その行の選択範囲を確保する
    dLetters = itLine->vcLine.size();

    wsSrcBuff.clear();
    iSelDot = -1; //    選択開始ドットを記録
    iDot = 0;
    iMozi = 0;

    for (j = 0; dLetters > j; j++)
    {
        //    選択されている部分を文字列に確保
        if (CT_SELECT & itLine->vcLine.at(j).mzStyle)
        {
            wsSrcBuff += itLine->vcLine.at(j).cchMozi;

            iMozi++;
            if (0 > iSelDot)
                iSelDot = iDot;
        }

        iDot += itLine->vcLine.at(j).rdWidth; //    そこまでのドット数確認
    }

    cchSz = wsSrcBuff.size();
    ptString = (LPTSTR)malloc((cchSz + 2) * sizeof(TCHAR));
    if (ptString)
        StringCchCopy(ptString, (cchSz + 2), wsSrcBuff.c_str());

    if (0 > iSelDot)
        iSelDot = 0;

    if (piDot)
        *piDot = iSelDot;
    if (piMozi)
        *piMozi = iMozi;

    return ptString;
}
//-------------------------------------------------------------------------------------------------

// 選択範囲のＡＡを反転する
HRESULT DocInverseTransform(UINT dStyle, UINT dMode, PINT pXdot, INT dLine)
{
    INT_PTR iLines;
    INT iTop, iBtm, iInX;

    try
    {

        iLines = DocNowFilePageLineCount(); //    ページ全体の行数

        //    開始地点から開始    //    D_SQUARE
        iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
        iBtm = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
        if (0 > iTop)
        {
            iTop = 0;
        }
        if (0 > iBtm)
        {
            iBtm = iLines - 1;
        }

        //    末端を確認・内容がないなら、使用行戻す
        iInX = DocLineParamGet(iBtm, nullptr, nullptr);
        if (0 == iInX)
        {
            iBtm--;
        }

        //    はみ出しないか
        if (iLines <= iTop || iLines <= iBtm)
            return E_OUTOFMEMORY;

        //    矩形なら、各行毎に反転処理・全体なら、ＭＡＸ幅に合わせる
        if (dStyle & D_SQUARE)
        {
            if (dMode)
            {
                DocMirrorTranceBox(iTop, iBtm);
            } //    左右
            else
            {
                DocUpsetTranceBox(iTop, iBtm);
            } //    上下
        }
        else
        {
            if (dMode)
            {
                DocMirrorTranceLine(iTop, iBtm);
            } //    左右
            else
            {
                DocUpsetTranceLine(iTop, iBtm);
            } //    上下
        }
        DocViewClearSelection();

        DocLetterPosGetAdjust(pXdot, dLine, 0); //    カーソル位置を適当に補正
        DocViewMoveCaret(*pXdot, dLine);

    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 通常選択のときの左右反転処理
HRESULT DocMirrorTranceLine(INT iTop, INT iBtm)
{
    INT_PTR iLns;
    INT iPadd, baseDot, iBytes;
    INT iDot, iGyou, iMzDot;
    LPTSTR ptPadd;
    LPTSTR ptInvStr;
    LPTSTR ptString = nullptr;

    UINT_PTR cchSz;
    UINT d;
    TCHAR atBuff[MIN_STRING];

    BOOLEAN bFirst = TRUE;

    wstring wsInvBuff;

    LINE_ITR itLine;

    try
    {

        FlipReplacementLoad(FLIP_HORIZONTAL); //    左右のパーツ確認

        //    選択範囲内でもっとも長いドット数を確認
        baseDot = DocPageMaxDotGet(iTop, iBtm);

        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, iTop);

        //    左右反転なら、壱行ずつ見ていけばいい
        for (iLns = iTop; iBtm >= iLns; iLns++)
        {
            wsInvBuff.clear();

            iMzDot = DocLineParamGet(iLns, nullptr, nullptr);
            if (0 >= iMzDot)
                continue; //    その行の内容がないなら何もしないでおｋ

            iPadd = baseDot - iMzDot;            //    埋めに必要な幅確保
            ptPadd = DocPaddingSpaceMake(iPadd); //    そこまでを埋める

            //    該当行を確保して
            iBytes = DocLineTextGetAlloc(gitFileIt, gixFocusPage, D_UNI, iLns, (LPVOID *)(&ptString));
            if (0 < iBytes)
            {
                StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSz);
                for (d = 0; cchSz > d;) //    順番に見ていく
                {
                    d += FlipReplacementApply(FLIP_HORIZONTAL, &(ptString[d]), atBuff, MIN_STRING);
                    wsInvBuff.insert(0, atBuff);
                }
            }
            FREE(ptString);

            //    元文字列と入れ替え
            cchSz = wsInvBuff.size() + 2; //    使い回し注意
            ptInvStr = (LPTSTR)malloc(cchSz * sizeof(TCHAR));
            StringCchCopy(ptInvStr, cchSz, wsInvBuff.c_str());

            DocLineErase(iLns, &bFirst); //    先ずその行の内容を削除して
            iDot = 0;
            iGyou = iLns;
            if (ptPadd)
            {
                DocInsertString(&iDot, &iGyou, nullptr, ptPadd, 0, bFirst);
                bFirst = FALSE;
            }
            DocInsertString(&iDot, &iGyou, nullptr, ptInvStr, 0, bFirst);
            bFirst = FALSE;
            //    埋め分を書き込んで、ひっくり返った文字列を書き込めばいい

            FREE(ptPadd);
            FREE(ptInvStr);
        }

        //    末端空白削除が必要

    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 矩形選択のときの左右反転処理
HRESULT DocMirrorTranceBox(INT iTop, INT iBtm)
{
    INT_PTR iLns;
    INT iGyou;
    INT iSelDot, iMozi;
    LPTSTR ptInvStr;
    LPTSTR ptString = nullptr;

    UINT_PTR cchSz;
    UINT d;
    TCHAR atBuff[MIN_STRING];

    BOOLEAN bFirst = TRUE;

    wstring wsInvBuff, wsSrcBuff;

    LINE_ITR itLine;

    try
    {

        FlipReplacementLoad(FLIP_HORIZONTAL); //    左右のパーツ確認

        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, iTop);

        //    左右反転なら、壱行ずつ見ていけばいい
        for (iLns = iTop; iBtm >= iLns; iLns++)
        {
            wsInvBuff.clear();

            //    その行の選択範囲を確保する
            ptString = SeledTextAlloc(itLine, &iSelDot, &iMozi);
            itLine++;
            StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSz);

            //    内容を逆転させる
            if (0 < cchSz) //    その行の中身があったら
            {
                for (d = 0; cchSz > d;)
                {
                    d += FlipReplacementApply(FLIP_HORIZONTAL, &(ptString[d]), atBuff, MIN_STRING);
                    wsInvBuff.insert(0, atBuff);
                }

                //    元内容削除してアンドゥ記録
                DocStringErase(iSelDot, iLns, nullptr, iMozi);
                SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE, ptString, iSelDot, iLns, bFirst);
                bFirst = FALSE;
                FREE(ptString); //    アンドゥ記録してから削除セヨ

                cchSz = wsInvBuff.size() + 2; //    使い回し注意
                ptInvStr = (LPTSTR)malloc(cchSz * sizeof(TCHAR));
                StringCchCopy(ptInvStr, cchSz, wsInvBuff.c_str());

                //    ひっくり返した文字列を挿入
                iGyou = iLns;
                DocInsertString(&iSelDot, &iGyou, nullptr, ptInvStr, 0, FALSE);

                FREE(ptInvStr);
            }
        }

    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 通常選択のときの上下反転処理
HRESULT DocUpsetTranceLine(INT iTop, INT iBtm)
{

    INT_PTR iLns;
    INT iBytes;
    INT iDot, iGyou;
    LPTSTR ptInvStr;
    LPTSTR ptString = nullptr;

    UINT_PTR cchSz, d, dL;
    TCHAR atBuff[MIN_STRING];

    BOOLEAN bFirst = TRUE;

    LINE_ITR itLine;

    wstring wsInvBuff;
    vector<wstring> vcUpset; //    変換結果の一時保存

    try
    {

        FlipReplacementLoad(FLIP_VERTICAL); //    上下のパーツ確認

        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, iTop); //    開始行まで移動

        iLns = (iBtm - iTop) + 1; //    全体行数
        vcUpset.resize(iLns);     //    先に確保しておく

        //    上から処理していく
        for (iLns = iTop, dL = 0; iBtm >= iLns; iLns++, dL++)
        {
            vcUpset.at(dL).clear();

            //    該当行を確保して
            iBytes = DocLineTextGetAlloc(gitFileIt, gixFocusPage, D_UNI, iLns, (LPVOID *)(&ptString));
            if (0 < iBytes)
            {
                //    前から順番に反転文字と入れ替えていく
                StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSz);
                for (d = 0; cchSz > d;)
                {
                    d += FlipReplacementApply(FLIP_VERTICAL, &(ptString[d]), atBuff, MIN_STRING);
                    vcUpset.at(dL) += wstring(atBuff); // wsInvBuff.push_back( atBuff );
                    //    とりあえず順番にいれておいて、書き込む時にreverseすればいい
                }
            }
            FREE(ptString);
        }

        for (iLns = iTop, dL = vcUpset.size() - 1; iBtm >= iLns; iLns++, dL--)
        {
            DocLineErase(iLns, &bFirst); //    先ずその行の内容を削除して

            cchSz = vcUpset.at(dL).size(); //    内容があれば処理する
            if (0 < cchSz)
            {
                cchSz += 2;
                ptInvStr = (LPTSTR)malloc(cchSz * sizeof(TCHAR));
                StringCchCopy(ptInvStr, cchSz, vcUpset.at(dL).c_str());

                iDot = 0;
                iGyou = iLns;
                DocInsertString(&iDot, &iGyou, nullptr, ptInvStr, 0, bFirst);
                bFirst = FALSE;

                FREE(ptInvStr);
            }
        }

    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 矩形選択のときの上下反転処理
HRESULT DocUpsetTranceBox(INT iTop, INT iBtm)
{
    INT_PTR iLns;
    INT iGyou;
    INT iSelDot, iMozi;
    LPTSTR ptInvStr;
    LPTSTR ptString = nullptr;

    UINT_PTR cchSz, d, dL;
    TCHAR atBuff[MIN_STRING];

    BOOLEAN bFirst = TRUE;

    LINE_ITR itLine, itStart;

    vector<wstring> vcUpset; //    変換結果の一時保存

    try
    {
        FlipReplacementLoad(FLIP_VERTICAL); //    上下のパーツ確認

        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, iTop); //    開始行まで移動
        itStart = itLine;

        iLns = (iBtm - iTop) + 1; //    全体行数
        vcUpset.resize(iLns);     //    先に確保しておく

        //    上から壱行ずつ見ていく
        for (iLns = iTop, dL = 0; iBtm >= iLns; iLns++, dL++)
        {
            vcUpset.at(dL).clear();

            //    その行の選択範囲を確保する
            ptString = SeledTextAlloc(itLine, &iSelDot, &iMozi);
            itLine++;
            StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSz);

            //    内容を逆転させる
            if (0 < cchSz) //    その行の中身があったら
            {
                //    前から順番に反転文字と入れ替えていく
                for (d = 0; cchSz > d;)
                {
                    d += FlipReplacementApply(FLIP_VERTICAL, &(ptString[d]), atBuff, MIN_STRING);
                    vcUpset.at(dL) += wstring(atBuff); // wsInvBuff.push_back( atBuff );
                    //    とりあえず順番にいれておいて、書き込む時にreverseすればいい
                }
            }
            FREE(ptString);
        }

        //    上から入れ替えていく
        itLine = itStart;
        for (iLns = iTop, dL = vcUpset.size() - 1; iBtm >= iLns; iLns++, dL--)
        {
            //    改めて選択範囲確保
            ptString = SeledTextAlloc(itLine, &iSelDot, &iMozi);
            itLine++;
            if (0 != iMozi)
            {
                //    元内容削除してアンドゥ記録
                DocStringErase(iSelDot, iLns, nullptr, iMozi);
                SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE, ptString, iSelDot, iLns, bFirst);
                bFirst = FALSE;
                FREE(ptString); //    アンドゥ記録してから削除セヨ
            }

            cchSz = vcUpset.at(dL).size() + 2; //    使い回し注意
            ptInvStr = (LPTSTR)malloc(cchSz * sizeof(TCHAR));
            StringCchCopy(ptInvStr, cchSz, vcUpset.at(dL).c_str());

            //    元文字列に該当する場所がなかったら、末端にしておく
            if (0 == iMozi)
            {
                iSelDot = DocLineParamGet(iLns, nullptr, nullptr);
            }

            //    ひっくり返した文字列を挿入
            iGyou = iLns;
            DocInsertString(&iSelDot, &iGyou, nullptr, ptInvStr, 0, bFirst);
            bFirst = FALSE;

            FREE(ptInvStr);
        }

    }
    catch (exception &err)
    {
        return (HRESULT)ETC_MSG(err.what(), E_UNEXPECTED);
    }
    catch (...)
    {
        return (HRESULT)ETC_MSG(("etc error"), E_UNEXPECTED);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// source가 여러 문자여도 가장 긴 항목부터 비교해 한 번에 치환한다.
UINT FlipReplacementApply(UINT dMode, LPCTSTR ptSource, LPTSTR ptOutput, UINT_PTR cchSz)
{
    UINT_PTR dParts;
    UINT_PTR cchPart = 1;
    UINT_PTR cchConsume = 1;
    FLIP_REPLACEMENT_ITR itParts, itBegin, itEnd;

    if (dMode)
    {
        dParts = gvcHorizontalFlipParts.size();
        itBegin = gvcHorizontalFlipParts.begin();
        itEnd = gvcHorizontalFlipParts.end();
    }
    else
    {
        dParts = gvcVerticalFlipParts.size();
        itBegin = gvcVerticalFlipParts.begin();
        itEnd = gvcVerticalFlipParts.end();
    }

    ZeroMemory(ptOutput, cchSz * sizeof(TCHAR));

    ptOutput[0] = ptSource[0]; //    デフォルト

    //    変換テーブルが無いならそのままコピって戻せばいい
    if (0 == dParts)
    {
        return 1;
    }

    //    全文字をチェック
    for (itParts = itBegin; itEnd != itParts; itParts++)
    {
        StringCchLength(itParts->atSrcStr, MIN_STRING, &cchPart);
        //    テーブルの文字列と比較していく
        if (0 == StrCmpN(ptSource, itParts->atSrcStr, cchPart))
        {
            StringCchCopy(ptOutput, cchSz, itParts->atDestStr);
            cchConsume = cchPart;
            break;
        }
        //    ヒットしたら、変換語文字列をコピーして終わり
    }

    return cchConsume;
}
//-------------------------------------------------------------------------------------------------
