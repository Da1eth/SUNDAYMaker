// 문서의 문자 삽입, 삭제, 클립보드 처리를 담당한다.
#include "DocContextInternal.h"
#include "DocViewBridgeInternal.h"
//-------------------------------------------------------------------------------------------------

extern FILES_ITR gitFileIt; //    今見てるファイルの本体

extern INT gixFocusPage; //    注目中のページ・とりあえず０・０インデックス

extern UINT gbUniPad;      //    パディングにユニコードをつかって、ドットを見せないようにする
extern UINT gbNoSjisSkipHangul; // 한글을 NOSJIS 색칠에서 제외할지 여부

extern UINT gdRightRuler; //    右線の位置
//-------------------------------------------------------------------------------------------------

HRESULT DocInputReturn(INT, INT);
INT DocSquareAddPreMod(INT, INT, INT, BOOLEAN);
INT DocLetterErase(INT, INT, INT);
//-------------------------------------------------------------------------------------------------

// 문자를 호환 저장/복사용 텍스트 조각으로 만든다.
BOOLEAN DocBuildNoSjisText(TCHAR cchMozi, LPSTR pcNoSjisText)
{
    TCHAR atMozi[2];
    CHAR acNoSjisText[10];
    BOOL bCant = FALSE;
    INT iRslt;
    /*
        CP932로 바로 표현할 수 없는 문자는 숫자 문자 참조로 바꾼다.
    */
    assert(pcNoSjisText);

    atMozi[0] = cchMozi;
    atMozi[1] = 0;
    acNoSjisText[0] = 0;
    acNoSjisText[1] = 0;
    acNoSjisText[2] = 0;

    iRslt = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, atMozi, 1, acNoSjisText, 10, "?", &bCant);

    if (bCant)
    {
        StringCchPrintfA(acNoSjisText, 10, ("&#%d;"), cchMozi);
    }

    if (IsSpMozi(cchMozi)) //    機種依存文字変換
    {
        StringCchPrintfA(acNoSjisText, 10, ("&#%d;"), cchMozi);

        bCant = TRUE; //    ユニコードのみ文字として扱う
    }

    StringCchCopyA(pcNoSjisText, 10, acNoSjisText);

    return bCant ? FALSE : TRUE;
}
//-------------------------------------------------------------------------------------------------

// 복사용 유니코드 텍스트에서, CP932 밖 문자만 숫자 문자 참조로 바꾼다.
BOOLEAN DocBuildCopyEntityText(TCHAR cchMozi, LPTSTR ptEntity, UINT_PTR cchSize)
{
    TCHAR atMozi[2];
    CHAR acDummy[10];
    BOOL bCant = FALSE;

    assert(ptEntity);

    if (0 >= cchSize)
        return FALSE;

    ptEntity[0] = 0;
    if (TEXT('\r') == cchMozi || TEXT('\n') == cchMozi)
        return FALSE;

    atMozi[0] = cchMozi;
    atMozi[1] = 0;
    ZeroMemory(acDummy, sizeof(acDummy));

    WideCharToMultiByte(932, WC_NO_BEST_FIT_CHARS, atMozi, 1, acDummy, 10, "?", &bCant);
    if (!bCant)
        return FALSE;

    StringCchPrintf(ptEntity, cchSize, TEXT("&#%u;"), (UINT)cchMozi);

    return TRUE;
}
//-------------------------------------------------------------------------------------------------

// 字のバイト数を確認
INT_PTR DocLetterByteCheck(LPLETTER pstLet)
{
    pstLet->mzByte = strlen(pstLet->acNoSjisText);

    if (pstLet->mzStyle & CT_NOSJISREF)
    {
        pstLet->mzByte += 4;
    }

    if (1 == pstLet->mzByte) //    １バイトだけど実は違うヤツを探す
    {
        //    半角カタカナ
        if (0xA1 <= (BYTE)(pstLet->acNoSjisText[0]) && (BYTE)(pstLet->acNoSjisText[0]) <= 0xDF)
        {
            pstLet->mzByte = 2;
        }

        //    HTML特殊記号
        else if ('"' == pstLet->acNoSjisText[0])
        {
            pstLet->mzByte = strlen(("&quot;"));
        }
        else if ('<' == pstLet->acNoSjisText[0])
        {
            pstLet->mzByte = strlen(("&lt;"));
        }
        else if ('>' == pstLet->acNoSjisText[0])
        {
            pstLet->mzByte = strlen(("&gt;"));
        }
        else if ('&' == pstLet->acNoSjisText[0])
        {
            pstLet->mzByte = strlen(("&amp;"));
        }
    }

    return pstLet->mzByte;
}
//-------------------------------------------------------------------------------------------------

