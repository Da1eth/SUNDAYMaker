#include "ViewCentralInternal.h"

static POINT gstSqSelBegin; // 始点
static POINT gstSqSelEnd;   // 終点

static POINT gstSelBgnOrig; // 範囲選択を開始した地点
static POINT gstSelEndOrig; // 範囲選択を終了した地点

static POINT gstPrePos; // 直前の選択位置

static BOOLEAN gbSelecting; // 選択操作中か？
EXTERNED UINT gbSqSelect;   // 矩形選択中である D_SQUARE

HRESULT ViewSelStateChange(UINT);
HRESULT ViewSqSelAdjust(INT);
static VOID ViewSelTrackedBoundsReset(VOID);
static VOID ViewSelTrackedBoundsPrime(INT dXdot, INT dLine);
static VOID ViewSelRefreshSelectingState(VOID);
static HRESULT ViewSelStateChangeExtract(VOID);
static VOID ViewSelToggleSquareDelta(INT dAnchorX, INT dAnchorY, INT dPrevX, INT dPrevY, INT dNowX, INT dNowY, INT dForce);
static VOID ViewSelUpdateRectBounds(INT dAnchorX, INT dAnchorY, INT dNowX, INT dNowY);
static VOID ViewSelApplyLinearDelta(INT dBeginDot, INT dEndDot, INT dBaseLine, INT dStep, INT dCaretXdot, INT dCaretLine, INT dForce);

static VOID ViewSelTrackedBoundsReset(VOID)
{
    gstSqSelBegin.x = -1;
    gstSqSelBegin.y = -1;
    gstSqSelEnd.x = -1;
    gstSqSelEnd.y = -1;

    gstSelBgnOrig.x = -1;
    gstSelBgnOrig.y = -1;
    gstSelEndOrig.x = -1;
    gstSelEndOrig.y = -1;
}

static VOID ViewSelTrackedBoundsPrime(INT dXdot, INT dLine)
{
    gstSqSelBegin.x = dXdot;
    gstSqSelBegin.y = dLine;
    gstSqSelEnd = gstSqSelBegin;

    gstSelBgnOrig = gstSqSelBegin;
    gstSelEndOrig = gstSqSelBegin;
}

static VOID ViewSelRefreshSelectingState(VOID)
{
    INT dTop;
    INT dBottom;

    DocSelRangeReset(&dTop, &dBottom);
    gbSelecting = (0 <= dTop) && (0 <= dBottom);
    if (!(gbSelecting))
    {
        ViewSelTrackedBoundsReset();
    }
}

static VOID ViewSelUpdateRectBounds(INT dAnchorX, INT dAnchorY, INT dNowX, INT dNowY)
{
    if (dAnchorY > dNowY)
    {
        gstSqSelBegin.y = dNowY;
        gstSqSelEnd.y = dAnchorY;
    }
    else
    {
        gstSqSelBegin.y = dAnchorY;
        gstSqSelEnd.y = dNowY;
    }

    if (dAnchorX > dNowX)
    {
        gstSqSelBegin.x = dNowX;
        gstSqSelEnd.x = dAnchorX;
    }
    else
    {
        gstSqSelBegin.x = dAnchorX;
        gstSqSelEnd.x = dNowX;
    }
}

static VOID ViewSelApplyLinearDelta(INT dBeginDot, INT dEndDot, INT dBaseLine, INT dStep, INT dCaretXdot, INT dCaretLine, INT dForce)
{
    INT dJpLn;

    DocRangeSelStateToggle(dBeginDot, dEndDot, dBaseLine, dForce);

    if (1 <= dStep)
    {
        DocReturnSelStateToggle(dBaseLine, dForce);

        for (dJpLn = (dBaseLine + 1); dCaretLine > dJpLn; dJpLn++)
        {
            DocReturnSelStateToggle(dJpLn, dForce);
            DocRangeSelStateToggle(0, -1, dJpLn, dForce);
        }

        DocRangeSelStateToggle(0, dCaretXdot, dCaretLine, dForce);
        return;
    }

    if (-1 >= dStep)
    {
        for (dJpLn = (dCaretLine + 1); dBaseLine > dJpLn; dJpLn++)
        {
            DocReturnSelStateToggle(dJpLn, dForce);
            DocRangeSelStateToggle(0, -1, dJpLn, dForce);
        }

        DocReturnSelStateToggle(dCaretLine, dForce);
        DocRangeSelStateToggle(dCaretXdot, -1, dCaretLine, dForce);
    }
}

