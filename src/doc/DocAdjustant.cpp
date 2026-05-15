// 문서 정렬과 패딩 계산을 담당한다.
#include "DocContextInternal.h"
#include "DocViewBridgeInternal.h"
#include "UiText.h"
//-------------------------------------------------------------------------------------------------

extern UINT
    gbUniPad; // パディングにユニコードをつかって、ドットを見せないようにする

static INT gdDiffLock; // ずれ調整の基準ドット値
//-------------------------------------------------------------------------------------------------

namespace
{
    struct PADDING_UNITS
    {
        INT dUnicode;
        INT dHalfWidth;
    };

    constexpr PADDING_UNITS gaUnicodePaddingUnits[11] = {
        {0, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {0, 1}, {1, 1}, {1, 1}, {1, 0}, {1, 1}, {1, 0}};

    LPTSTR AllocateTextBuffer(INT cchSize)
    {
        auto *ptStr = static_cast<LPTSTR>(malloc((cchSize + 1) * sizeof(TCHAR)));
        if (ptStr == nullptr)
        {
            return nullptr;
        }

        ZeroMemory(ptStr, (cchSize + 1) * sizeof(TCHAR));
        return ptStr;
    }
} // namespace

//    右揃え用空白パヤーン
CONST TCHAR gaatDotPtrnPeriod[11][9] = {
    {TEXT("　　")},     //    22    0
    {TEXT("　....")},   //    23    1
    {TEXT(" 　 .")},    //    24    2
    {TEXT("　　.")},    //    25    3
    {TEXT("　.....")},  //    26    4
    {TEXT("　　 ")},    //    27    5
    {TEXT("　　..")},   //    28    6
    {TEXT("　......")}, //    29    7
    {TEXT("　　 .")},   //    30    8
    {TEXT("　　...")},  //    31    9
    {TEXT("　 　 ")}    //    32    10
};
#define RIGHT_WALL TEXT('|') //    右揃え用壁文字

//    右揃え用空白パヤーンユニコード    20120315
CONST TCHAR gaatDotPtrnUnic[11][6] = {
    {TEXT("　　")},                   //    22    0    全SP　全SP
    {0x3000, 0x3000, 0x200A},         //    23    1    全SP　全SP　1dot
    {0x3000, 0x3000, 0x2009},         //    24    2    全SP　全SP　2dot
    {0x3000, 0x3000, 0x2006},         //    25    3    全SP　全SP　3dot
    {0x3000, 0x3000, 0x2005},         //    26    4    全SP　全SP　4dot
    {TEXT("　　 ")},                  //    27    5    全SP　全SP　半SP
    {0x3000, 0x0020, 0x3000, 0x200A}, //    28    6    全SP　半SP　全SP　1dot
    {0x3000, 0x0020, 0x3000, 0x2009}, //    29    7    全SP　半SP　全SP　2dot
    {0x3000, 0x0020, 0x3000, 0x2006}, //    30    8    全SP　半SP　全SP　3dot
    {0x3000, 0x0020, 0x3000, 0x2005}, //    31    9    全SP　半SP　全SP　4dot
    {TEXT("　 　 ")}                  //    32    10    全SP　半SP　全SP　半SP
};

//    パディング用空白パヤーン・非ユニコード
CONST TCHAR gaatPaddingSpDot[34][9] = {
    {TEXT("")},        //    0
    {TEXT(".")},       //    1    3
    {TEXT(".")},       //    2    3
    {TEXT(".")},       //    3    3
    {TEXT(".")},       //    4    3
    {TEXT(" ")},       //    5        TEXT("..")    6
    {TEXT("..")},      //    6
    {TEXT("..")},      //    7    6
    {TEXT(". ")},      //    8
    {TEXT("...")},     //    9
    {TEXT("　")},      //    10    11
    {TEXT("　")},      //    11
    {TEXT("　")},      //    12    11    { TEXT("....") }
    {TEXT(".　")},     //    13    14
    {TEXT(".　")},     //    14
    {TEXT("　 ")},     //    15    16    { TEXT(".....") }
    {TEXT("　 ")},     //    16
    {TEXT(".　.")},    //    17
    {TEXT(". 　")},    //    18    19    { TEXT("......") }
    {TEXT(". 　")},    //    19
    {TEXT("..　.")},   //    20
    {TEXT(" 　 ")},    //    21    { TEXT(".......") }
    {TEXT("　　")},    //    22
    {TEXT("　　")},    //    23    22    { TEXT("..　..") }
    {TEXT("　 . ")},   //    24
    {TEXT(".　　")},   //    25
    {TEXT("　 　")},   //    26    27    { TEXT("...　..") }
    {TEXT("　 　")},   //    27
    {TEXT(".　　.")},  //    28
    {TEXT(".　 　")},  //    29    30    { TEXT("...　...") }
    {TEXT(".　 　")},  //    30
    {TEXT(".　.　.")}, //    31
    {TEXT("　 　 ")},  //    32
    {TEXT("　　　")}   //    33
};

//    パディング用空白パヤーン・ユニコード
CONST TCHAR gaatPaddingSpDotW[11][3] = {
    {TEXT("")},        //    0    0    0
    {8202},            //    1    1    0
    {8201},            //    2    1    0
    {8198},            //    3    1    0
    {8197},            //    4    1    0
    {TEXT(' ')},       //    5    0    1
    {8202, TEXT(' ')}, //    6    1    1
    {8201, TEXT(' ')}, //    7    1    1
    {8194},            //    8    1    0
    {8197, TEXT(' ')}, //    9    1    1
    {8199}             //    10    1    0
}; //        U    H

//-------------------------------------------------------------------------------------------------

UINT SpaceWidthAdjust(INT, PINT, PINT);
LPTSTR SpaceStrAlloc(INT, INT);

UINT DocSpaceDifference(UINT, PINT, INT, UINT);

HRESULT DocRightGuideSet(INT, INT);

LPTSTR DocPaddingSpace(INT, PINT, PINT);
//-------------------------------------------------------------------------------------------------

// 使うスペースの数を、減算して調整
UINT SpaceWidthAdjust(INT dDot, PINT pZen, PINT pHan)
{
    INT dZen, dHan, size;

    dZen = *pZen;
    dHan = *pHan;

    do
    {
        size = (dZen * SPACE_ZEN) + (dHan * SPACE_HAN);

        if (dDot == size)
        {
            *pZen = dZen;
            *pHan = dHan;
            return 1;
        }

        dZen--;    //    全SP:11dot、半SP:5dotなので、
        dHan += 2; //    全SP減らして半SP弐増やすと1dot縮む
    } while (0 <= dZen); //    全SPが無くなるとアウト

    return 0;
}
//-------------------------------------------------------------------------------------------------

// 全角スペース、半角スペースの個数を受けて、文字列をつくって返す
LPTSTR SpaceStrAlloc(INT dZen, INT dHan)
{
    INT cchSize, i;
    LPTSTR ptStr;

    cchSize = dZen + dHan; //    必要数    ↓nullptrたみねた分増やす
    ptStr = AllocateTextBuffer(cchSize);
    if (ptStr == nullptr)
        return nullptr;

    for (i = (cchSize - 1); 0 <= i;)
    {
        if (0 < dHan)
        {
            ptStr[i--] = TEXT(' ');
            dHan--;
            if (0 > i)
                break;
        }

        if (0 < dZen)
        {
            ptStr[i--] = TEXT('　');
            dZen--;
            if (0 > i)
                break;
        }
    }

    return ptStr;
}
//-------------------------------------------------------------------------------------------------

// ドット数を受けて、そこに収まるようなスペースの組み合わせを計算・ユニコード空白も使う
LPTSTR DocPaddingSpaceUni(INT dTgtDot, PINT pdZenSp, PINT pdHanSp,
                          PINT pdUniSp)
{
    INT dZen, dHan, dUni;
    INT iCnt, iRem;
    INT cchSize, i;
    LPTSTR ptStr = nullptr;

    //    幅０だと作れない
    if (0 >= dTgtDot)
        return nullptr;

    iCnt = dTgtDot / SPACE_ZEN; //    全角で出来るだけ埋める
    iRem = dTgtDot % SPACE_ZEN; //    余るか？

    dZen = iCnt; //    とりあえず必要数

    dUni = gaUnicodePaddingUnits[iRem].dUnicode;
    dHan = gaUnicodePaddingUnits[iRem].dHalfWidth;

    cchSize = dZen + dHan + dUni; //    必要数    ↓nullptrたみねた分増やす
    ptStr = AllocateTextBuffer(cchSize);
    if (ptStr == nullptr)
        return nullptr;

    for (i = 0; dZen > i; i++)
    {
        ptStr[i] = TEXT('　');
    }

    switch (iRem) //    各文字を追加
    {
    case 1:
        ptStr[i++] = gaatPaddingSpDotW[1][0];
        break;
    case 2:
        ptStr[i++] = gaatPaddingSpDotW[2][0];
        break;
    case 3:
        ptStr[i++] = gaatPaddingSpDotW[3][0];
        break;
    case 4:
        ptStr[i++] = gaatPaddingSpDotW[4][0];
        break;
    case 5:
        ptStr[i++] = gaatPaddingSpDotW[5][0];
        break;
    case 6:
        ptStr[i++] = gaatPaddingSpDotW[6][0];
        ptStr[i++] = gaatPaddingSpDotW[6][1];
        break;
    case 7:
        ptStr[i++] = gaatPaddingSpDotW[7][0];
        ptStr[i++] = gaatPaddingSpDotW[7][1];
        break;
    case 8:
        ptStr[i++] = gaatPaddingSpDotW[8][0];
        break;
    case 9:
        ptStr[i++] = gaatPaddingSpDotW[9][0];
        ptStr[i++] = gaatPaddingSpDotW[9][1];
        break;
    case 10:
        ptStr[i++] = gaatPaddingSpDotW[10][0];
        break;
    default:
        break;
    }

    if (pdZenSp)
        *pdZenSp = dZen;
    if (pdHanSp)
        *pdHanSp = dHan;
    if (pdUniSp)
        *pdUniSp = dUni;

    return ptStr;
}
//-------------------------------------------------------------------------------------------------

// ドット数を受けて、そこに収まるようなスペースの組み合わせを計算
LPTSTR DocPaddingSpace(INT dTgtDot, PINT pdZenSp, PINT pdHanSp)
{
    INT dZen, dHan;
    INT iCnt, iRem;
    UINT dRslt;
    LPTSTR ptStr = nullptr;

    //    幅０だと作れない
    if (0 >= dTgtDot)
        return nullptr;

    iCnt = dTgtDot / SPACE_ZEN; //    全角で出来るだけ埋める
    iRem = dTgtDot % SPACE_ZEN; //    余るか？

    dZen = iCnt; //    とりあえず必要数

    if (1 <= iRem && iRem <= 5) //    半角で埋める
    {
        dHan = 1;
    }
    else if (6 <= iRem && iRem <= 10) //    全角で埋める
    {
        dHan = 0;
        dZen++;
    }
    else //    ぴったりだった
    {
        dHan = 0;
    }

    //    数調整
    dRslt = SpaceWidthAdjust(dTgtDot, &dZen, &dHan);

    if (dRslt)
    {
        if (pdZenSp)
            *pdZenSp = dZen;
        if (pdHanSp)
            *pdHanSp = dHan;

        //    メモリ確保して文字列作る
        ptStr = SpaceStrAlloc(dZen, dHan);

        return ptStr;
    }

    return nullptr;
}
//-------------------------------------------------------------------------------------------------

// ドット数を多少前後してもいいから埋める
LPTSTR DocPaddingSpaceWithGap(INT dTgtDot, PINT pdZenSp, PINT pdHanSp)
{
    INT cchSize, i;
    LPTSTR ptStr = nullptr;

    if (16 <= dTgtDot) //    幅を増やしながら収まる範囲をさがす
    {
        i = 0;

        do
        {
            if (22 < i)
                return nullptr; //    無限ループ阻止。大丈夫と思うけど。

            ptStr = DocPaddingSpace(dTgtDot, pdZenSp, pdHanSp);
            dTgtDot++;
            i++;

        } while (!(ptStr));

        return ptStr;
    }

    //    エリアが小さすぎるので例外
    cchSize = 1; //    必要数    ↓nullptrたみねた分増やす
    ptStr = AllocateTextBuffer(cchSize);
    if (ptStr == nullptr)
        return nullptr;

    if (7 >= dTgtDot) //    半角壱個で目をつぶる
    {
        ptStr[0] = TEXT(' ');
        if (pdZenSp)
            *pdZenSp = 0;
        if (pdHanSp)
            *pdHanSp = 1;
    }
    else if (8 <= dTgtDot && dTgtDot <= 15) //    全角壱個でごまかす
    {
        ptStr[0] = TEXT('　');
        if (pdZenSp)
            *pdZenSp = 1;
        if (pdHanSp)
            *pdHanSp = 0;
    }
    ptStr[1] = 0x0000;

    return ptStr;
}
//-------------------------------------------------------------------------------------------------

// 指定ドット位置が含まれている、スペースか非スペースの文字の始点終点ドット数確保
INT DocLineStateCheckWithDot(INT dDot, INT rdLine, PINT pLeft, PINT pRight,
                             PINT pStCnt, PINT pCount, PBOOLEAN pIsSp)
{
    UINT_PTR iCount;
    INT bgnDot, endDot;
    INT iBgnCnt, iRngCnt;
    TCHAR ch, chb;
    UINT dMozis;
    INT bSpace;
    LETR_ITR itMozi, itHead, itTail, itTemp;

    LINE_ITR itLine;

    itLine = DocCurrentLine(rdLine);

    if (!(pLeft) || !(pRight) || !(pIsSp))
    {
        return 0;
    }

    itMozi = itLine->vcLine.begin();
    iCount = itLine->vcLine.size(); //    この行の文字数確認

    //    中身が無いならエラー
    if (0 >= iCount)
    {
        *pIsSp = FALSE;
        *pLeft = 0;
        *pRight = 0;
        return 0;
    }

    dMozis = DocLetterPosGetAdjust(&dDot, rdLine, 0); //    今の文字位置を確認

    if (1 <= dMozis)
    {
        itMozi += (dMozis - 1);
    } //    キャレットの位置の左文字で判定
    //    最初から先頭ならなにもしなくておｋ
    ch = itMozi->cchMozi;
    bSpace = iswspace(ch);

    //    その場所から頭方向に辿って、途切れ目を探す
    itHead = itLine->vcLine.begin();

    for (; itHead != itMozi; itMozi--)
    {
        chb = itMozi->cchMozi;
        if (iswspace(chb) != bSpace)
        {
            itMozi++;
            break;
        }
    }
    //    先頭までイッちゃった場合・これが抜けてた
    if (itHead == itMozi)
    {
        chb = itMozi->cchMozi;
        if (iswspace(chb) != bSpace)
        {
            itMozi++;
        }
    }
    //    基準と異なる文字にヒットしたか、先頭位置である

    //    先頭から、ヒット位置まで辿ってドット数と文字数確認
    bgnDot = 0;
    iBgnCnt = 0;
    for (itTemp = itHead; itTemp != itMozi; itTemp++)
    {
        bgnDot += itTemp->rdWidth; //    文字幅増やして
        iBgnCnt++;                 //    文字数も増やす
    } // もし最初から先頭なら両方０のまま

    itTail = itLine->vcLine.end();

    //    その場所から、同じグループの所まで確認
    endDot = bgnDot;
    iRngCnt = 0;
    for (; itTemp != itTail; itTemp++)
    {
        chb = itTemp->cchMozi; //    同じタイプである間加算続ける
        if (iswspace(chb) != bSpace)
        {
            break;
        }

        endDot += itTemp->rdWidth;
        iRngCnt++;
    }

    *pLeft = bgnDot;
    *pRight = endDot;
    *pIsSp = bSpace ? TRUE : FALSE;

    if (pCount)
        *pCount = iRngCnt;
    if (pStCnt)
        *pStCnt = iBgnCnt;

    return (endDot - bgnDot);
}
//-------------------------------------------------------------------------------------------------

// 現在のドット位置を含んでいる空白エリアを１ドットずつずらす
UINT DocSpaceDifference(UINT vk, PINT pXdot, INT dLine, UINT dFirst)
{
    INT dTgtDot, dNowDot;
    INT dBgnDot, dEndDot;
    INT dBgnCnt, dRngCnt;
    UINT_PTR cchSize;
    BOOLEAN bIsSpace;
    LPTSTR ptSpace; //, ptOldSp;
    INT dZenSp, dHanSp, dUniSp;
    INT iDots, iBytes;

    wstring wsBuffer;
    LETR_ITR vcLtrBgn, vcLtrEnd, vcItr;

    LINE_ITR itLine;

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, dLine);