// 文字データ作る・不要ならバイト数だけ確保できる
INT_PTR DocLetterDataCheck(LPLETTER pstLttr, TCHAR ch)
{
    INT_PTR iByte;
    LETTER stTemp; //    この函数内用・データ不要時に使うダミー君
    BOOLEAN bNeedWidth = TRUE;

    if (pstLttr == nullptr)
    {
        pstLttr = &stTemp;
        bNeedWidth = FALSE;
    } //    ダミー君

    ZeroMemory(pstLttr, sizeof(LETTER));
    pstLttr->cchMozi = ch;
    if (bNeedWidth)
    {
        pstLttr->rdWidth = ViewLetterWidthGet(ch);
    }
    pstLttr->mzStyle = CT_NORMAL;
    if (iswspace(ch))
    {
        pstLttr->mzStyle |= CT_SPACE;
    }
    if (!(DocBuildNoSjisText(ch, pstLttr->acNoSjisText)))
    {
        pstLttr->mzStyle |= CT_NOSJISREF;
    }
    //    CP932 기준 색상 표시 여부 확인
    {
        TCHAR atCheck[2] = {ch, 0};
        CHAR acDummy[10];
        BOOL bCantCp932 = FALSE;
        WideCharToMultiByte(932, WC_NO_BEST_FIT_CHARS, atCheck, 1, acDummy, 10, "?", &bCantCp932);
        if (bCantCp932)
        {
            if (!(gbNoSjisSkipHangul && IsHangulChar(ch)))
            {
                pstLttr->mzStyle |= CT_NOSJIS;
            }
        }
    }
    iByte = DocLetterByteCheck(pstLttr); //    バイト数確認

    return iByte;
}
//-------------------------------------------------------------------------------------------------

// 文字列の改行処理をする
HRESULT DocCrLfAdd(INT xDot, INT yLine, BOOLEAN bFirst)
{
    SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_INSERT, CH_CRLFW, xDot, yLine, bFirst);

    return DocInputReturn(xDot, yLine);
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)で改行する
HRESULT DocInputReturn(INT nowDot, INT rdLine)
{
    INT_PTR iLetter, iLines, iCount;
    ONELINE stLine;
    auto &stPage = (*gitFileIt).vcCont.at(gixFocusPage);

    LETR_ITR vcLtrItr, vcLtrEnd;

    LINE_ITR itLine, ltLineItr;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        iLines = DocNowFilePageLineCount();

        if (iLines <= rdLine)
            return E_OUTOFMEMORY;

        ZeroONELINE(&stLine);

        iLetter = DocLetterPosGetAdjust(&nowDot, rdLine, 0); //    今の文字位置を確認

        //    文字数確認
        itLine = stPage.ltPage.begin();
        std::advance(itLine, rdLine);

        iCount = itLine->vcLine.size();

        if (iLetter < iCount) //    もし行の途中で改行したら？
        {
            ltLineItr = stPage.ltPage.begin();
            std::advance(ltLineItr, (rdLine + 1));

            //    今の行の次の場所に行のデータを挿入
            stPage.ltPage.insert(ltLineItr, stLine);

            //    その行の、文字データの先頭をとる
            ltLineItr = stPage.ltPage.begin();
            std::advance(ltLineItr, (rdLine + 1)); //    追加した行まで移動

            //    ぶった切った場所を設定しなおして
            itLine = stPage.ltPage.begin();
            std::advance(itLine, rdLine);

            vcLtrItr = itLine->vcLine.begin();
            vcLtrItr += iLetter;             //    今の文字位置を示した
            vcLtrEnd = itLine->vcLine.end(); //    末端

            //    その部分を次の行にコピーする
            std::copy(vcLtrItr, vcLtrEnd, back_inserter(ltLineItr->vcLine));

            //    元の文字列を削除する
            itLine->vcLine.erase(vcLtrItr, vcLtrEnd);

            //    総ドット数再計算
            DocLineParamGet(rdLine, nullptr, nullptr);
            DocLineParamGet(rdLine + 1, nullptr, nullptr);
        }
        else //    末端で改行した
        {
            if ((iLines - 1) == rdLine) //    EOF的なところ
            {
                stPage.ltPage.push_back(stLine);
            }
            else
            {
                ltLineItr = stPage.ltPage.begin();
                std::advance(ltLineItr, (rdLine + 1)); //    今の行を示した

                //    次の場所に行のデータを挿入
                stPage.ltPage.insert(ltLineItr, stLine);
            }
        }

        DocBadSpaceCheck(rdLine);     //    ここで空白チェキ
        DocBadSpaceCheck(rdLine + 1); //    空白チェキ・次の行も確認

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

// 指定行のドット位置(キャレット位置)でバックスペース押した
INT DocInputBkSpace(PINT pdDot, PINT pdLine)
{
    INT_PTR iLines;
    INT iLetter, width = 0, neDot, bCrLf = 0;
    INT dLine = *pdLine; //    函数内で使う行番号・調整に注意
    TCHAR ch;

    LINE_ITR itLine;

    iLines = DocNowFilePageLineCount();

    if (iLines <= dLine)
        return 0; //    はみ出してたらアウツ！

    iLetter = DocLetterPosGetAdjust(pdDot, dLine, 0); //    今の文字位置を確認
    neDot = *pdDot;

    if (0 == iLetter && 0 == dLine)
        return 0; //    先頭かつ最初の行なら、なにもしない

    //    バックスペースとは、壱文字戻ってDELETEである

    if (0 != iLetter) //    行の先頭でないなら
    {
        iLetter--; //    キャレット一つ戻す
        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, dLine);

        width = itLine->vcLine.at(iLetter).rdWidth;
        ch = itLine->vcLine.at(iLetter).cchMozi;

        *pdDot = neDot - width; //    文字幅分ドットも戻して
        bCrLf = 0;

        SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE, ch, *pdDot, dLine, TRUE);
    }
    else //    行の先頭であるなら
    {
        dLine--;
        *pdLine = dLine; //    前の行に移動して
        neDot = DocLineParamGet(dLine, &iLetter, nullptr);
        *pdDot = neDot; //    CARET位置調整
        bCrLf = 1;

        //    ここでやって問題無いはず
        SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE, CH_CRLFW, *pdDot, dLine, TRUE);
    }

    DocLetterErase(*pdDot, dLine, iLetter);
    DocBadSpaceCheck(dLine); //    良くないスペースを調べておく

    return bCrLf;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)でデリート押した