static VOID ViewSelToggleSquareDelta(INT dAnchorX, INT dAnchorY, INT dPrevX, INT dPrevY, INT dNowX, INT dNowY, INT dForce)
{
    const BOOLEAN bAddOnly = (0 < dForce);
    const INT dOldTop = min(dAnchorY, dPrevY);
    const INT dOldBottom = max(dAnchorY, dPrevY);
    const INT dNewTop = min(dAnchorY, dNowY);
    const INT dNewBottom = max(dAnchorY, dNowY);
    const INT dUnionTop = min(dOldTop, dNewTop);
    const INT dUnionBottom = max(dOldBottom, dNewBottom);

    for (INT iLine = dUnionTop; iLine <= dUnionBottom; ++iLine)
    {
        BOOLEAN bHasOld = (dOldTop <= iLine) && (iLine <= dOldBottom);
        BOOLEAN bHasNew = (dNewTop <= iLine) && (iLine <= dNewBottom);
        INT dOldBegin = 0;
        INT dOldEnd = 0;
        INT dNewBegin = 0;
        INT dNewEnd = 0;

        if (bHasOld)
        {
            dOldBegin = min(dAnchorX, dPrevX);
            dOldEnd = max(dAnchorX, dPrevX);
            DocLetterPosGetAdjust(&dOldBegin, iLine, 0);
            DocLetterPosGetAdjust(&dOldEnd, iLine, 0);
        }

        if (bHasNew)
        {
            dNewBegin = min(dAnchorX, dNowX);
            dNewEnd = max(dAnchorX, dNowX);
            DocLetterPosGetAdjust(&dNewBegin, iLine, 0);
            DocLetterPosGetAdjust(&dNewEnd, iLine, 0);
        }

        if (bHasOld && bHasNew)
        {
            if (dOldBegin < dNewBegin)
            {
                if (!(bAddOnly))
                {
                    DocRangeSelStateToggle(dOldBegin, min(dOldEnd, dNewBegin), iLine, 0);
                }
            }
            else if (dNewBegin < dOldBegin)
            {
                DocRangeSelStateToggle(dNewBegin, min(dNewEnd, dOldBegin), iLine, dForce);
            }

            if (dOldEnd < dNewEnd)
            {
                DocRangeSelStateToggle(max(dOldEnd, dNewBegin), dNewEnd, iLine, dForce);
            }
            else if (dNewEnd < dOldEnd)
            {
                if (!(bAddOnly))
                {
                    DocRangeSelStateToggle(max(dNewEnd, dOldBegin), dOldEnd, iLine, 0);
                }
            }

            continue;
        }

        if (bHasOld)
        {
            if (!(bAddOnly))
            {
                DocRangeSelStateToggle(dOldBegin, dOldEnd, iLine, 0);
            }
            continue;
        }

        if (bHasNew)
        {
            DocRangeSelStateToggle(dNewBegin, dNewEnd, iLine, dForce);
        }
    }
}

static HRESULT ViewSelStateChangeExtract(VOID)
{
    const INT dForce = gbShiftOn ? 1 : 0;
    INT dBeginDot;
    INT dEndDot;
    INT dStep = 0;
    INT dBaseLine;
    auto caret = ViewCurrentCaret();

    if (gstPrePos.y != caret.dLine)
    {
        dStep = caret.dLine - gstPrePos.y;

        if (0 < dStep)
        {
            dBeginDot = gstPrePos.x;
            dEndDot = -1;
        }
        else
        {
            dBeginDot = 0;
            dEndDot = gstPrePos.x;
        }

        gstSqSelEnd.x = caret.dXdot;
        dBaseLine = gstPrePos.y;
    }
    else
    {
        if (gstPrePos.x < caret.dXdot)
        {
            dBeginDot = gstPrePos.x;
            dEndDot = caret.dXdot;
        }
        else
        {
            dBeginDot = caret.dXdot;
            dEndDot = gstPrePos.x;
        }

        dBaseLine = caret.dLine;
    }

    gstSelEndOrig.x = caret.dXdot;
    gstSelEndOrig.y = caret.dLine;
    ViewSelUpdateRectBounds(gstSelBgnOrig.x, gstSelBgnOrig.y, gstSelEndOrig.x, gstSelEndOrig.y);

    if (gbSqSelect)
    {
        ViewSelToggleSquareDelta(gstSelBgnOrig.x, gstSelBgnOrig.y, gstPrePos.x, gstPrePos.y, caret.dXdot, caret.dLine, dForce);
    }
    else
    {
        ViewSelApplyLinearDelta(dBeginDot, dEndDot, dBaseLine, dStep, caret.dXdot, caret.dLine, dForce);
    }

    ViewSelRefreshSelectingState();

    return S_OK;
}