    dNowDot = *pXdot;

    if (0 == dNowDot) //    ０の場合は強引に移動
    {
        dNowDot = itLine->vcLine.at(0).rdWidth;
    }

    dTgtDot = DocLineStateCheckWithDot(dNowDot, dLine, &dBgnDot, &dEndDot,
                                       &dBgnCnt, &dRngCnt, &bIsSpace);
    if (!(bIsSpace))
        return 0; //    非スペースエリアは意味が無い

    if (VK_RIGHT == vk)
        dTgtDot++; //    右なら増やすってこと
    else if (VK_LEFT == vk)
        dTgtDot--;
    else
        return 0; //    関係ないのはアウツ

    //    当てはめるアレを計算する
    ptSpace = DocPaddingSpace(dTgtDot, &dZenSp, &dHanSp);
    if (gbUniPad)
    {
        //    作成不可だったり半角多すぎたら、ユニコード使って作り直し
        if (!(ptSpace) || (dZenSp < dHanSp)) //    (dZenSp + 1)
        {
            FREE(ptSpace);
            ptSpace = DocPaddingSpaceUni(dTgtDot, &dZenSp, &dHanSp, &dUniSp);
        }
    }

    if (!(ptSpace))
        return 0; //    作成不可だった場合

    StringCchLength(ptSpace, STRSAFE_MAX_CCH, &cchSize);