INT DocInputDelete(INT xDot, INT yLine)
{
    INT_PTR iLines;
    INT iCount, iLetter, iCrLf;
    TCHAR ch;

    LINE_ITR itLine;

    iLines = DocNowFilePageLineCount();
    if (iLines <= yLine)
        return 0; //    はみ出してたらアウツ！

    iLetter = DocLetterPosGetAdjust(&xDot, yLine, 0); //    今の文字位置を確認

    DocLineParamGet(yLine, &iCount, nullptr); //    この行の文字数を斗留

    if (iCount <= iLetter)
    {
        if (iLines <= (yLine + 1))
            return 0; //    完全に末端なら何もしない
        ch = CC_LF;
    }
    else
    {
        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, yLine);

        ch = itLine->vcLine.at(iLetter).cchMozi;
    }

    iCrLf = DocLetterErase(xDot, yLine, iLetter);
    if (0 > iCrLf)
    {
        return -1;
    }

    DocBadSpaceCheck(yLine); //    良くないスペースを調べておく

    if (0 < iCrLf)
    {
        SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE, CH_CRLFW, xDot, yLine, TRUE);
    }
    else
    {
        SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE, ch, xDot, yLine, TRUE);
    }

    return iCrLf;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)で壱文字削除
INT DocLetterErase(INT xDot, INT yLine, INT iLetter)
{
    INT iCount, iRslt;

    LETR_ITR vcLtrItr;
    LINE_ITR itLine;

    iRslt = DocLineParamGet(yLine, &iCount, nullptr); //    この行の文字数を斗留
    if (0 > iRslt)
    {
        return -1;
    }

    //    ここからDELETEの処理
    if (iCount > iLetter) //    末端でないなら、今の文字消せばOK
    {
        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, yLine);

        vcLtrItr = itLine->vcLine.begin();
        vcLtrItr += iLetter; //    今の文字位置を示した

        DocIterateDelete(vcLtrItr, yLine);
        return 0;
    }
    else
    {
        DocLineCombine(yLine);
        return 1;
    }
}
//-------------------------------------------------------------------------------------------------

// 指定行の内容を削除する・改行はそのまま
HRESULT DocLineErase(INT yLine, PBOOLEAN pFirst)
{
    INT dLines, iMozis, i;
    INT_PTR cbSize, cchSize;
    LPTSTR ptBuffer;
    wstring wsString;
    LINE_ITR itLine;

    wsString.clear();

    dLines = DocNowFilePageLineCount(); // DocPageParamGet( nullptr, nullptr );    //    行数確認
    if (dLines <= yLine)
        return E_OUTOFMEMORY; //    はみ出し確認

    DocLineParamGet(yLine, &iMozis, nullptr); //    指定行の文字数確保

    if (0 >= iMozis)
        return E_ABORT; //    文字がないならすること無い

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, yLine);

    for (i = 0; iMozis > i; i++) //    全文字を確保
    {
        wsString += itLine->vcLine.at(i).cchMozi;
    }

    cchSize = wsString.size() + 1;    //    nullptrターミネータ分足す
    cbSize = cchSize * sizeof(TCHAR); //    ユニコードなのでバイト数は２倍である

    ptBuffer = (LPTSTR)malloc(cbSize);
    ZeroMemory(ptBuffer, cbSize);
    StringCchCopy(ptBuffer, cchSize, wsString.c_str());
    SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE, ptBuffer, 0, yLine, *pFirst);
    *pFirst = FALSE;

    //    削除処理
    itLine->vcLine.clear();

    DocLineParamGet(yLine, nullptr, nullptr); //    行内容の再計算
    DocPageParamGet(nullptr, nullptr);        //    再計算

    DocBadSpaceCheck(yLine); //    リセットに必要
    DocViewRefreshLine(yLine);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 対象文字のイテレータと行を受けて、その文字を削除する
INT DocIterateDelete(LETR_ITR itLtr, INT dBsLine)
{
    INT width = 0, bySz;
    LINE_ITR itLine;

    width = itLtr->rdWidth;
    bySz = itLtr->mzByte;

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, dBsLine);

    itLine->vcLine.erase(itLtr);

    itLine->iDotCnt -= width;
    itLine->iByteSz -= bySz;

    (*gitFileIt).vcCont.at(gixFocusPage).dByteSz -= bySz;

    //    DocBadSpaceCheck( dBsLine );    //    ついでに良くないスペースを調べておく
    //    ここで調べると重そうなので、もっと上のほうで纏めてチェキるほうがよい

    return width;
}
//-------------------------------------------------------------------------------------------------

