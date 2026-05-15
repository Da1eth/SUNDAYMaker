// 문서 조작 로그와 실행 취소/다시 실행을 담당한다.
#include "DocContextInternal.h"
//-------------------------------------------------------------------------------------------------

static BOOLEAN gbGroupUndo; // 真ならグループアンドゥをする
//-------------------------------------------------------------------------------------------------

INT SqnUndoExec(LPUNDOBUFF, PINT, PINT);
INT SqnRedoExec(LPUNDOBUFF, PINT, PINT);
//-------------------------------------------------------------------------------------------------

// アンドゥを実行するのかを受ける
INT DocUndoExecute(PINT pxDot, PINT pyLine)
{
    INT iRslt = 0;

    iRslt = SqnUndoExec(&(DocCurrentPage().stUndoLog), pxDot, pyLine);

    DocModifyContent(TRUE);

    return iRslt;
}
//-------------------------------------------------------------------------------------------------

// リドゥを実行するのかを受ける
INT DocRedoExecute(PINT pxDot, PINT pyLine)
{
    INT iRslt = 0;

    iRslt = SqnRedoExec(&(DocCurrentPage().stUndoLog), pxDot, pyLine);

    DocModifyContent(TRUE);

    return iRslt;
}
//-------------------------------------------------------------------------------------------------