    vcLtrBgn = itLine->vcLine.begin();
    vcLtrBgn += dBgnCnt; //    該当位置まで移動して
    vcLtrEnd = vcLtrBgn;
    vcLtrEnd += dRngCnt; //    そのエリアの終端も確認

    iDots = 0;
    iBytes = 0;
    wsBuffer.clear();
    for (vcItr = vcLtrBgn; vcLtrEnd != vcItr; vcItr++)
    {
        wsBuffer += vcItr->cchMozi;
        iDots += vcItr->rdWidth;
        iBytes += vcItr->mzByte;
    }

    //    該当部分を一旦削除・アンドゥリドゥするなら内容を記録する必要がある
    itLine->vcLine.erase(vcLtrBgn, vcLtrEnd);
    itLine->iByteSz -= iBytes;
    if (0 > itLine->iByteSz)
    {
        itLine->iByteSz = 0;
    }
    itLine->iDotCnt -= iDots;
    if (0 > itLine->iDotCnt)
    {
        itLine->iDotCnt = 0;
    }

    //    Space文字列を追加
    dNowDot = dBgnDot;
    DocStringAdd(&dNowDot, &dLine, ptSpace, cchSize);

    *pXdot = dNowDot;

    SqnAppendString(&(DocCurrentPage().stUndoLog), DO_DELETE,
                    wsBuffer.c_str(), dBgnDot, dLine, dFirst);
    SqnAppendString(&(DocCurrentPage().stUndoLog), DO_INSERT,
                    ptSpace, dBgnDot, dLine,
                    FALSE); //    弐回目なので確定でよろし

    FREE(ptSpace);

    return dTgtDot;
}
//-------------------------------------------------------------------------------------------------

// 現在のドット位置を含んでいる空白エリアを１ドットずらすシーケンス
INT DocSpaceShiftProc(UINT vk, PINT pXdot, INT dLine)
{
    INT dDot, dMozi, dPreByte;

    //    20110720    ０文字で操作するとあぼーんするので確認しておく
    dDot = DocLineParamGet(dLine, &dMozi, &dPreByte);
    if (0 >= dMozi)
        return 0;

    dDot = DocSpaceDifference(vk, pXdot, dLine, TRUE);

    DocLetterPosGetAdjust(
        pXdot, dLine, 0); //    この中のDocLineParamGetでバイト数が計算されてる

    DocViewRefreshLine(dLine);

    DocViewMoveCaret(*pXdot, dLine);

    return dDot;
}
//-------------------------------------------------------------------------------------------------