// 対象行の、次の行を、対象行の末尾にくっつける。末端でDELETE操作
HRESULT DocLineCombine(INT dBsLine)
{
    LETR_ITR vcLtrItr, vcLtrNxItr, vcLtrNxEnd;

    LINE_ITR itLine, itLineNx, ltLineItr;

    itLineNx = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLineNx, dBsLine + 1);

    if (itLineNx == (*gitFileIt).vcCont.at(gixFocusPage).ltPage.end())
        return E_ACCESSDENIED;

    //    選択範囲ある時にアンドゥして、選択範囲が死んでる状態で切り取りするとここで落ちる
    vcLtrNxItr = itLineNx->vcLine.begin(); //    次の行の先頭
    vcLtrNxEnd = itLineNx->vcLine.end();   //    次の行の尻尾

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, dBsLine);
    std::copy(vcLtrNxItr, vcLtrNxEnd, back_inserter(itLine->vcLine));

    DocLineParamGet(dBsLine, nullptr, nullptr); //    呼び出せば中で面倒みてくれる

    ltLineItr = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(ltLineItr, dBsLine + 1); //    次の行

    (*gitFileIt).vcCont.at(gixFocusPage).ltPage.erase(ltLineItr);

    DocBadSpaceCheck(dBsLine); //    ついでに良くないスペースを調べておく

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)に壱文字追加してアンドゥ記録する
INT DocInsertLetter(PINT pxDot, INT yLine, TCHAR ch)
{
    INT width;

    SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_INSERT, ch, *pxDot, yLine, TRUE);

    width = DocInputLetter(*pxDot, yLine, ch);
    *pxDot += width; //    途中でもいける

    DocBadSpaceCheck(yLine); //    ここで空白チェキ

    return width;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)に壱文字追加する・この函数内ではアンドゥの面倒は見ない