BOOLEAN IsSelecting(PUINT pSqSel)
{
    if (pSqSel)
        *pSqSel = gbSqSelect;

    return gbSelecting;
}

// カーソル操作したときとか、マウスクルックでカーソル移動したときとかに呼ばれる
HRESULT ViewSelPositionSet(LPVOID pVoid)
{
    INT iBgn, iEnd, iDmy = 0;
    auto caret = ViewCurrentCaret();

    //    ルーラ再描画・Ｘ移動無しなら描画の必要は無い
    if ((gstPrePos.x != caret.dXdot))
    {
        iBgn = gstPrePos.x;
        if (0 > iBgn)
            iBgn = 0;
        iEnd = gstPrePos.x + 1;
        ViewPositionTransform(&iBgn, &iDmy, 1);
        ViewPositionTransform(&iEnd, &iDmy, 1);
        ViewRulerRedraw(iBgn, iEnd); //    更新!?範囲をいれる

        iBgn = caret.dXdot;
        if (0 > iBgn)
            iBgn = 0;
        iEnd = caret.dXdot + 1;
        ViewPositionTransform(&iBgn, &iDmy, 1);
        ViewPositionTransform(&iEnd, &iDmy, 1);
        ViewRulerRedraw(iBgn, iEnd); //    更新!?範囲をいれる
    }

    gstPrePos.x = caret.dXdot;
    gstPrePos.y = caret.dLine;

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 矩形選択モードのON/OFFトグル
UINT ViewSqSelModeToggle(UINT bMode, LPVOID pVoid)
{
    POINT point;

    //    マウスドラッグ中だけモード変更を止める
    if (ghViewWnd == GetCapture())
        return gbSqSelect; //    なぜかRETURNが抜けてた？

    if (bMode) //    20120313
    {
        gbSqSelect ^= D_SQUARE;
    }
    else
    {
        if (gbAltOn)
        {
            gbSqSelect |= D_SQUARE;
        }
        //    20120323    Alt押されてたらON、違うなら素通り
    }

    //    開始しても終了しても初期化するのは変わらない
    gstSqSelBegin.x = -1;
    gstSqSelBegin.y = -1;
    gstSqSelEnd.x = -1;
    gstSqSelEnd.y = -1;

    MenuItemCheckOnOff(IDM_SQSELECT, gbSqSelect); //    メニューチェック

    OperationOnStatusBar();

    //    カーソルを変更してみる・矩形ならクロスで
    if (D_SQUARE & gbSqSelect)
    {
        SetClassLongPtr(ghViewWnd, GCLP_HCURSOR, (LONG_PTR)(LoadCursor(nullptr, IDC_CROSS)));
    }
    else
    {
        SetClassLongPtr(ghViewWnd, GCLP_HCURSOR, (LONG_PTR)(LoadCursor(nullptr, IDC_IBEAM)));
    }
    GetCursorPos(&point);
    SetCursorPos(point.x, point.y);

    return gbSqSelect;
}
//-------------------------------------------------------------------------------------------------

// 選択範囲確認して、始点終点同じだったら解除する
UINT ViewSelRangeCheck(UINT dMode)
{
    const BOOLEAN bKeepSelection = gbExtract || (gbSqSelect && gbShiftOn);

    //    始点終点が同じ位置＝何も選択していない
    if (gstSelBgnOrig.x == gstSelEndOrig.x && gstSelBgnOrig.y == gstSelEndOrig.y)
    {
        if (bKeepSelection)
        {
            ViewSelRefreshSelectingState();
            return 0;
        }

        if (IsSelecting(nullptr))
        {
            ViewSelPageAll(-1); //    中の中でDocSelRangeSet
        }

        ViewSelTrackedBoundsReset();

        return 0;
    }

    return 1;
}
//-------------------------------------------------------------------------------------------------

// 選択するかどうか確認して、始点とかいれるかも・キーマウス両方から来る
HRESULT ViewSelMoveCheck(UINT dMode)
{
    auto caret = ViewCurrentCaret();
    const BOOLEAN bSpecialAccumulation = gbExtract || (gbSqSelect && gbShiftOn);

    if (gbSelecting)
    {
        if (bSpecialAccumulation && !(dMode))
        {
            ViewSelTrackedBoundsPrime(caret.dXdot, caret.dLine);
            return S_OK;
        }

        if (gbShiftOn || dMode) //    シフト押されてるか、ドラッグ選択中である
        {
            ViewSelStateChange(FALSE);
            ViewSelRangeCheck(dMode);
        }
        else
        {
            ViewSelPageAll(-1);
            ViewSelTrackedBoundsReset();
        }
    }
    else
    {
        //    未選択状態で、シフトオサレながらカーソルの移動があったら
        //    もしくはドラッグ選択なら
        if (gbShiftOn || dMode)
        {
            //    ALT押しながら選択開始したら、矩形選択をToggleする
            ViewSqSelModeToggle(0, nullptr);

            ViewSelTrackedBoundsPrime(gstPrePos.x, gstPrePos.y);
            gstSqSelEnd.x = caret.dXdot;
            gstSqSelEnd.y = caret.dLine;
            gstSelEndOrig = gstSqSelEnd;

            gbSelecting = TRUE; //    選択処理開始

            ViewSelStateChange(TRUE);
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ページ全体の選択状態をON/OFFする
INT ViewSelPageAll(INT dForce)
{
    if (0 < dForce)
        gbSelecting = TRUE; //    選択処理開始
    else if (0 > dForce)
    {
        gbSelecting = FALSE; //    選択処理終了
        ViewSelTrackedBoundsReset();
    }
    else
        return 0; //    ０なら処理しない

    return DocPageSelStateToggle(dForce);
}
//-------------------------------------------------------------------------------------------------

// 更新された選択範囲の、選択状態のON/OFFする
HRESULT ViewSelStateChange(UINT dFirst)
{
    //    直前のカーソル位置から、今の選択範囲END位置の間の文字
    //    これは範囲更新された範囲に含むやつの処理
    INT dBeginDot, dEndDot, dStep = 0;
    INT dBaseLine;
    auto caret = ViewCurrentCaret();

    //    直前の状態から行またぎあったかどうか-
    if (gstPrePos.y != caret.dLine)
    {
        dStep = caret.dLine - gstPrePos.y; //    マイナス方向に注意セヨ

        //    元々キャレットのあった行の処理
        //    逆方向への選択処理はここでやっていけるはず
        if (0 < dStep) //    末尾に向かって
        {
            dBeginDot = gstPrePos.x; //    キャレット位置から
            dEndDot = -1;            //    行終端まで
        }
        else //    差分０ならそもそもここまで来ないからおｋ
        {
            dBeginDot = 0;
            dEndDot = gstPrePos.x;
        }

        gstSqSelEnd.x = caret.dXdot; //    洗濯業範囲指定に使う

        dBaseLine = gstPrePos.y; //    元々キャレットの存在していた行
    }
    else //    またいでない
    {
        if (gstPrePos.x < caret.dXdot) //    末尾に向かって
        {
            dBeginDot = gstPrePos.x;
            dEndDot = caret.dXdot;
        }
        else //    銭湯に向かって
        {
            dBeginDot = caret.dXdot;
            dEndDot = gstPrePos.x;
        }

        dBaseLine = caret.dLine; //    元々キャレットの存在していた行
    }

    gstSelEndOrig.x = caret.dXdot;
    gstSelEndOrig.y = caret.dLine;
    ViewSelUpdateRectBounds(gstSelBgnOrig.x, gstSelBgnOrig.y, gstSelEndOrig.x, gstSelEndOrig.y);

    //    開始位置を挟むとき、それを失わないように修正
    //    上端、下端も失わないように注意セヨ

    if (gbExtract)
    {
        return ViewSelStateChangeExtract();
    }

    DocSelRangeSet(gstSqSelBegin.y, gstSqSelEnd.y);

    if (gbSqSelect) //    矩形の時は専用に処理する
    {
        ViewSqSelAdjust(dBaseLine);
    }
    else
    {
        ViewSelApplyLinearDelta(dBeginDot, dEndDot, dBaseLine, dStep, caret.dXdot, caret.dLine, 0);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 選択開始地点より前に選択しているか
UINT ViewSelBackCheck(INT line)
{
    //    選択範囲、左上右下調整・Orig位置は絶対的な内容のはず・Orig値を元にすればいいか？

    //    開始から上の行イッたら
    if (gstSelBgnOrig.y >= line)
    {
        return TRUE;
    }

    return FALSE;
}
//-------------------------------------------------------------------------------------------------

// 矩形選択の調整
HRESULT ViewSqSelAdjust(INT dBaseLine)
{
    INT i, xDotBegin, xDotEnd, xDotLast;
    //    もっと良いやり方ないか

    //    マウスクルックとかで、行単位で選択が変更された場合
    //    上が開いてる・開いてる所の選択解除
    if (dBaseLine < gstSqSelBegin.y)
    {
        for (i = dBaseLine; gstSqSelBegin.y > i; i++)
        {
            DocRangeSelStateToggle(0, -1, i, -1);
        }
    }
    //    下が開いてる
    if (gstSqSelEnd.y < dBaseLine)
    {
        for (i = gstSqSelEnd.y + 1; dBaseLine >= i; i++)
        {
            DocRangeSelStateToggle(0, -1, i, -1);
        }
    }

    for (i = gstSqSelBegin.y; gstSqSelEnd.y >= i; i++)
    {
        xDotBegin = gstSqSelBegin.x;
        DocLetterPosGetAdjust(&xDotBegin, i, 0); //    各行のキャレット位置の調整

        xDotEnd = (gstSqSelEnd.x < gstCursor.x) ? gstCursor.x : gstSqSelEnd.x;
        DocLetterPosGetAdjust(&xDotEnd, i, 0);

        //    末端確認
        xDotLast = DocLineParamGet(i, nullptr, nullptr);

        if (0 < xDotBegin) //    先頭から選択範囲直前まで
        {
            DocRangeSelStateToggle(0, xDotBegin, i, -1);
        }

        //    選択範囲
        DocRangeSelStateToggle(xDotBegin, xDotEnd, i, 1);

        if (xDotEnd < xDotLast) //    選択範囲の終わりから末端まで
        {
            DocRangeSelStateToggle(xDotEnd, -1, i, -1);
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// カーソルのある、空白列もしくは文字列を選択状態にする
HRESULT ViewSelAreaSelect(LPVOID pVoid)
{
    INT iBeginDot, iEndDot, iStCnt, iCount;
    INT iRangeDot;
    BOOLEAN bIsSpase;
    auto caret = ViewCurrentCaret();

    DocPageSelStateToggle(FALSE); //    一旦選択状態は解除

    iRangeDot = DocLineStateCheckWithDot(caret.dXdot, caret.dLine, &iBeginDot, &iEndDot, &iStCnt, &iCount, &bIsSpase);
    //    始点ドット・終点ドット・開始地点の文字数・間の文字数・該当はスペースであるか
    caret.dXdot = iBeginDot; //    選択範囲として移動する
    ViewSelMoveCheck(FALSE);
    ViewSelPositionSet(nullptr);

    //    ドラッグ移動を模擬的に行う

    caret.dXdot = iEndDot; //    選択範囲として移動する

    ViewDrawCaret(caret.dXdot, caret.dLine, 1); //    ここでキャレットも移動

    ViewSelMoveCheck(TRUE);
    ViewSelPositionSet(nullptr); //    移動した位置を記録

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