// 右揃え線の面倒見る
HRESULT DocRightGuideline(LPVOID pVoid)
{
    INT iTop, iBottom, i;

    iTop = DocCurrentPage().dSelLineTop;
    iBottom = DocCurrentPage().dSelLineBottom;

    DocRightGuideSet(iTop, iBottom);

    DocViewClearSelection();

    if (0 > iTop || 0 > iBottom)
    {
        DocViewRefreshAll();
    }
    else
    {
        for (i = iTop; iBottom >= i; i++)
        {
            DocViewRefreshLine(i);
        }
    }

    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 指定範囲に右揃え線を付ける
HRESULT DocRightGuideSet(INT dTop, INT dBottom)
{
    //    処理が終わったら、呼んだ方で選択範囲の解除と画面更新すること

    UINT_PTR iLines, cchSize;
    INT baseDot, i, j, iMz, nDot, sDot, lDot, iUnt, iPadot;
    TCHAR ch, atBuffer[MAX_PATH];
    LPTSTR ptBuffer;
    BOOLEAN bFirst;
    wstring wsBuffer;

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    if (0 > dTop)
        dTop = 0;
    if (0 > dBottom)
        dBottom = iLines - 1;

    //
    ZeroMemory(atBuffer, sizeof(atBuffer));
    atBuffer[0] = RIGHT_WALL;

    //    一番長いとところを確認
    baseDot = DocPageMaxDotGet(dTop, dBottom);

    bFirst = TRUE;
    //    各行毎に追加する感じで
    for (i = dTop; dBottom >= i; i++)
    {
        nDot = DocLineParamGet(i, nullptr,
                               nullptr); //    呼び出せば中で面倒みてくれる
        sDot = baseDot - nDot;
        iUnt = sDot / SPACE_ZEN; //    埋める分
        sDot = sDot % SPACE_ZEN; //    はみ出しドット確認
        //    変数使い回し注意

        iPadot = nDot;
        wsBuffer.clear(); //    アンドゥバッファ用記録

        for (j = 0; iUnt > j; j++)
        {
            ch = TEXT('　'); //    入れるのは全角空白確定
            wsBuffer += ch;
            lDot = DocInputLetter(nDot, i, ch);
            nDot += lDot;
        }

        //    20120315    ユニコードモードならゆにゆにっとする
        if (gbUniPad)
        {
            iMz = lstrlen(gaatDotPtrnUnic[sDot]);
        }
        else
        {
            iMz = lstrlen(gaatDotPtrnPeriod[sDot]);
        }

        //    揃え線までの空白を埋める
        for (j = 0; iMz > j; j++)
        {
            if (gbUniPad)
            {
                ch = gaatDotPtrnUnic[sDot][j];
            } //    20120315
            else
            {
                ch = gaatDotPtrnPeriod[sDot][j];
            }

            wsBuffer += ch;
            lDot = DocInputLetter(nDot, i, ch);
            nDot += lDot;
        }

        //    揃え末端文字入れ込む
        wsBuffer += atBuffer[0];
        lDot = DocInputLetter(nDot, i, atBuffer[0]);
        nDot += lDot;

        DocBadSpaceCheck(i); //    ここで空白チェキ

        //    入れた文字を統合してアンドゥバッファリング
        cchSize = wsBuffer.size() + 1;
        ptBuffer = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
        StringCchCopy(ptBuffer, cchSize, wsBuffer.c_str());

        SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog),
                        DO_INSERT, ptBuffer, iPadot, i, bFirst);
        bFirst = FALSE;

        FREE(ptBuffer);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ずれ調整用の基準ドット値をセット
INT DocDiffAdjBaseSet(INT yLine)
{
    INT dDot = 0;
    TCHAR atMessage[MAX_STRING];

    dDot = DocLineParamGet(yLine, nullptr, nullptr);

    gdDiffLock = dDot;

    StringCchPrintf(atMessage, MAX_STRING,
                    TEXT("조정 기준 위치를 %d 도트로 설정했습니다."), dDot);
    NotifyBalloonExist(atMessage, TEXT("잠금"), NIIF_INFO);

    return dDot;
}
//-------------------------------------------------------------------------------------------------

// ずれ調整を実行する
INT DocDiffAdjExec(PINT pxDot, INT yLine)
{
    INT dMotoDot = 0;
    INT dBgnDot, dEndDot, dBgnCnt, dRngCnt, iSabun, dTgtDot, nDot;
    UINT_PTR cchSize, cchPlus;
    BOOLEAN bIsSpace;
    LPTSTR ptPlus, ptBuffer;

    wstring wsDelBuf, wsAddBuf;
    LETR_ITR vcLtrBgn, vcLtrEnd, vcItr;

    LINE_ITR itLine;

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, yLine);

    //    調整値の状況を確認
    dTgtDot = DocLineStateCheckWithDot(*pxDot, yLine, &dBgnDot, &dEndDot,
                                       &dBgnCnt, &dRngCnt, &bIsSpace);
    if (!(bIsSpace))
    {
        NotifyBalloonExist(TEXT("공백 문자가 존재하는 위치로 커서를 이동해주세요."),
                           TEXT("조정 불가"), NIIF_ERROR);
        return 0;
    }
    // 今現在の空白幅を確認

    //    対象行の長さを確認
    dMotoDot = DocLineParamGet(yLine, nullptr, nullptr);
    iSabun = gdDiffLock - dMotoDot; //    差分確認・マイナスならはみ出してる

    // まず全角半角で埋めて、半角が多いようならピリオド付けて再計算

    dTgtDot += iSabun; //    変更後のドット数

    if (41 > dTgtDot) //    ユニコード使うなら確認しなくても大丈夫？
    {
        NotifyBalloonExist(
            TEXT("공백 문자의 길이가 너무 작아서 조정할 수 없습니다."),
            TEXT("너무 좁아요"), NIIF_ERROR);
        return 0;
    }

    // 埋め文字列作成
    ptPlus = DocPaddingSpaceWithPeriod(dTgtDot, nullptr, nullptr, nullptr, FALSE);

    if (!(ptPlus))
    {
        NotifyBalloonExist(TEXT("길이를 조정하지 못했습니다."),
                           TEXT("자동 조정 실패"), NIIF_ERROR);
        return 0;
    }

    StringCchLength(ptPlus, STRSAFE_MAX_CCH, &cchPlus);

    vcLtrBgn = itLine->vcLine.begin();
    vcLtrBgn += dBgnCnt; //    該当位置まで移動して
    vcLtrEnd = vcLtrBgn;
    vcLtrEnd += dRngCnt; //    そのエリアの終端も確認

    wsDelBuf.clear();
    for (vcItr = vcLtrBgn; vcLtrEnd != vcItr; vcItr++)
    {
        wsDelBuf += vcItr->cchMozi;
    }

    //    該当部分を削除
    itLine->vcLine.erase(vcLtrBgn, vcLtrEnd);
    nDot = dBgnDot;

    cchSize = wsDelBuf.size() + 1;
    ptBuffer = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
    StringCchCopy(ptBuffer, cchSize, wsDelBuf.c_str());
    SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE,
                    ptBuffer, dBgnDot, yLine, TRUE);
    FREE(ptBuffer);

    // ここで文字列追加
    DocStringAdd(&nDot, &yLine, ptPlus, cchPlus);
    SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_INSERT,
                    ptPlus, dBgnDot, yLine, FALSE);
    FREE(ptPlus);

    // もろもろの位置合わせしておｋ
    *pxDot = nDot;

    DocLetterPosGetAdjust(pxDot, yLine, 0);

    DocViewRefreshLine(yLine);

    DocViewMoveCaret(*pxDot, yLine);

    return iSabun;
}
//-------------------------------------------------------------------------------------------------