INT DocInputLetter(INT nowDot, INT rdLine, TCHAR ch)
{
    INT_PTR iLetter, iCount, iLines;
    LETTER stLetter;
    LETR_ITR vcItr;
    LINE_ITR itLine;

    //    アンドゥリドゥは呼んだところで

#ifdef DO_TRY_CATCH
    try
    {
#endif

        if (0 == ch)
        {
            return 0;
        }

        iLines = DocNowFilePageLineCount();

        if (iLines <= rdLine)
        {
            return 0;
        }

        iLetter = DocLetterPosGetAdjust(&nowDot, rdLine, 0); //    今の文字位置を確認

        itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
        std::advance(itLine, rdLine); //    対象行までイテレートする

        //    文字数確認
        iCount = itLine->vcLine.size();

        //    データ作成
        DocLetterDataCheck(&stLetter, ch); //    指定行のドット位置(キャレット位置)に壱文字追加するとき

        if (iLetter >= iCount) //    文字数同じなら末端に追加ということ
        {
            itLine->vcLine.push_back(stLetter);
        }
        else //    そうでないなら途中に追加
        {
            vcItr = itLine->vcLine.begin();
            vcItr += iLetter;
            itLine->vcLine.insert(vcItr, stLetter);
        }

        itLine->iDotCnt += stLetter.rdWidth;
        itLine->iByteSz += stLetter.mzByte;

        (*gitFileIt).vcCont.at(gixFocusPage).dByteSz += stLetter.mzByte;

        //    DocBadSpaceCheck( rdLine );    呼んだところでまとめてやる

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

    return stLetter.rdWidth;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置で文字数分削除する・改行は弐文字占有
INT DocStringErase(INT xDot, INT yLine, LPTSTR ptDummy, INT cchSize)
{
    INT i, iCrLf, iLetter, rdCnt;

    //    今の文字位置・キャレットより末尾方向に削除するので、この位置は変わらない
    iLetter = DocLetterPosGetAdjust(&xDot, yLine, 0); //    今の文字位置を確認

    rdCnt = 0;
    for (i = 0; cchSize > i; i++) //    DEL連打ってこと
    {
        iCrLf = DocLetterErase(xDot, yLine, iLetter);
        if (0 > iCrLf)
            break; //    異常発生
        if (iCrLf)
        {
            i++;
            rdCnt++;
        }
    }

    DocBadSpaceCheck(yLine); //    良くないスペースを調べておく

    return rdCnt;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)に文字列を追加する・こっちが下位函数
INT DocStringAdd(PINT pNowDot, PINT pdLine, LPCTSTR ptStr, INT cchSize)
{
    INT i, insDot, dLn, dCrLf;

    assert(ptStr);

    dCrLf = 0;
    dLn = *pdLine;
    insDot = *pNowDot;

#ifdef DO_TRY_CATCH
    try
    {
#endif
        // 2024kai クリップボード貼り付け、ページ移動高速化（3倍以上）
        GetHdcC();
        for (i = 0; cchSize > i; i++)
        {
            if (CC_CR == ptStr[i] && CC_LF == ptStr[i + 1]) //    改行であったら
            {
                DocInputReturn(insDot, *pdLine);
                i++;         //    0x0D,0x0Aだから、壱文字飛ばすのがポイント
                (*pdLine)++; //    改行したからFocusは次の行へ
                insDot = 0;  //    そして行の先頭である
                dCrLf++;     //    改行した回数カウント
            }
            else if (CC_TAB == ptStr[i])
            {
                //    タブは挿入しない
            }
            else
            {
                insDot += DocInputLetter(insDot, *pdLine, ptStr[i]);
            }
        }
        ReleaseHdcC();

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

#ifdef DO_TRY_CATCH
    try
    {
#endif
        //    ここで空白チェキ・開始行から終了行までブンブンする
        for (i = dLn; *pdLine >= i; i++)
        {
            DocBadSpaceCheck(i);
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
    //    アンドゥリドゥはここではなく呼んだほうで面倒見るほうがいい

    *pNowDot = insDot;

    return dCrLf;
}
//-------------------------------------------------------------------------------------------------

// 指定行のドット位置(キャレット位置)に文字列を矩形で追加する
INT DocSquareAdd(PINT pNowDot, PINT pdLine, LPCTSTR ptStr, INT cchSize, LPPOINT *ppstPt)
{
    LPCTSTR ptCaret, ptSprt;
    UINT_PTR cchMozi;
    INT dCrLf, dLineTotal, iScan;
    INT dBaseDot, dBaseLine;

    LPPOINT pstBuf;

    dCrLf = 0;
    dLineTotal = 0;

    iScan = 0;
    do
    {
        dLineTotal++;

        while (cchSize > iScan && ptStr[iScan] && !(CC_CR == ptStr[iScan] && (iScan + 1) < cchSize && CC_LF == ptStr[iScan + 1]))
        {
            iScan++;
        }

        if (cchSize > (iScan + 1) && CC_CR == ptStr[iScan] && CC_LF == ptStr[iScan + 1])
        {
            iScan += 2;
        }
    } while (cchSize > iScan && ptStr[iScan]);

    pstBuf = (LPPOINT)realloc(*ppstPt, sizeof(POINT) * dLineTotal);
    if (pstBuf)
    {
        *ppstPt = pstBuf;
    }
    else
    {
        return 0;
    }

    ptCaret = ptStr;
    dBaseLine = *pdLine;

    do
    {
        dBaseDot = *pNowDot;
        DocLetterPosGetAdjust(&dBaseDot, dBaseLine, 0); //    場所合わせ

        ptSprt = StrStr(ptCaret, CH_CRLFW); //    改行のところまで
        if (!(ptSprt))
        {
            ptSprt = ptStr + cchSize;
        }
        //    末端まで改行がなかったら、末端文字の位置を入れる
        cchMozi = ptSprt - ptCaret; //    そこまでの文字数求めて

        pstBuf[dCrLf].x = dBaseDot;
        pstBuf[dCrLf].y = dBaseLine;
        DocStringAdd(&dBaseDot, &dBaseLine, ptCaret, cchMozi);

        ptCaret = NextLineW(ptSprt); //    次の行の先頭に移動
        if (*ptCaret)
        {
            dBaseLine++;
        } //    行位置も進める

        dCrLf++;

    } while (*ptCaret); //    データ有る限りループで探す

    *pdLine = dBaseLine; //    末端位置を書き戻す
    *pNowDot = dBaseDot;

    return dCrLf;
}
//-------------------------------------------------------------------------------------------------

// 現在頁の末端に改行を追加する
INT DocAdditionalLine(INT addLine, PBOOLEAN pFirst)
{
    UINT_PTR iLines;
    INT cbSize, cchMozi, i;
    INT dBaseDot, dBaseLine;
    LPTSTR ptBuffer = nullptr;

    iLines = DocNowFilePageLineCount();
    //    この頁の行数

    //    追加するのは最終行の末端
    dBaseLine = iLines - 1;

    cchMozi = CH_CRLF_CCH * addLine; //    改行の文字数＋ぬるたーみねーた
    cbSize = (cchMozi + 1) * sizeof(TCHAR);
    ptBuffer = (LPTSTR)malloc(cbSize);
    if (!ptBuffer)
        return iLines;

    ptBuffer[cchMozi] = 0;
    for (i = 0; addLine > i; i++)
    {
        ptBuffer[i * CH_CRLF_CCH] = CC_CR;
        ptBuffer[(i * CH_CRLF_CCH) + 1] = CC_LF;
        DocViewRefreshLine(dBaseLine + i);
    }

    dBaseDot = DocLineParamGet(dBaseLine, nullptr, nullptr);
    SqnAppendString(&(gitFileIt->vcCont.at(gixFocusPage).stUndoLog), DO_INSERT, ptBuffer, dBaseDot, dBaseLine, *pFirst);
    DocStringAdd(&dBaseDot, &dBaseLine, ptBuffer, cchMozi);

    FREE(ptBuffer);

    *pFirst = FALSE;

    return iLines;
}
//-------------------------------------------------------------------------------------------------

// 矩形貼付をする前に、    場の状況を確認して、必要なら整形する
INT DocSquareAddPreMod(INT xDot, INT yLine, INT dNeedLine, BOOLEAN bFirst)
{
    //    行増やすのと、所定の位置までスペースで埋める
    INT_PTR iLines;
    INT iBaseDot, iBaseLine, iMinus, i;
    UINT cchBuf;
    LPTSTR ptBuffer = nullptr;

    //    この頁の行数
    iLines = DocNowFilePageLineCount();

    //    全体行数より、追加行数が多かったら、改行増やす
    if (iLines < (dNeedLine + yLine))
    {
        iMinus = (dNeedLine + yLine) - iLines; //    追加する行数

        DocAdditionalLine(iMinus, &bFirst); //    bFirst = FALSE;

        //    この頁の行数取り直し
        iLines = DocNowFilePageLineCount();
    }

    for (i = 0; dNeedLine > i; i++)
    {
        iBaseLine = yLine + i;
        iBaseDot = DocLineParamGet(iBaseLine, nullptr, nullptr);
        //    基準から存在ドットを引くと、＋なら足りない
        iMinus = xDot - iBaseDot;

        if (gbUniPad)
        {
            if (0 >= iMinus)
                continue;
        }
        else
        {
            if (3 >= iMinus)
                continue;
        } //    余るか３以下なら気にする必要は無い

        ptBuffer = DocPaddingSpaceWithPeriod(iMinus, nullptr, nullptr, nullptr, FALSE);
        if (!ptBuffer) //    まずは綺麗に納めるのを試みて、ダメならズレありで再計算
        {
            ptBuffer = DocPaddingSpaceWithGap(iMinus, nullptr, nullptr);
        }
        if (!ptBuffer)
        {
            continue;
        }
        StringCchLength(ptBuffer, STRSAFE_MAX_CCH, &cchBuf);

        SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_INSERT, ptBuffer, iBaseDot, iBaseLine, bFirst);
        bFirst = FALSE;
        DocStringAdd(&iBaseDot, &iBaseLine, ptBuffer, cchBuf);

        FREE(ptBuffer);
    }

    return bFirst;
}
//-------------------------------------------------------------------------------------------------

// 文字列（矩形もアリ）を挿入する
INT DocInsertString(PINT pNowDot, PINT pdLine, PINT pdMozi, LPCTSTR ptText, UINT dStyle, BOOLEAN bFirst)
{
    INT dBaseDot, dBaseLine, dNeedLine;
    INT dCrLf, dLastLine;
    UINT_PTR cchSize;
    LPPOINT pstPoint;

    dBaseDot = *pNowDot;
    dBaseLine = *pdLine;
    dLastLine = *pdLine;

    if (!(ptText))
        return 0; //    挿入文字列がないなら直ぐ終わってよい

    StringCchLength(ptText, STRSAFE_MAX_CCH, &cchSize);

    if (dStyle & D_SQUARE) //    矩形用
    {
        //    使う行数
        dNeedLine = DocStringInfoCount(ptText, cchSize, nullptr, nullptr);

        bFirst = DocSquareAddPreMod(*pNowDot, *pdLine, dNeedLine, bFirst);
        //    中でアンドゥ追加までやる。

        pstPoint = nullptr; //    nullptr化必須
        dCrLf = DocSquareAdd(pNowDot, pdLine, ptText, cchSize, &pstPoint);

        //    貼付前の整形を含めて１Groupとして扱う必要がある
        SqnAppendSquare(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_INSERT, ptText, pstPoint, dCrLf, bFirst);
        bFirst = FALSE;

        FREE(pstPoint);

        dLastLine = *pdLine;
    }
    else
    {
        //    この中で改行とか面倒見る
        dCrLf = DocStringAdd(pNowDot, pdLine, ptText, cchSize);

        SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_INSERT, ptText, dBaseDot, dBaseLine, bFirst);
        bFirst = FALSE;

        dLastLine = DocPageParamGet(nullptr, nullptr); // 再計算必要か？
    }

    if (pdMozi)
    {
        *pdMozi = DocLetterPosGetAdjust(pNowDot, *pdLine, 0);
    } //    今の文字位置を確認
    DocViewRefreshAll();

    //    ヤバイ状態のときは操作しないようにする
    if (!(D_INVISI & dStyle))
        DocViewMoveCaret(*pNowDot, *pdLine);

    return dCrLf;
}
//-------------------------------------------------------------------------------------------------

// クリップボードの文字列を挿入する・いわゆる貼り付け
INT DocInputFromClipboard(PINT pNowDot, PINT pdLine, PINT pdMozi, UINT bSqMode)
{
    LPTSTR ptString = nullptr;
    UINT cchSize, dStyle = 0, i, j;
    INT dCrLf, dTop, dBtm;
    BOOLEAN bSelect;
    UINT dSqSel, iLines;

    //    クリップボードからデータを頂く
    ptString = DocClipboardDataGet(&dStyle);
    if (!(ptString))
    {
        NotifyBalloonExist(TEXT("텍스트가 아니라서 붙여넣을 수 없습니다."), TEXT("이건 오린린이에요"), NIIF_INFO);
        return 0;
    }

    StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSize);

    //    タブをヌく
    for (i = 0; cchSize > i;)
    {
        if (CC_TAB == ptString[i])
        {
            for (j = i; cchSize > j; j++)
            {
                ptString[j] = ptString[j + 1];
            }
            cchSize--;
            continue;
        }
        i++;
    }

    bSelect = IsSelecting(&dSqSel); //    選択状態であるか
    if (bSelect)
    {
        DocSelRangeGet(&dTop, &dBtm);
        dCrLf = DocSelectedDelete(pNowDot, pdLine, dSqSel, TRUE);
        if (dCrLf) //    処理した行以降全取っ替え
        {
            iLines = DocPageParamGet(nullptr, nullptr); //    再計算も要るかも・・・
            DocViewRefreshLines(*pdLine, iLines);
        }
        else
        {
            DocViewRefreshLine(*pdLine);
        }
    }

    if (bSqMode)
        dStyle |= D_SQUARE; //    矩形挿入として扱うか
    dCrLf = DocInsertString(pNowDot, pdLine, pdMozi, ptString, dStyle, TRUE);

    FREE(ptString);

    DocPageInfoRenew(-1, 1);

    return dCrLf;
}
//-------------------------------------------------------------------------------------------------

// 選択範囲のデータをクリップボードする
INT DocExClipSelect(UINT bStyle)
{
    INT cbSize;
    LPVOID pString = nullptr;

    //    SJISの場合は、ユニコード文字は&#dddd;で確保される

    cbSize = DocSelectTextGetAlloc(bStyle, &pString, nullptr);

    //    もし選択範囲なかったら、Focus行の内容をコピるとか

    DocClipboardDataSet(pString, cbSize, bStyle);

    FREE(pString);

    return cbSize;
}
//-------------------------------------------------------------------------------------------------

// クリップボードのデータをいただく
LPTSTR DocClipboardDataGet(PUINT pdStyle)
{
    LPTSTR ptString = nullptr, ptClipTxt;
    LPSTR pcStr, pcClipTp; //    変換用臨時
    DWORD cbSize;
    UINT ixSqrFmt, dEnumFmt;
    INT ixCount, iC;
    HANDLE hClipData;

    ixSqrFmt = RegisterClipboardFormat(CLIP_SQUARE);

    //    クリップボードの中身をチェキ・どっちにしてもユニコードテキストフラグはある
    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        if (pdStyle) //    矩形であるか
        {
            if (IsClipboardFormatAvailable(ixSqrFmt))
            {
                *pdStyle = D_SQUARE;
            }
        }

        OpenClipboard(nullptr); //    クリップボードをオーポンする
        //    開けっ放しだと他のアプリに迷惑なのですぐ閉めるように

        dEnumFmt = 0; //    初期値は０
        ixCount = CountClipboardFormats();
        for (iC = 0; ixCount > iC; iC++)
        { //    順番に列挙して、先にヒットしたフォーマットで扱う
            dEnumFmt = EnumClipboardFormats(dEnumFmt);
            if (CF_UNICODETEXT == dEnumFmt || CF_TEXT == dEnumFmt)
            {
                break;
            }
        }
        if (0 == dEnumFmt)
        {
            return nullptr;
        }
        //    それ以上列挙が無いか、函数失敗なら０になる

        //    クリップボードのデータをゲッツ！
        //    ハンドルのオーナーはクリップボードなので、こちらからは操作しないように
        //    中身の変更などもってのほかである
        hClipData = GetClipboardData(dEnumFmt);

        if (CF_UNICODETEXT == dEnumFmt)
        {
            //    取得データを処理。TEXTなら、ハンドルはグローバルメモリハンドル
            //    新たにコピーされたらハンドルは無効になるので、中身をコピーせよ
            ptClipTxt = (LPTSTR)GlobalLock(hClipData);
            cbSize = GlobalSize((HGLOBAL)hClipData);
            //    確保出来るのはバイトサイズ・テキストだと末尾のnullptrターミネータ含む

            if (0 < cbSize)
            {
                ptString = (LPTSTR)malloc(cbSize);
                StringCchCopy(ptString, (cbSize / 2), ptClipTxt);
            }
        }
        else //    非ユニコードが優先されている場合
        {
            pcClipTp = (LPSTR)GlobalLock(hClipData);
            cbSize = GlobalSize((HGLOBAL)hClipData);

            if (0 < cbSize)
            {
                pcStr = (LPSTR)malloc(cbSize);
                StringCchCopyA(pcStr, cbSize, pcClipTp);

                ptString = LegacyEncodedTextDecodeAlloc(pcStr); //    ANSI 텍스트를 유니코드로 바꾼다
                free(pcStr);
            }
        }

        //    使い終わったら閉じておく
        GlobalUnlock(hClipData);
        CloseClipboard();
    }

    return ptString;
}
//-------------------------------------------------------------------------------------------------

// クリップボードに文字列貼り付け
HRESULT DocClipboardDataSet(LPVOID pDatum, INT cbSize, UINT dStyle)
{
    HGLOBAL hGlobal;
    HANDLE hClip;
    LPVOID pBuffer;
    HRESULT hRslt;
    UINT ixFormat, ixSqrFmt;

    //    オリジナルフォーマット名を定義しておく
    ixFormat = RegisterClipboardFormat(CLIP_FORMAT);
    ixSqrFmt = RegisterClipboardFormat(CLIP_SQUARE);

    //    クリップするデータは共有メモリに入れる
    hGlobal = GlobalAlloc(GHND, cbSize);
    pBuffer = GlobalLock(hGlobal);
    CopyMemory(pBuffer, pDatum, cbSize);
    GlobalUnlock(hGlobal);

    //    クリップボードオーポン
    OpenClipboard(nullptr);

    //    中身を消しちゃう
    EmptyClipboard();

    //    共有メモリにブッ込んだデータをクリッペする
    if (dStyle & D_UNI)
        hClip = SetClipboardData(CF_UNICODETEXT, hGlobal);
    else
        hClip = SetClipboardData(CF_TEXT, hGlobal);

    if (hClip)
    {
        SetClipboardData(ixFormat, hGlobal);
        //    クリッポが上手くいったら、オリジナル名でも記録しておく

        if (dStyle & D_SQUARE) //    矩形選択だったら
        {
            SetClipboardData(ixSqrFmt, hGlobal);
        }

        hRslt = S_OK;
    }
    else
    {
        //    登録失敗の場合は、自分で共有メモリを破壊せないかん
        GlobalFree(hGlobal);
        hRslt = E_OUTOFMEMORY;
    }

    //    クリップボード閉じる・閉じたら即CHAINが発生する・函数内で発生させてる？
    CloseClipboard();

    return hRslt;
}
//-------------------------------------------------------------------------------------------------

// クリップボードに１文字だけ入れる
HRESULT DocClipLetter(TCHAR ch)
{
    TCHAR atBuff[3];

    ZeroMemory(atBuff, sizeof(atBuff));
    atBuff[0] = ch;

    DocClipboardDataSet(atBuff, 4, D_UNI);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 頁全体をコピーする
HRESULT DocPageAllCopy(UINT bStyle)
{
    INT cbSize;
    LPVOID pString = nullptr;

    //    SJISの場合は、ユニコード文字は&#dddd;で確保される

    cbSize = DocPageGetAlloc(bStyle, &pString);

    DocClipboardDataSet(pString, cbSize, bStyle);

    FREE(pString);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 各行の末端から８００くらいまでを、指定した文字で埋める。
HRESULT DocScreenFill(LPTSTR ptFill)
{
    UINT_PTR dLines, dRiDot, cchSize;
    BOOLEAN bSel = TRUE, bFirst;
    INT iTop, iBottom, i, iUnt, j, remain;
    INT nDot, sDot, mDot;
    LPTSTR ptBuffer;
    wstring wsBuffer;

    //    現在行数と、右ドット数・ルーラ位置を使う
    dLines = DocNowFilePageLineCount();
    dRiDot = gdRightRuler;

    //    選択範囲あるならそっち優先。ないなら画面全体
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
    if (0 > iTop || 0 > iBottom)
    {
        iTop = 0;
        iBottom = dLines - 1;
        bSel = FALSE;
    }

    DocViewClearSelection();

    //    埋め文字列の幅
    mDot = ViewStringWidthGet(ptFill);

    bFirst = TRUE;
    //    各行毎に追加する感じで
    for (i = iTop; iBottom >= i; i++)
    {
        nDot = DocLineParamGet(i, nullptr, nullptr); //    呼び出せば中で面倒みてくれる
        sDot = dRiDot - nDot;                        //    残りドット
        if (0 >= sDot)
        {
            continue;
        } //    右端超えてるならなにもせんでいい

        iUnt = (sDot / mDot) + 1; //    埋める分・はみ出し・適当で良い

        //    入れる文字列作成
        wsBuffer.clear();
        for (j = 0; iUnt > j; j++)
        {
            wsBuffer += wstring(ptFill);
        }

        cchSize = wsBuffer.size() + 1;
        ptBuffer = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
        StringCchCopy(ptBuffer, cchSize, wsBuffer.c_str());
        //    末端にブチこむ
        DocInsertString(&nDot, &i, nullptr, ptBuffer, 0, bFirst);
        bFirst = FALSE;
        FREE(ptBuffer);

        DocBadSpaceCheck(i); //    ここで空白チェキ・あまり意味はないが色換えは必要
    }

    if (!(bSel)) //    非選択状態で
    {
        remain = 40 - dLines; //    とりあえず４０行とする
        if (0 < remain)       //    足りないなら
        {
            DocAdditionalLine(remain, &bFirst); //    とりあえず改行して
            dLines = DocNowFilePageLineCount();
            iUnt = (dRiDot / mDot) + 1; //    埋める分・はみ出し・適当で良い

            //    入れる文字列作成
            wsBuffer.clear();
            for (j = 0; iUnt > j; j++)
            {
                wsBuffer += wstring(ptFill);
            }
            cchSize = wsBuffer.size() + 1;
            ptBuffer = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
            StringCchCopy(ptBuffer, cchSize, wsBuffer.c_str());

            iTop = iBottom + 1;
            iBottom = dLines - 1;

            for (i = iTop; iBottom >= i; i++)
            {
                //    末端にブチこむ
                nDot = DocLineParamGet(i, nullptr, nullptr); //    多分０のハズ
                DocInsertString(&nDot, &i, nullptr, ptBuffer, 0, bFirst);
                bFirst = FALSE;

                DocBadSpaceCheck(i); //    ここで空白チェキ・あまり意味はないが色換えは必要
            }

            FREE(ptBuffer);
        }
    }

    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