// アンドゥの動作設定をいただく
HRESULT SqnSetting(VOID)
{
    gbGroupUndo = 1;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 頁データの初期化・頁作成時に呼ぶ
HRESULT SqnInitialise(LPUNDOBUFF pstBuff)
{
    pstBuff->dNowSqn = 0;
    pstBuff->dTopSqn = 0;
    pstBuff->dGrpSqn = 0;

    pstBuff->vcOpeSqn.clear();

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 全ログ破壊
HRESULT SqnFreeAll(LPUNDOBUFF pstBuff)
{
    for (auto &stOpe : pstBuff->vcOpeSqn)
    {
        free(stOpe.ptText);
    }

    SqnInitialise(pstBuff);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// バッファ途中かどうか確認して、途中ならそこから後を削除
HRESULT SqnNumberCheck(LPUNDOBUFF pstBuff)
{
    OPSQ_ITR itOpe, itBuf;

    //    何も無し
    if (pstBuff->dNowSqn == pstBuff->dTopSqn)
        return S_FALSE;

    //    何も無い
    if (0 == pstBuff->dNowSqn)
    {
        SqnFreeAll(pstBuff);
        return S_FALSE;
    }

    itOpe = pstBuff->vcOpeSqn.end();
    itOpe--; //    最後のいっこ

    do
    {
        if (pstBuff->dNowSqn == itOpe->ixSequence)
        {
            break;
        }

        free(itOpe->ptText);
        itBuf = itOpe;
        itBuf--;
        pstBuff->vcOpeSqn.erase(itOpe);
        itOpe = itBuf;

    } while (itOpe != pstBuff->vcOpeSqn.begin());

    pstBuff->dTopSqn = itOpe->ixSequence;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 壱文字追加、又は削除
UINT SqnAppendLetter(LPUNDOBUFF pstBuff, UINT dCmd, TCHAR ch, INT xDot, INT yLine, UINT bAlone)
{
    UINT uRslt;
    TCHAR atBuffer[3];

    ZeroMemory(atBuffer, sizeof(atBuffer));
    atBuffer[0] = ch;

    //    壱文字固定なので、グループは常に孤独
    uRslt = SqnAppendString(pstBuff, dCmd, atBuffer, xDot, yLine, bAlone);

    DocModifyContent(TRUE);

    return uRslt;
}
//-------------------------------------------------------------------------------------------------

// 矩形の文字列追加、又は削除を記録
UINT SqnAppendSquare(LPUNDOBUFF pstBuff, UINT dCmd, LPCTSTR ptStr, LPPOINT pstPt, INT yLine, UINT bAlone)
{
    INT i;
    UINT_PTR cchMozi, cchSize;
    LPCTSTR ptCaret, ptSprt;
    OPERATELOG stOpe;

    //    アンドゥとかで最新位置がずれてたら、そこより新しいの破棄して付け足していく
    //    ここでシーケンス番号チェキ
    SqnNumberCheck(pstBuff);

    //    Group番号調整
    if (bAlone)
    {
        pstBuff->dGrpSqn += 1;
    }

    StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);

    ptCaret = ptStr;

    for (i = 0; yLine > i; i++) //    行数分回す
    {
        if (!(*ptCaret))
        {
            break;
        }

        pstBuff->dTopSqn += 1; //    シーケンスは常時インクリ

        ZeroMemory(&stOpe, sizeof(OPERATELOG));
        stOpe.dCommando = dCmd;
        stOpe.ixSequence = pstBuff->dTopSqn;
        stOpe.ixGroup = pstBuff->dGrpSqn; //

        stOpe.rdXdot = pstPt[i].x;
        stOpe.rdYline = pstPt[i].y;

        ptSprt = StrStr(ptCaret, CH_CRLFW); //    改行のところまで
        if (!(ptSprt))
        {
            ptSprt = ptStr + cchSize;
        }
        //    末端まで改行がなかったら、末端文字の位置を入れる
        //    末端がピタリ改行になるはず
        cchMozi = ptSprt - ptCaret; //    そこまでの文字数求めて

        cchMozi++; //    ﾇﾙﾀｰﾐﾈｰﾀ分
        stOpe.cchSize = cchMozi;
        stOpe.ptText = (LPTSTR)malloc(cchMozi * sizeof(TCHAR)); //    必要分確保
        StringCchCopy(stOpe.ptText, cchMozi, ptCaret);

        pstBuff->vcOpeSqn.push_back(stOpe);

        ptCaret = NextLineW(ptSprt); //    次の行の先頭に移動
    }

    pstBuff->dNowSqn = pstBuff->vcOpeSqn.size();

    DocModifyContent(TRUE);

    return pstBuff->dNowSqn;
}
//-------------------------------------------------------------------------------------------------

// 文字列追加、又は削除を記録
UINT SqnAppendString(LPUNDOBUFF pstBuff, UINT dCmd, LPCTSTR ptStr, INT xDot, INT yLine, UINT bAlone)
{
    UINT_PTR cchSize;
    OPERATELOG stOpe;

    //    アンドゥとかで最新位置がずれてたら、そこより新しいの破棄して付け足していく
    //    ここでシーケンス番号チェキ
    SqnNumberCheck(pstBuff);

    pstBuff->dTopSqn += 1;

    stOpe.dCommando = dCmd;
    stOpe.ixSequence = pstBuff->dTopSqn;

    //    Group番号調整
    if (bAlone)
    {
        pstBuff->dGrpSqn += 1;
    }
    stOpe.ixGroup = pstBuff->dGrpSqn; //    グループ番号、０に成らないように注意

    stOpe.rdXdot = xDot;
    stOpe.rdYline = yLine;

    //    入力した文字・もしくは削除した文字
    StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);
    stOpe.cchSize = cchSize;                                //    文字数にはヌルターミネータ分は含めない
    cchSize++;                                              //    ヌルターミネータ分
    stOpe.ptText = (LPTSTR)malloc(cchSize * sizeof(TCHAR)); //    必要分確保
    StringCchCopy(stOpe.ptText, cchSize, ptStr);

    pstBuff->vcOpeSqn.push_back(stOpe);

    pstBuff->dNowSqn = pstBuff->vcOpeSqn.size();

    DocModifyContent(TRUE);

    return pstBuff->dNowSqn;
}
//-------------------------------------------------------------------------------------------------

// アンドゥを実行
INT SqnUndoExec(LPUNDOBUFF pstBuff, PINT pxDot, PINT pyLine)
{
    INT xDot, yLine, iRslt = 0, dCrLf = 0, yPreLine = 0;
    UINT dCmd, dGrp, dNow, cchSize;
    UINT dPreGroup = 0;
    LPTSTR ptStr;

    if (pstBuff == nullptr)
        return 0; //    安全対策

#ifdef DO_TRY_CATCH
    try
    {
#endif

        do
        {
            dNow = pstBuff->dNowSqn;

            if (0 >= dNow)
            {
                return dCrLf;
            }

            dNow--; //    位置合わせ

            const auto &stOpe = pstBuff->vcOpeSqn.at(dNow);
            dCmd = stOpe.dCommando;
            dGrp = stOpe.ixGroup;
            xDot = stOpe.rdXdot;
            yLine = stOpe.rdYline;

            if (0 == dPreGroup) //    １回目は初期化すればおｋ
            {
                dPreGroup = dGrp;
                yPreLine = yLine;
            }
            else //    ２回目以降
            {
                //    別グループなら即離脱
                if (dPreGroup != dGrp)
                {
                    break;
                }

                //    複数行にわたっているなら、フラグ的にＯＮ
                if (yPreLine != yLine && 0 == dCrLf)
                {
                    dCrLf = 1;
                }
            }

            ptStr = stOpe.ptText;
            // OutOfRangeは、始点ドットが狂ってる場合があるようだ
            switch (dCmd)
            {
            case DO_INSERT: //    挿入なら、該当範囲を削除
                StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);
                iRslt = DocStringErase(xDot, yLine, nullptr, cchSize);
                break;

            case DO_DELETE: //    削除だったら、文字を入れる、すればいい
                iRslt = DocStringAdd(&xDot, &yLine, stOpe.ptText, stOpe.cchSize);
                break;

            default:
                return dCrLf;
            }

            *pxDot = xDot;
            *pyLine = yLine;

            if (dCrLf < iRslt)
            {
                dCrLf = iRslt;
            }

            pstBuff->dNowSqn -= 1;

            if (!(gbGroupUndo))
            {
                break;
            } //    グループアンドゥしない

        } while (pstBuff->dNowSqn);

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

    return dCrLf;
}
//-------------------------------------------------------------------------------------------------

// リドゥを実行
INT SqnRedoExec(LPUNDOBUFF pstBuff, PINT pxDot, PINT pyLine)
{
    INT xDot, yLine, iRslt = 0, dCrLf = 0, yPreLine = 0;
    UINT dCmd, dGrp, dNow, cchSize;
    UINT dPreGroup = 0;
    LPTSTR ptStr;

#ifdef DO_TRY_CATCH
    try
    {
#endif

        do
        {
            dNow = pstBuff->dNowSqn;
            if (dNow == pstBuff->vcOpeSqn.size())
            {
                return 0;
            }

            // dNow++;    //    位置合わせ
            const auto &stOpe = pstBuff->vcOpeSqn.at(dNow);
            dCmd = stOpe.dCommando;
            dGrp = stOpe.ixGroup;
            xDot = stOpe.rdXdot;
            yLine = stOpe.rdYline;

            if (0 == dPreGroup) //    １回目は初期化すればおｋ
            {
                dPreGroup = dGrp;
                yPreLine = yLine;
            }
            else //    ２回目以降
            {
                //    別グループなら即離脱
                if (dPreGroup != dGrp)
                {
                    break;
                }

                //    複数行にわたっているなら、フラグ的にＯＮ
                if (yPreLine != yLine && 0 == dCrLf)
                {
                    dCrLf = 1;
                }
            }

            ptStr = stOpe.ptText;

            switch (dCmd) //    リドゥの場合は素直に処理すればいい
            {
            case DO_INSERT:
                iRslt = DocStringAdd(&xDot, &yLine, stOpe.ptText, stOpe.cchSize);
                break;

            case DO_DELETE:
                StringCchLength(ptStr, STRSAFE_MAX_CCH, &cchSize);
                iRslt = DocStringErase(xDot, yLine, nullptr, cchSize);
                break;

            default:
                return dCrLf;
            }

            *pxDot = xDot;
            *pyLine = yLine;

            if (dCrLf < iRslt)
            {
                dCrLf = iRslt;
            }

            pstBuff->dNowSqn += 1;

            if (!(gbGroupUndo))
            {
                break;
            } //    グループアンドゥしない

        } while (pstBuff->dNowSqn != pstBuff->vcOpeSqn.size());

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

    return dCrLf;
}
//-------------------------------------------------------------------------------------------------