// 指定されたドット幅を、ピリオドも使って綺麗に確保する・１９幅までなら調整できる・これはこれで必要
LPTSTR DocPaddingSpaceWithPeriod(INT dTgtDot, PINT pdZen, PINT pdHan,
                                 PINT pdPrd, BOOLEAN bFull)
{
    INT dZenSp, dHanSp, dPrdSp, m, dPre;
    LPTSTR ptSpace = nullptr, ptPlus = nullptr;
    UINT cchSize, cchPlus;

    dPre = dTgtDot;
    dPrdSp = 0;

    do
    {
        dZenSp = 0;
        dHanSp = 0;
        ptSpace = DocPaddingSpace(dTgtDot, &dZenSp, &dHanSp);

        //    作成不可だった場合    半角多すぎても不可
        if (!(ptSpace) || (dZenSp < dHanSp)) //    (dZenSp + 1)
        {
            FREE(ptSpace);
            if (gbUniPad) //    上手くいかないようなら、ユニコード使ってやり直す
            {
                ptSpace = DocPaddingSpaceUni(dTgtDot, &dZenSp, &dHanSp, nullptr);
                break;
            }
            else
            {
                dPrdSp++;
                dTgtDot -= 3; //    ピリオドは３ドット
            }
        }
        else //    問題無い文字列ならおｋ
        {
            break;
        }

    } while (dTgtDot >= 19); //    これ以上は不可？

    if (!(ptSpace) && bFull) //    まだ制作できてなく、固定テーブル使うなら
    {
        dPrdSp = 0;
        dTgtDot = dPre;

        StringCchLength(gaatPaddingSpDot[dTgtDot], STRSAFE_MAX_CCH, &cchSize);

        cchSize += 1;
        ptSpace = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
        ZeroMemory(ptSpace, cchSize * sizeof(TCHAR));

        StringCchCopy(ptSpace, cchSize, gaatPaddingSpDot[dTgtDot]);
    }

    if (ptSpace)
    {
        StringCchLength(ptSpace, STRSAFE_MAX_CCH, &cchSize);

        //    ピリオド入れてサイズ調整
        cchPlus = cchSize + dPrdSp + 1;
        ptPlus = (LPTSTR)malloc(cchPlus * sizeof(TCHAR));
        ZeroMemory(ptPlus, cchPlus * sizeof(TCHAR));

        StringCchCopy(ptPlus, cchPlus, ptSpace);
        FREE(ptSpace); //    スペースおしまい

        //    複数ピリオドあったら前後につけるようにしたい
        for (m = 0; dPrdSp > m; m++)
        {
            StringCchCat(ptPlus, cchPlus, TEXT("."));
        }
    }

    if (pdZen)
        *pdZen = dZenSp;
    if (pdHan)
        *pdHan = dHanSp;
    if (pdPrd)
        *pdPrd = dPrdSp;

    return ptPlus;
}
//-------------------------------------------------------------------------------------------------

// 行頭に、文字(主に空白)を追加
HRESULT DocTopLetterInsert(TCHAR ch, PINT pXdot, INT dLine)
{
    UINT_PTR iLines;
    INT iTop, iBottom, i, xDot = 0;
    BOOLEAN bFirst = TRUE, bSeled = FALSE;

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
    if (0 <= iTop && 0 <= iBottom)
        bSeled = TRUE;

    if (0 > iTop)
        iTop = 0;
    if (0 > iBottom)
        iBottom = iLines - 1;

    //    選択範囲は、操作した行全体を選択状態にする

    for (i = iTop; iBottom >= i; i++) //    範囲内の各行について
    {
        //    先頭位置に文字桃得留。
        xDot = DocInputLetter(0, i, ch);

        SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog),
                        DO_INSERT, ch, 0, i, bFirst);
        bFirst = FALSE;

        if (bSeled)
        {
            DocRangeSelStateToggle(-1, -1, i, 1); //    該当行全体を選択状態にする
            DocReturnSelStateToggle(i, 1);        //    改行も選択で
        }

        DocBadSpaceCheck(i);
        DocViewRefreshLine(i);
    }

    //    キャレット位置ずれてたら適当に調整
    *pXdot += xDot;
    DocLetterPosGetAdjust(pXdot, dLine, 0); //    キャレット位置適当に調整
    DocViewMoveCaret(*pXdot, dLine);

    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 行頭全角空白及びユニコード空白を削除する
HRESULT DocTopSpaceErase(PINT pXdot, INT dLine)
{
    UINT_PTR iLines;
    INT iTop, iBottom, i;
    BOOLEAN bFirst = TRUE, bSeled = FALSE;
    TCHAR ch;

    LETR_ITR vcLtrItr;

    LINE_ITR itLine;

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
    if (0 <= iTop && 0 <= iBottom)
        bSeled = TRUE;

    if (0 > iTop)
        iTop = 0;
    if (0 > iBottom)
        iBottom = iLines - 1;
    //    選択範囲は、操作した行全体を選択状態にする

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, iTop); //    位置合わせ

    for (i = iTop; iBottom >= i; i++, itLine++) //    範囲内の各行について
    {
        //    文字があるなら操作する
        if (0 != itLine->vcLine.size())
        {
            vcLtrItr = itLine->vcLine.begin();
            ch = vcLtrItr->cchMozi;

            //    空白かつ半角ではない
            if ((iswspace(ch) && TEXT(' ') != ch))
            {
                SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog),
                                DO_DELETE, ch, 0, i, bFirst);
                bFirst = FALSE;

                DocIterateDelete(vcLtrItr, i);
            }
        }

        if (bSeled)
        {
            DocRangeSelStateToggle(-1, -1, i, 1); //    該当行全体を選択状態にする
            DocReturnSelStateToggle(i, 1);        //    改行も選択で
        }

        DocBadSpaceCheck(i); //    状態をリセット
        DocViewRefreshLine(i);
    }

    //    キャレット位置ずれてたら適当に調整
    *pXdot = 0;
    DocLetterPosGetAdjust(pXdot, dLine, 0); //    キャレット位置適当に調整
    DocViewMoveCaret(*pXdot, dLine);

    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
// 行末文字を削除する。ただし空白だったら削除しない
HRESULT DocLastLetterErase(PINT pXdot, INT dLine)
{
    UINT_PTR iLines;
    INT iTop, iBottom, i, xDot = 0;
    TCHAR ch;
    BOOLEAN bFirst = TRUE, bSeled = FALSE;
    RECT rect;

    LETR_ITR vcLtrItr;

    LINE_ITR itLine;

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
    if (0 <= iTop && 0 <= iBottom)
        bSeled = TRUE;

    if (0 > iTop)
        iTop = 0;
    if (0 > iBottom)
        iBottom = iLines - 1;
    //    選択してる場合は、操作行を全選択状態にする
    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, iTop); //    位置合わせ

    for (i = iTop; iBottom >= i; i++, itLine++) //    範囲内の各行について
    {
        //    文字があるなら操作する
        if (0 != itLine->vcLine.size())
        {
            vcLtrItr = itLine->vcLine.end();
            vcLtrItr--; //    終端の一個前が末端文字
            ch = vcLtrItr->cchMozi;

            rect.top = i * LINE_HEIGHT;
            rect.bottom = rect.top + LINE_HEIGHT;

            if (!(iswspace(ch)))
            {
                xDot = DocLineParamGet(i, nullptr, nullptr);

                xDot -= vcLtrItr->rdWidth;

                SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog),
                                DO_DELETE, ch, xDot, i, bFirst);
                bFirst = FALSE;

                DocIterateDelete(vcLtrItr, i);

                rect.left = xDot;
                rect.right = xDot + 40; //    壱文字＋改行・適当でよろし

                DocViewRefreshRect(&rect);

                DocBadSpaceCheck(i); //    良くないスペースを調べておく
            }
        }

        if (bSeled)
        {
            DocRangeSelStateToggle(-1, -1, i, 1); //    該当行全体を選択状態にする
            DocReturnSelStateToggle(i, 1);        //    改行も選択で
        }
    }

    //    キャレット位置適当に調整
    *pXdot = 0;
    DocLetterPosGetAdjust(pXdot, dLine, 0); //    キャレット位置適当に調整
    DocViewMoveCaret(*pXdot, dLine);

    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 行末空白削除の面倒見る・選択行とか
HRESULT DocLastSpaceErase(PINT pXdot, INT dLine)
{
    UINT_PTR iLines;
    INT iTop, iBottom, i, xDelDot, xMotoDot;
    BOOLEAN bFirst = TRUE;
    LPTSTR ptBuffer = nullptr;
    RECT rect;

    LINE_ITR itLine;

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;

    if (0 > iTop)
        iTop = 0;
    if (0 > iBottom)
        iBottom = iLines - 1;

    DocViewClearSelection();

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, iTop); //    位置合わせ

    for (i = iTop; iBottom >= i; i++, itLine++)
    {
        xMotoDot = itLine->iDotCnt;
        ptBuffer = DocLastSpDel(&(itLine->vcLine));
        xDelDot = DocLineParamGet(
            i, nullptr, nullptr); //    サクった後の行末端すなわち削除位置

        if (ptBuffer)
        {
            SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog),
                            DO_DELETE, ptBuffer, xDelDot, i, bFirst);
            bFirst = FALSE;
        }

        FREE(ptBuffer);

        DocBadSpaceCheck(i); //    状態をリセット・中で行書換でいいか？

        rect.top = i * LINE_HEIGHT;
        rect.bottom = rect.top + LINE_HEIGHT;
        rect.left = xDelDot;        //    削ったら左側になる
        rect.right = xMotoDot + 20; //    元長さ＋改行マーク
        DocViewRefreshRect(&rect);
    }

    //    キャレット位置ずれてたら適当に調整
    DocLetterPosGetAdjust(pXdot, dLine, 0); //    キャレット位置適当に調整
    DocViewMoveCaret(*pXdot, dLine);

    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 行末空白削除・コア部分
LPTSTR DocLastSpDel(vector<LETTER> *vcTgLine)
{
    UINT_PTR cchSize;
    LPTSTR ptBuffer = nullptr;
    wstring wsDelBuf;
    LETR_ITR itLtr, itDel;

    if (0 >= vcTgLine->size())
        return nullptr;

    itLtr = vcTgLine->end();
    itLtr--;

    //    末尾から逆に見ていく
    for (; itLtr != vcTgLine->begin(); itLtr--)
    {
        if (!(iswspace(itLtr->cchMozi))) //    空白じゃなくなったら
        {
            itLtr++; //    その次からがターゲット
            break;
        }
    }
    if (itLtr == vcTgLine->begin()) //    先頭文字確認
    {
        //    空白じゃなくなったらその次からがターゲット
        if (!(iswspace(itLtr->cchMozi)))
        {
            itLtr++;
        }
    }

    //    空白エリアがないなら特にすることはない
    if (itLtr == vcTgLine->end())
    {
        return nullptr;
    }

    wsDelBuf.clear();
    for (itDel = itLtr; vcTgLine->end() != itDel; itDel++)
    {
        wsDelBuf += itDel->cchMozi;
    }

    cchSize = wsDelBuf.size() + 1;
    ptBuffer = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
    StringCchCopy(ptBuffer, cchSize, wsDelBuf.c_str());

    //    該当部分を削除
    vcTgLine->erase(itLtr, vcTgLine->end());

    return ptBuffer;
}
//-------------------------------------------------------------------------------------------------

// 指定範囲を削除する
UINT DocRangeDeleteByMozi(INT xDot, INT yLine, INT dBgnMozi, INT dEndMozi,
                          PBOOLEAN pFirst)
{
    UINT_PTR cchSize;
    INT iBytes;
    LPTSTR ptBuffer;
    LETR_ITR vcLtrBgn, vcLtrEnd, vcItr;
    wstring wsDelBuf;

    LINE_ITR itLine;

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, yLine);

    vcLtrBgn = itLine->vcLine.begin();
    vcLtrEnd = itLine->vcLine.begin();
    vcLtrBgn += dBgnMozi; //    該当位置まで移動して
    vcLtrEnd += dEndMozi; //    そのエリアの終端も確認

    wsDelBuf.clear();
    iBytes = 0;
    for (vcItr = vcLtrBgn; vcLtrEnd != vcItr; vcItr++)
    {
        wsDelBuf += vcItr->cchMozi;
        iBytes += vcItr->mzByte;
    }

    //    該当部分を削除
    itLine->vcLine.erase(vcLtrBgn, vcLtrEnd);
    itLine->iByteSz -= iBytes;
    //    アンドゥバッファ作成
    cchSize = wsDelBuf.size() + 1;
    ptBuffer = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
    StringCchCopy(ptBuffer, cchSize, wsDelBuf.c_str());
    SqnAppendString(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog), DO_DELETE,
                    ptBuffer, xDot, yLine, *pFirst);
    FREE(ptBuffer);
    *pFirst = FALSE;

    return (UINT)(cchSize - 1);
}
//-------------------------------------------------------------------------------------------------

// ユニコードの使用不使用とか考慮しつつ埋め空白を作る
LPTSTR DocPaddingSpaceMake(INT dTgtDot)
{
    LPTSTR ptReplc = nullptr;
    INT dZenSp, dHanSp, dUniSp;

    //    幅０だと作れない
    if (0 >= dTgtDot)
        return nullptr;

    if (gbUniPad)
    {
        ptReplc = DocPaddingSpace(dTgtDot, &dZenSp, &dHanSp);
        //    作成不可だったり半角多すぎたら、ユニコード使って作り直し
        if (!(ptReplc) || (dZenSp < dHanSp)) //    (dZenSp + 1)
        {
            FREE(ptReplc);
            ptReplc = DocPaddingSpaceUni(dTgtDot, &dZenSp, &dHanSp, &dUniSp);
        }
    }
    else //    ユニコード空白使わないなら
    {
        ptReplc = DocPaddingSpaceWithGap(dTgtDot, &dZenSp, &dHanSp);
    }

    return ptReplc;
}
//-------------------------------------------------------------------------------------------------

// ＡＡ全体を、１dotずつずらす。文字なら空白に置き換えながら
HRESULT DocPositionShift(UINT vk, PINT pXdot, INT dLine)
{
    UINT_PTR iLines, cchSz;
    INT iTop, iBottom, i;
    INT wid, iDot, iLin, iMzCnt;
    INT iTgtWid, iLefDot, iRitDot;
    BOOLEAN bFirst = TRUE, bSeled = FALSE, bDone = FALSE;
    BOOLEAN bRight; //    非０右へ　０左へ
    BOOLEAN bIsSp;
    LPTSTR ptRepl;
    TCHAR ch, chOneSp;

    LPUNDOBUFF pstUndoBuff;

    LETR_ITR vcLtrItr;
    LINE_ITR itLine;

    chOneSp = gaatPaddingSpDotW[1][0];

    pstUndoBuff = &((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog);

    if (VK_RIGHT == vk)
        bRight = TRUE;
    else if (VK_LEFT == vk)
        bRight = FALSE;
    else
        return E_INVALIDARG;

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
    if (0 <= iTop && 0 <= iBottom)
        bSeled = TRUE;

    if (0 > iTop)
        iTop = 0;
    if (0 > iBottom)
        iBottom = iLines - 1;

    //    そのままだと容量が狂う・一旦選択状態を解除して計算しなおす
    if (bSeled)
    {
        DocPageSelStateToggle(-1);
    }

    //    壱行ずつ面倒見ていく
    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, iTop); //    位置合わせ

    for (i = iTop; iBottom >= i; i++, itLine++)
    {
        //    文字があるなら操作する
        if (0 != itLine->vcLine.size())
        {
            //    先頭文字を確認
            vcLtrItr = itLine->vcLine.begin();
            ch = vcLtrItr->cchMozi;
            wid = vcLtrItr->rdWidth; //    文字幅

            bDone = FALSE;

            if (!(iswspace(ch))) //    空白ではなく
            {
                if (bRight) //    右ずらしなら
                {
                    //    先頭に1dotスペース足せばおｋ
                    DocInputLetter(0, i, chOneSp);
                    SqnAppendLetter(pstUndoBuff, DO_INSERT, chOneSp, 0, i, bFirst);
                    bFirst = FALSE;
                    bDone = TRUE; //    処理しちゃった
                }
                else //    左イクなら、先頭文字を空白にして調整する
                {
                    ptRepl = DocPaddingSpaceMake(wid); //    必要な空白確保
                    StringCchLength(ptRepl, STRSAFE_MAX_CCH, &cchSz);
                    //    今の文字を削除
                    SqnAppendLetter(pstUndoBuff, DO_DELETE, ch, 0, i, bFirst);
                    bFirst = FALSE;
                    DocIterateDelete(vcLtrItr, i);
                    //    そして先頭に空白をアッー！
                    iDot = 0;
                    iLin = i;
                    DocStringAdd(&iDot, &iLin, ptRepl, cchSz);
                    SqnAppendString(pstUndoBuff, DO_INSERT, ptRepl, 0, i, bFirst);
                    bFirst = FALSE;
                    FREE(ptRepl); //    開放忘れないように
                }
            }
            //    この先Beginイテレータ無効

            if (!(bDone)) //    未処理であるなら・この時点で、先頭文字は空白確定
            {
                //    空白範囲を確認
                iTgtWid = DocLineStateCheckWithDot(0, i, &iLefDot, &iRitDot, nullptr,
                                                   &iMzCnt, &bIsSp);
                if (bRight)
                    iTgtWid++; //    方向に合わせて
                else
                    iTgtWid--; //    ドット数を求める
                if (0 > iTgtWid)
                    iTgtWid = 0; //    マイナスは無いと思うけど念のため

                ptRepl = DocPaddingSpaceMake(iTgtWid); //    必要な空白確保
                //    ターゲット幅が０ならnullptrなので、先頭文字の削除だけでおｋ

                DocRangeDeleteByMozi(0, i, 0, iMzCnt, &bFirst); //    元の部分削除して

                if (ptRepl) //    必要な文字を入れる
                {
                    StringCchLength(ptRepl, STRSAFE_MAX_CCH, &cchSz);
                    iDot = 0;
                    iLin = i;
                    DocStringAdd(&iDot, &iLin, ptRepl, cchSz);
                    SqnAppendString(pstUndoBuff, DO_INSERT, ptRepl, 0, i, bFirst);
                    bFirst = FALSE;
                    FREE(ptRepl); //    開放忘れないように
                }
            }

            if (bSeled) //    選択状態でヤッてたのなら、選択状態を維持する
            {
                DocRangeSelStateToggle(-1, -1, i, 1); //    該当行全体を選択状態にする
                //    次の行があるなら改行も選択で
                if (iBottom > i)
                    DocReturnSelStateToggle(i, 1);
            }

            DocBadSpaceCheck(i); //    状態をリセット
            DocViewRefreshLine(i);
        }
    }

    if (bSeled) //    選択範囲はり直し
    {
        (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop = iTop;
        (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom = iBottom;
    }

    //    キャレット位置調整
    iDot = 0;
    DocLetterPosGetAdjust(&iDot, dLine, 0); //    キャレット位置適当に調整
    DocViewMoveCaret(iDot, dLine);

    DocPageByteCount(gitFileIt, gixFocusPage, nullptr, nullptr);
    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 行頭半角空白をユニコードに変換・ファイルコア函数
HRESULT DocHeadHalfSpaceExchange(HWND hWnd)
{
    UINT_PTR iLines;
    INT iTop, iBottom, i;
    INT xDot;
    BOOLEAN bFirst = TRUE, bSeled = FALSE;
    TCHAR ch;

    LETR_ITR vcLtrItr;
    LINE_ITR itLine;

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
    if (0 <= iTop && 0 <= iBottom)
        bSeled = TRUE;

    if (0 > iTop)
        iTop = 0;
    if (0 > iBottom)
        iBottom = iLines - 1;

    DocViewClearSelection();

    //    容量計算、バッド空白の確認と再描画必要

    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, iTop); //    行の位置合わせ

    for (i = iTop; iBottom >= i; i++, itLine++) //    範囲内の各行について
    {
        //    文字があるなら操作する
        if (0 != itLine->vcLine.size())
        {
            vcLtrItr = itLine->vcLine.begin();
            ch = vcLtrItr->cchMozi;

            if (TEXT(' ') == ch) //    半角なら
            {
                //    一旦削除して
                SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog),
                                DO_DELETE, ch, 0, i, bFirst);
                bFirst = FALSE;
                DocIterateDelete(vcLtrItr, i);

                //    先頭位置に5dotユニコード空白をアッー！。
                ch = (TCHAR)0x2004;
                xDot = DocInputLetter(0, i, ch);
                SqnAppendLetter(&((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog),
                                DO_INSERT, ch, 0, i, bFirst);
            }

            DocBadSpaceCheck(i); //    状態をリセット
            DocViewRefreshLine(i);
        }
    }

    //    キャレット位置はずれない

    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

#ifdef DOT_SPLIT_MODE
// キャレット位置から、左右に１dotずつずらす。文字なら空白に置き換えながら
HRESULT DocCentreWidthShift(UINT vk, PINT pXdot, INT dLine)
{
    UINT_PTR iLines, cchSz;
    UINT dRslt, dFirst;
    INT iBaseDot, iTop, iBottom, iBufDot;
    INT wid, iDot, iLin, iMzCnt;
    INT iFnlDot;
    BOOLEAN bSeled = FALSE;
    BOOLEAN bRight; //    非０右へ　０左へ
    LPTSTR ptRepl;
    TCHAR ch, chOneSp;

    LPUNDOBUFF pstUndoBuff;

    LETR_ITR vcLtrItr;
    LINE_ITR itLine;

    chOneSp = gaatPaddingSpDotW[1][0]; //    幅1dot・文字間に挿入

    pstUndoBuff = &((*gitFileIt).vcCont.at(gixFocusPage).stUndoLog);

    iBaseDot = *pXdot; //    基準点、なるべく動かないようにせないかん

    iFnlDot = iBaseDot;

    if (VK_RIGHT == vk)
        bRight = TRUE;
    else if (VK_LEFT == vk)
        bRight = FALSE;
    else
        return E_INVALIDARG;

    if (0 == iBaseDot) //    基準が０なら、全体左右ずらしってこと
    {
        return DocPositionShift(vk, pXdot, dLine);
    }

    //    範囲確認
    iLines = DocNowFilePageLineCount();
    iTop = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop;
    iBottom = (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom;
    if (0 <= iTop && 0 <= iBottom)
        bSeled = TRUE;

    if (0 > iTop)
        iTop = 0;
    if (0 > iBottom)
        iBottom = iLines - 1;

    //    そのままだと容量が狂う・一旦選択状態を解除して計算しなおす
    if (bSeled)
    {
        DocPageSelStateToggle(-1);
    }

    //    壱行ずつ面倒見ていく
    itLine = (*gitFileIt).vcCont.at(gixFocusPage).ltPage.begin();
    std::advance(itLine, iTop); //    位置合わせ

    // なんか時々連続空白ができる

    dFirst = TRUE;
    //    順番に処理していく
    for (iLin = iTop; iBottom >= iLin; iLin++, itLine++)
    {
        iDot = itLine->iDotCnt;
        if (iBaseDot >= iDot)
        {
            continue;
        }
        //    操作位置に満たないのなら、何もする必要は無い

        //    操作開始
        iDot = iBaseDot;                                 //    調整位置確定
        iMzCnt = DocLetterPosGetAdjust(&iDot, iLin, -1); //    常に左側をみる
        //    操作する位置の文字確認

        //    該当位置が空白なら、伸び縮み兼用
        iBufDot = iDot; //    値ズレるのでそのまま使うとイケない
        dRslt = DocSpaceDifference(vk, &iBufDot, iLin,
                                   dFirst); //    iBufDotはズラしたら使えない
        if (dRslt)                          //    ズラし成功
        {
            if (iLin == dLine)
            {
                iFnlDot = iBaseDot;
            } //    ずらした後の位置の面倒見る
            dFirst = FALSE;
        }
        else //    返り値０なら、文字なので処理を
        {
            vcLtrItr = itLine->vcLine.begin();
            std::advance(vcLtrItr, iMzCnt); //    注目位置の文字まで移動して
            ch = vcLtrItr->cchMozi;
            wid = vcLtrItr->rdWidth; //    該当の文字の幅を確認

            if (bRight) //    右に動かす
            {
                if (iswspace(ch)) //    右側の文字は空白であったら
                {
                    iBufDot = iDot + wid; //    その空白を延ばす
                    DocSpaceDifference(vk, &iBufDot, iLin,
                                       dFirst); //    iBufDotは使えない
                    if (iLin == dLine)
                    {
                        iFnlDot = iBaseDot;
                    } //    ずらした後の位置の面倒見る
                }
                else
                {
                    SqnAppendLetter(pstUndoBuff, DO_INSERT, chOneSp, iDot, iLin, dFirst);
                    DocInputLetter(iDot, iLin,
                                   chOneSp); //    その場所に1dotスペース足せばおｋ
                    if (iLin == dLine)
                    {
                        iFnlDot = iDot + 1;
                    } //    ずらした後の位置の面倒見る
                }
                dFirst = FALSE;
            }
            else //    左に動かす
            {
                if (iLin == dLine)
                {
                    iFnlDot = iDot;
                } //    ずらす前の位置の面倒見る

                //    今の文字を削除
                SqnAppendLetter(pstUndoBuff, DO_DELETE, ch, iDot, iLin, dFirst);
                dFirst = FALSE;
                DocIterateDelete(vcLtrItr, iLin);
                if (2 <= wid) //    幅が１なら、削除だけでおｋ
                {
                    ptRepl = DocPaddingSpaceMake(wid - 1);            //    必要な空白確保
                    StringCchLength(ptRepl, STRSAFE_MAX_CCH, &cchSz); //    文字数確認
                    SqnAppendString(pstUndoBuff, DO_INSERT, ptRepl, iDot, iLin, dFirst);
                    dFirst = FALSE;
                    DocStringAdd(&iDot, &iLin, ptRepl,
                                 cchSz); //    そして先頭に空白をアッー！
                    FREE(ptRepl);        //    開放忘れないように
                }
            }
        }

        if (bSeled) //    選択状態でヤッてたのなら、選択状態を維持する
        {
            if (iLin == iTop)
            {
                iDot = iBaseDot;
                DocLetterPosGetAdjust(&iDot, iLin, 0); //    キャレット位置適当に調整
                DocRangeSelStateToggle(iDot, -1, iLin,
                                       1); //    該当行全体を選択状態にする
            }
            else
            {
                DocRangeSelStateToggle(-1, -1, iLin,
                                       1); //    該当行全体を選択状態にする
            }

            //    次の行があるなら改行も選択で
            if (iBottom > iLin)
                DocReturnSelStateToggle(iLin, 1);
        }

        DocBadSpaceCheck(iLin); //    状態をリセット
        DocViewRefreshLine(iLin);
    }

    if (bSeled) //    選択範囲はり直し
    {
        (*gitFileIt).vcCont.at(gixFocusPage).dSelLineTop = iTop;
        (*gitFileIt).vcCont.at(gixFocusPage).dSelLineBottom = iBottom;
    }

    //    キャレット位置調整
    DocLetterPosGetAdjust(&iFnlDot, dLine, 0); //    キャレット位置適当に調整
    *pXdot = iFnlDot;                          //    位置を戻す
    DocViewMoveCaret(iFnlDot, dLine);
    //    再描画
    DocPageByteCount(gitFileIt, gixFocusPage, nullptr, nullptr);
    DocPageInfoRenew(-1, 1);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
#endif
