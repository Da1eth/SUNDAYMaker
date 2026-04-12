#include "ViewCentralInternal.h"
#include "Palette.h"

extern UINT gbUniPad; // パディングにユニコードをつかって、ドットを見せないようにする

BOOLEAN gbShiftOn; // シフトが押されている
BOOLEAN gbCtrlOn;  // コントロールが押されている
BOOLEAN gbAltOn;   // アルタが押されている

POINT gstCursor; // 文字を考慮しない、Cursorのドット＆行位置

EXTERNED UINT gbPalBucketMode;        // 非零ブラシモード
static TCHAR gatBrushPtn[SUB_STRING]; // ブラシパヤーン

static UINT gdSqFillCnt; // 矩形選択を、IME文字列で塗りつぶした時の文字数

static UINT gbLDoubleClick; // ダブルクルックした

static POINT gstLClicken; // 左クルックした位置

enum VIEW_CHANGESET_REDRAW_KIND
{
    VIEW_CHANGESET_REDRAW_NONE = 0,
    VIEW_CHANGESET_REDRAW_LINE,
    VIEW_CHANGESET_REDRAW_RANGE,
    VIEW_CHANGESET_REDRAW_ALL,
};

enum VIEW_CHANGESET_CARET_KIND
{
    VIEW_CHANGESET_CARET_NONE = 0,
    VIEW_CHANGESET_CARET_MOVE,
};

struct VIEW_EDIT_CHANGESET
{
    UINT bRenewPageInfo{};
    VIEW_CHANGESET_REDRAW_KIND dRedrawKind{VIEW_CHANGESET_REDRAW_NONE};
    INT dRedrawTopLine{};
    INT dRedrawBottomLine{};
    VIEW_CHANGESET_CARET_KIND dCaretKind{VIEW_CHANGESET_CARET_NONE};
    INT dCaretXdot{};
    INT dCaretLine{};
};

HRESULT ViewBrushFilling(VOID);

HRESULT ViewScriptedLineFeed(VOID);
static VOID ViewChangeSetApply(const VIEW_EDIT_CHANGESET &stChangeSet);
static VOID ViewChangeSetRequestRedrawLine(VIEW_EDIT_CHANGESET *pstChangeSet, INT dLine);
static VOID ViewChangeSetRequestRedrawRange(VIEW_EDIT_CHANGESET *pstChangeSet, INT dTopLine, INT dBottomLine);
static VOID ViewChangeSetRequestRedrawAll(VIEW_EDIT_CHANGESET *pstChangeSet);
static VOID ViewChangeSetRequestCaretMove(VIEW_EDIT_CHANGESET *pstChangeSet, INT dXdot, INT dLine);
static VOID ViewRedrawChangedLines(INT dLine, INT iLines, INT bCrLf);
static INT ViewInsertLiteralCharacter(TCHAR ch);
static INT ViewInsertStringWithStyle(LPCTSTR ptText, UINT dStyle, PINT pdMozi);
static HRESULT ViewBrushFillAtCaret(VIEW_EDIT_CHANGESET *pstChangeSet);
static HRESULT ViewScriptedLineFeedAtDot(INT dTargetDot, VIEW_EDIT_CHANGESET *pstChangeSet);

static VOID ViewChangeSetApply(const VIEW_EDIT_CHANGESET &stChangeSet)
{
    switch (stChangeSet.dRedrawKind)
    {
    case VIEW_CHANGESET_REDRAW_ALL:
        ViewRedrawSetLine(-1);
        break;

    case VIEW_CHANGESET_REDRAW_RANGE:
        for (INT iLine = stChangeSet.dRedrawTopLine; stChangeSet.dRedrawBottomLine >= iLine; ++iLine)
        {
            ViewRedrawSetLine(iLine);
        }
        break;

    case VIEW_CHANGESET_REDRAW_LINE:
        ViewRedrawSetLine(stChangeSet.dRedrawTopLine);
        break;

    default:
        break;
    }

    switch (stChangeSet.dCaretKind)
    {
    case VIEW_CHANGESET_CARET_MOVE:
        ViewDrawCaret(stChangeSet.dCaretXdot, stChangeSet.dCaretLine, TRUE);
        break;

    default:
        break;
    }

    if (stChangeSet.bRenewPageInfo)
    {
        DocPageInfoRenew(-1, 1);
    }
}

static VOID ViewChangeSetRequestRedrawLine(VIEW_EDIT_CHANGESET *pstChangeSet, INT dLine)
{
    if (!(pstChangeSet))
    {
        return;
    }

    if (VIEW_CHANGESET_REDRAW_ALL == pstChangeSet->dRedrawKind)
    {
        return;
    }

    if (VIEW_CHANGESET_REDRAW_RANGE == pstChangeSet->dRedrawKind)
    {
        if (pstChangeSet->dRedrawTopLine > dLine)
        {
            pstChangeSet->dRedrawTopLine = dLine;
        }
        if (pstChangeSet->dRedrawBottomLine < dLine)
        {
            pstChangeSet->dRedrawBottomLine = dLine;
        }
        return;
    }

    if (VIEW_CHANGESET_REDRAW_LINE == pstChangeSet->dRedrawKind)
    {
        if (pstChangeSet->dRedrawTopLine == dLine)
        {
            return;
        }

        pstChangeSet->dRedrawKind = VIEW_CHANGESET_REDRAW_RANGE;
        pstChangeSet->dRedrawTopLine = min(pstChangeSet->dRedrawTopLine, dLine);
        pstChangeSet->dRedrawBottomLine = max(pstChangeSet->dRedrawBottomLine, dLine);
        return;
    }

    pstChangeSet->dRedrawKind = VIEW_CHANGESET_REDRAW_LINE;
    pstChangeSet->dRedrawTopLine = dLine;
    pstChangeSet->dRedrawBottomLine = dLine;
}

static VOID ViewChangeSetRequestRedrawRange(VIEW_EDIT_CHANGESET *pstChangeSet, INT dTopLine, INT dBottomLine)
{
    if (!(pstChangeSet))
    {
        return;
    }

    if (VIEW_CHANGESET_REDRAW_ALL == pstChangeSet->dRedrawKind)
    {
        return;
    }

    if (VIEW_CHANGESET_REDRAW_RANGE == pstChangeSet->dRedrawKind)
    {
        pstChangeSet->dRedrawTopLine = min(pstChangeSet->dRedrawTopLine, dTopLine);
        pstChangeSet->dRedrawBottomLine = max(pstChangeSet->dRedrawBottomLine, dBottomLine);
        return;
    }

    if (VIEW_CHANGESET_REDRAW_LINE == pstChangeSet->dRedrawKind)
    {
        pstChangeSet->dRedrawKind = VIEW_CHANGESET_REDRAW_RANGE;
        pstChangeSet->dRedrawTopLine = min(pstChangeSet->dRedrawTopLine, dTopLine);
        pstChangeSet->dRedrawBottomLine = max(pstChangeSet->dRedrawBottomLine, dBottomLine);
        return;
    }

    pstChangeSet->dRedrawKind = VIEW_CHANGESET_REDRAW_RANGE;
    pstChangeSet->dRedrawTopLine = dTopLine;
    pstChangeSet->dRedrawBottomLine = dBottomLine;
}

static VOID ViewChangeSetRequestRedrawAll(VIEW_EDIT_CHANGESET *pstChangeSet)
{
    if (!(pstChangeSet))
    {
        return;
    }

    pstChangeSet->dRedrawKind = VIEW_CHANGESET_REDRAW_ALL;
    pstChangeSet->dRedrawTopLine = -1;
    pstChangeSet->dRedrawBottomLine = -1;
}

static VOID ViewChangeSetRequestCaretMove(VIEW_EDIT_CHANGESET *pstChangeSet, INT dXdot, INT dLine)
{
    if (!(pstChangeSet))
    {
        return;
    }

    pstChangeSet->dCaretKind = VIEW_CHANGESET_CARET_MOVE;
    pstChangeSet->dCaretXdot = dXdot;
    pstChangeSet->dCaretLine = dLine;
}

VOID ViewEditUndoForward(VOID)
{
    auto caret = ViewCurrentCaret();

    DocPageSelStateToggle(-1);

    const INT dCrLf = DocUndoExecute(&caret.dXdot, &caret.dLine);
    if (dCrLf)
    {
        ViewRedrawSetLine(-1);
    }
    else
    {
        ViewRedrawSetLine(caret.dLine);
    }

    ViewDrawCaret(caret.dXdot, caret.dLine, TRUE);
}

VOID ViewEditRedoForward(VOID)
{
    auto caret = ViewCurrentCaret();

    DocPageSelStateToggle(-1);

    const INT dCrLf = DocRedoExecute(&caret.dXdot, &caret.dLine);
    if (dCrLf)
    {
        ViewRedrawSetLine(-1);
    }
    else
    {
        ViewRedrawSetLine(caret.dLine);
    }

    ViewDrawCaret(caret.dXdot, caret.dLine, TRUE);
}

VOID ViewEditCopySelection(UINT dSquareMode)
{
    DocExClipSelect(D_UNI | dSquareMode);
}

VOID ViewEditExecuteExtraction(HINSTANCE hInstance)
{
    DocExtractExecute(hInstance);
}

VOID ViewEditCutSelection(UINT dSquareMode)
{
    ViewEditCopySelection(dSquareMode);
    if (IsSelecting(nullptr))
    {
        ViewEditDeleteForward();
    }
}

INT ViewEditFillSelection(LPTSTR ptPattern)
{
    VIEW_EDIT_CHANGESET stChangeSet{};
    auto caret = ViewCurrentCaret();
    const INT rslt = DocSelectedBrushFilling(ptPattern, &caret.dXdot, &caret.dLine);

    if (rslt)
    {
        stChangeSet.bRenewPageInfo = TRUE;
        ViewChangeSetRequestCaretMove(&stChangeSet, caret.dXdot, caret.dLine);
        ViewChangeSetApply(stChangeSet);
    }

    return rslt;
}

static VOID ViewRedrawChangedLines(INT dLine, INT iLines, INT bCrLf)
{
    if (0 < bCrLf)
    {
        for (INT i = dLine; iLines >= i; i++)
        {
            ViewRedrawSetLine(i);
        }
        return;
    }

    ViewRedrawSetLine(dLine);
}

VOID ViewEditDeleteForward(VOID)
{
    UINT bSqSel = 0;
    const BOOLEAN bSelect = IsSelecting(&bSqSel);
    const INT iLines = DocNowFilePageLineCount();
    auto caret = ViewCurrentCaret();

    const INT bCrLf = bSelect ? DocSelectedDelete(&caret.dXdot, &caret.dLine, bSqSel, TRUE)
                              : DocInputDelete(caret.dXdot, caret.dLine);

    ViewRedrawChangedLines(caret.dLine, iLines, bCrLf);
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);
    DocPageInfoRenew(-1, 1);
}

VOID ViewEditDeleteBackward(VOID)
{
    UINT bSqSel = 0;
    const BOOLEAN bSelect = IsSelecting(&bSqSel);
    const INT iLines = DocNowFilePageLineCount();
    auto caret = ViewCurrentCaret();

    const INT bCrLf = bSelect ? DocSelectedDelete(&caret.dXdot, &caret.dLine, bSqSel, TRUE)
                              : DocInputBkSpace(&caret.dXdot, &caret.dLine);

    ViewRedrawChangedLines(caret.dLine, iLines, bCrLf);
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);
    gdXmemory = caret.dXdot;
}

HRESULT ViewEditInsertLineBreak(BOOLEAN bScripted)
{
    if (bScripted)
    {
        return ViewScriptedLineFeed();
    }

    UINT bSqSel = 0;
    const BOOLEAN bSelect = IsSelecting(&bSqSel);
    BOOLEAN bFirst = TRUE;
    auto caret = ViewCurrentCaret();

    if (bSelect)
    {
        DocSelectedDelete(&caret.dXdot, &caret.dLine, bSqSel, bFirst);
        bFirst = FALSE;
    }

    DocCrLfAdd(caret.dXdot, caret.dLine, bFirst);
    ViewRedrawSetLine(caret.dLine);

    caret.dXdot = 0;
    caret.dMozi = 0;
    caret.dLine++;
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);
    gdXmemory = caret.dXdot;

    const INT iLines = DocPageParamGet(nullptr, nullptr);
    for (INT i = caret.dLine; iLines >= i; i++)
    {
        ViewRedrawSetLine(i);
    }

    return S_OK;
}

INT ViewEditInputCharacter(TCHAR ch)
{
    UINT bSqSel = 0;
    const BOOLEAN bSelect = IsSelecting(&bSqSel);
    auto caret = ViewCurrentCaret();
    INT bCrLf = 0;
    const INT iLines = DocPageParamGet(nullptr, nullptr);

    if (bSelect)
    {
        if (bSqSel)
        {
            TCHAR atCh[2] = {ch, 0};
            ViewEditFillSelection(atCh);
            return 0;
        }

        bCrLf = DocSelectedDelete(&caret.dXdot, &caret.dLine, 0, TRUE);
    }

    const INT width = DocInsertLetter(&caret.dXdot, caret.dLine, ch);
    caret.dMozi = DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);
    gdXmemory = caret.dXdot;

    if (bCrLf)
    {
        for (INT i = caret.dLine; iLines > i; i++)
        {
            ViewRedrawSetLine(i);
        }
    }
    else
    {
        ViewRedrawSetLine(caret.dLine);
    }

    DocPageInfoRenew(-1, 1);

    return width;
}

INT ViewEditPasteFromClipboard(UINT bSquareMode)
{
    auto caret = ViewCurrentCaret();

    return DocInputFromClipboard(&caret.dXdot, &caret.dLine, &caret.dMozi, bSquareMode);
}

static INT ViewInsertLiteralCharacter(TCHAR ch)
{
    auto caret = ViewCurrentCaret();

    const INT width = DocInsertLetter(&caret.dXdot, caret.dLine, ch);
    caret.dMozi = DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);
    ViewRedrawSetLine(caret.dLine);
    DocPageInfoRenew(-1, 1);

    return width;
}

static INT ViewInsertStringWithStyle(LPCTSTR ptText, UINT dStyle, PINT pdMozi)
{
    auto caret = ViewCurrentCaret();

    const INT dCrLf = DocInsertString(&caret.dXdot, &caret.dLine, pdMozi ? pdMozi : &caret.dMozi, ptText, dStyle, TRUE);
    DocPageInfoRenew(-1, 1);

    return dCrLf;
}
//-------------------------------------------------------------------------------------------------

// CreateAcceleratorTable メモ

// シフト、コントロール、アルトキーの状態を確認する
VOID ViewCombiKeyCheck(VOID)
{
    //    SHIFT,CONTROL,ALTのキーの具合は、GetKeyStateもしくはGetKeyboardStateを使えばいい
    gbShiftOn = (0x8000 & GetKeyState(VK_SHIFT)) ? TRUE : FALSE;
    gbCtrlOn = (0x8000 & GetKeyState(VK_CONTROL)) ? TRUE : FALSE;
    gbAltOn = (0x8000 & GetKeyState(VK_MENU)) ? TRUE : FALSE;

    return;
}
//-------------------------------------------------------------------------------------------------

// 編集ビューのキーダウンが発生
VOID Evw_OnKey(HWND hWnd, UINT vk, BOOL fDown, INT cRepeat, UINT flags)
{
    INT bXdirect = 0; //    Ｘの移動方向
    UINT dXwidth;     //    Ｘの移動ドット
    INT dDot, iLines;
    BOOLEAN bJump = FALSE, bMemoryX = FALSE;
    auto caret = ViewCurrentCaret();

    ViewCombiKeyCheck();

    ViewSelPositionSet(nullptr); //    操作直前の位置
    //    中でルーラーのドローが発生してる。Ctrlとか押しっぱでちらつく

#ifdef DO_TRY_CATCH
    try
    {
#endif

        if (fDown)
        {
            switch (vk)
            {
            case VK_RIGHT:
                bXdirect = 1;
                bMemoryX = TRUE;
                break;

            case VK_LEFT:
                bXdirect = -1;
                bMemoryX = TRUE;
                break;

            case VK_DOWN: //    行を下へ
                caret.dLine++;
                dDot = DocLineParamGet(caret.dLine, nullptr, nullptr); //    次の行が無かったら死にます
                if (-1 == dDot)
                {
                    caret.dLine--;
                } //    戻しておく
                break;

            case VK_UP: //    行を上へ
                if (0 < caret.dLine)
                {
                    caret.dLine--;
                } //    ０なら変化無し
                break;

            case VK_PRIOR: //    PageUp
                caret.dLine -= 10;
                if (0 > caret.dLine)
                {
                    caret.dLine = 0;
                }
                break;

            case VK_NEXT: //    PageDown
                caret.dLine += 10;
                iLines = DocNowFilePageLineCount(); // DocPageParamGet( nullptr, nullptr );    //    行数確保
                if (iLines <= caret.dLine)
                {
                    caret.dLine = iLines - 1;
                }
                break;

            case VK_END: //    Ctrl+Endで末尾へ
                if (gbCtrlOn)
                {
                    caret.dLine = DocNowFilePageLineCount() - 1;
                } //    DocPageParamGet( nullptr, nullptr )    //    行数確保
                caret.dXdot = DocLineParamGet(caret.dLine, &caret.dMozi, nullptr);
                bMemoryX = TRUE;
                break;

            case VK_HOME: //    Ctrl+Homeで先頭へ
                caret.dXdot = 0;
                caret.dMozi = 0;
                bMemoryX = TRUE;
                if (gbCtrlOn)
                {
                    caret.dLine = 0;
                }
                break;

            case VK_DELETE: //    DELキー入力
                ViewEditDeleteForward();
                return;

            case VK_PROCESSKEY:
                //    大日本帝國的言語変換をしてるときはキー入力は全部ここにくる・シフトとかは別っぽい
                return;

            default:
                return;
            }
            //    IMEがONのときのオサレは、vk：0xE5連発・確定された文字はCHARへ

            if (bMemoryX)
                gdXmemory = caret.dXdot;
            else
                caret.dXdot = gdXmemory;

            DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0); //    キャレット位置調整
            //    ここで文字位置のインクリ・デクリの面倒みて、ドットと行位置を変更
            dXwidth = DocLetterShiftPos(caret.dXdot, caret.dLine, bXdirect, nullptr, &bJump);

            if (0 > bXdirect) //    左・先頭へ向かって
            {
                caret.dXdot -= dXwidth;
                if (0 > caret.dXdot)
                    caret.dXdot = 0;

                if (bJump)
                {
                    if (0 < caret.dLine)
                    {
                        caret.dLine--;
                    } //    ０なら変化無し

                    //    隣の行の末尾に移動する
                    dDot = DocLineParamGet(caret.dLine, nullptr, nullptr);
                    caret.dXdot = dDot;
                }
            }

            if (0 < bXdirect) //    右・末尾へ向かって
            {
                caret.dXdot += dXwidth;
                if (bJump)
                {
                    caret.dLine++;

                    //    次の行が無かったら
                    dDot = DocLineParamGet(caret.dLine, nullptr, nullptr);
                    if (0 > dDot)
                    {
                        caret.dLine--;
                    } //    戻しておく
                    else
                    {
                        caret.dXdot = 0;
                    } //    次の行へ移動して行頭へ
                }
            }

            caret.dMozi = DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0); //    今の文字位置を確認

            ViewSelMoveCheck(FALSE); //    位置決まったら、選択状態をCheck

            if (bMemoryX)
                gdXmemory = caret.dXdot;

            ViewDrawCaret(caret.dXdot, caret.dLine, 1); //    最終的な位置にキャレット位置を変更
        }
        else //    キー離され
        {
            //    ない？
        }

#ifdef DO_TRY_CATCH
    }
    catch (exception &err)
    {
        ETC_MSG(err.what(), 0);
        return;
    }
    catch (...)
    {
        ETC_MSG(("etc error"), 0);
        return;
    }
#endif

    return;
}
//-------------------------------------------------------------------------------------------------

// 編集ビューの文字キーオサレが発生
VOID Evw_OnChar(HWND hWnd, TCHAR ch, INT cRepeat)
{
    const INT isctrl = iswcntrl(ch);

    ViewCombiKeyCheck();
    //    Ctrl+Zとかは制御文字で来るので注意
    if (isctrl) //    制御文字はBSとReturn以外は無視でいいか
    {
        TRACE(TEXT("制御文字[%04X]"), ch);

        //    TABはこっちに来る

        if (VK_RETURN == ch) //    改行
        {
            TRACE(TEXT("Enter Shift[%d]"), gbShiftOn);
            ViewEditInsertLineBreak(gbShiftOn);
        }

        if (VK_BACK == ch) //    BackSpace
        {
            TRACE(TEXT("BACKSP"));
            ViewEditDeleteBackward();
        }

        return;
    }

    TRACE(TEXT("入力文字[%c]"), ch);

    if (0 < gdSqFillCnt) //    IME経由で矩形塗り潰しが発生している
    {
        gdSqFillCnt--;
        TRACE(TEXT("キャンセル[%u]"), gdSqFillCnt);
        return;
    }

    ViewEditInputCharacter(ch);

    return;
}
//-------------------------------------------------------------------------------------------------

// ビューでマウスの左ボタンがダウンされたとき
VOID Evw_OnLButtonDown(HWND hWnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags)
{
    INT dX, dY;                         //
    INT dDot, dMaxDot, dLine, iMaxLine; //
    auto caret = ViewCurrentCaret();

    SetFocus(hWnd); //    マウスインでフォーカス

    dX = x;
    dY = y;
    ViewPositionTransform(&dX, &dY, 0); //    ここで、ドキュメント位置に変更

    ViewCombiKeyCheck();

    //    マイナスのときはルーラーか行番号エリア
    if (0 > dX)
        dX = 0;
    if (0 > dY)
        dY = 0;

    dDot = dX;
    dLine = dY / LINE_HEIGHT;

    if (fDoubleClick) //    ダブルクルック
    {
        TRACE(TEXT("マウス左ダブルクルック[%d / %d]%d:%d:%d"), dDot, dLine, gbShiftOn, gbCtrlOn, gbAltOn);

        ViewSelAreaSelect(nullptr);

        gbLDoubleClick = TRUE;
        return; //    これ以降は処理しなくて良いはず
    }

    TRACE(TEXT("マウス左ダウン[%d / %d]%d:%d:%d"), dDot, dLine, gbShiftOn, gbCtrlOn, gbAltOn);

    SetCapture(hWnd); //    マウスキャプチャ

    //    有効な位置かどうか確認

    //    行数確認して、はみ出してたら末端にしておく
    iMaxLine = DocNowFilePageLineCount(); // DocPageParamGet( nullptr, nullptr );    //    行数確認かも・・・
    if (iMaxLine <= dLine)
        dLine = iMaxLine - 1;

    //    その行のドット数を確認して、はみ出してたら末端にしておく
    dMaxDot = DocLineParamGet(dLine, nullptr, nullptr);
    if (dMaxDot <= dDot)
        dDot = dMaxDot;

    //    文字位置に合わせて調整
    caret.dMozi = DocLetterPosGetAdjust(&dDot, dLine, 0); //    今の文字位置を確認
    caret.dXdot = dDot;
    caret.dLine = dLine;
    //    この時点で移動は確定

    gstLClicken.x = caret.dXdot;
    gstLClicken.y = caret.dLine;

    gdXmemory = caret.dXdot;
    ViewDrawCaret(caret.dXdot, caret.dLine, 1); //    位置移動

    ViewSelMoveCheck(FALSE); //    選択状態の上でもドラッグ移動は行わず通常のクリックとして扱う

    ViewSelPositionSet(nullptr); //    移動した位置を記録と再描画

    return;
}
//-------------------------------------------------------------------------------------------------

// ビューでマウスを動かしたとき
VOID Evw_OnMouseMove(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    TCHAR atString[SUB_STRING];
    INT dX, dY;
    INT dDot, dMaxDot, dLine, iMaxLine; //
    auto caret = ViewCurrentCaret();

    //    ダブルクルック操作後は何もしない
    if (gbLDoubleClick)
    {
        return;
    }

    dX = x;
    dY = y;
    ViewPositionTransform(&dX, &dY, 0); //    ここで、ドキュメント位置に変更

    ViewCombiKeyCheck(); //    範囲選択しようとしてるか

    //    マイナスのときはルーラーか行番号エリア
    if (0 > dY)
        dY = 0;

    dLine = dY / LINE_HEIGHT;

    if (0 > dX)
    {
        dX = 0;
        if ((keyFlags & MK_LBUTTON)) //    ドラッグ中であるなら
        {
            //    その行のドット数を確認して、常に末端にカーソルがあると仮定する
            dX = DocLineParamGet(dLine, nullptr, nullptr);
            //    バック選択中なら、逆に先頭にカーソルが来るようにする
            if (ViewSelBackCheck(dLine))
            {
                dX = 0;
            }
        }
    }

    dDot = dX;

    gstCursor.x = dDot;
    gstCursor.y = dLine; //    Cursor位置記憶

    if ((keyFlags & MK_LBUTTON))
    {
        //    行数確認して、はみ出してたら末端にしておく
        iMaxLine = DocNowFilePageLineCount(); // DocPageParamGet( nullptr, nullptr );
        if (iMaxLine <= dLine)
        {
            dLine = iMaxLine - 1;
        }

        //    その行のドット数を確認して、はみ出してたら末端にしておく
        dMaxDot = DocLineParamGet(dLine, nullptr, nullptr);
        if (dMaxDot <= dDot)
            dDot = dMaxDot;

        //    文字位置に合わせて調整
        caret.dMozi = DocLetterPosGetAdjust(&dDot, dLine, 0); //    今の文字位置を確認
        caret.dXdot = dDot;
        caret.dLine = dLine;

        ViewDrawCaret(caret.dXdot, caret.dLine, 1); //    位置移動

        ViewSelMoveCheck(TRUE);

        ViewSelPositionSet(nullptr); //    移動した位置を記録
    }

    StringCchPrintf(atString, SUB_STRING, TEXT("마우스 %d번째 줄 %d도트"), gstCursor.y, gstCursor.x);
    MainStatusBarSetText(SB_MOUSEPOS, atString);

    return;
}
//-------------------------------------------------------------------------------------------------

// ビューでマウスの左ボタンがうっｐされたとき
VOID Evw_OnLButtonUp(HWND hWnd, INT x, INT y, UINT keyFlags)
{
    auto caret = ViewCurrentCaret();

    TRACE(TEXT("マウス左アップ[%d / %d]"), x, y);

    //    ダブルクルック操作後はすることはない
    if (gbLDoubleClick)
    {
        gbLDoubleClick = FALSE;
        return;
    }

    ViewSelRangeCheck(FALSE); //    とりあえず選択範囲の様子確認

    ReleaseCapture(); //    マウスキャプチャ解除

    //    Downのほうから    20120328
    if (!(gbExtract)) //    抽出モード中は処理しない
    {
        ViewBrushFilling(); //    ブラシする

        //    最終的にここで解除
        if ((gstLClicken.x == caret.dXdot) && (gstLClicken.y == caret.dLine))
        {
            ViewSelMoveCheck(FALSE);
        }
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// ビューでマウスの右ボタンがダウンされたとき・コンテキストメニューの前
VOID Evw_OnRButtonDown(HWND hWnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags)
{
    //    キャレット移動のみ面倒見る
    //    20110704    選択中はキャレット移動しないようにする

    INT dX, dY;                         //
    INT dDot, dMaxDot, dLine, iMaxLine; //
    auto caret = ViewCurrentCaret();

    SetFocus(hWnd); //    マウスインでフォーカス

    if (IsSelecting(nullptr)) //    選択作業中であるならなにもしない
    {
        TRACE(TEXT("[%X]マウス右ダウン　%d:%d　選択中"), hWnd, x, y);
        return;
    }

    dX = x;
    dY = y;
    ViewPositionTransform(&dX, &dY, 0); //    ここで、ドキュメント位置に変更

    dDot = dX;
    dLine = dY / LINE_HEIGHT;

    TRACE(TEXT("[%X]マウス右ダウン[%d:%d[%d] / %d:%d:%d]"), hWnd, dX, dY, dLine, gbShiftOn, gbCtrlOn, gbAltOn);

    if (0 <= dX || 0 <= dY) //    マイナスのときはルーラーか行番号エリア
    {
        //    函数にしておくか

        //    有効な位置かどうか確認

        //    行数確認して、はみ出してたら末端にしておく
        iMaxLine = DocNowFilePageLineCount(); // DocPageParamGet( nullptr, nullptr );
        if (iMaxLine <= dLine)
            dLine = iMaxLine - 1;

        //    その行のドット数を確認して、はみ出してたら末端にしておく
        dMaxDot = DocLineParamGet(dLine, nullptr, nullptr);
        if (dMaxDot <= dDot)
            dDot = dMaxDot;

        //    文字位置に合わせて調整
        caret.dMozi = DocLetterPosGetAdjust(&dDot, dLine, 0); //    今の文字位置を確認
        caret.dXdot = dDot;
        caret.dLine = dLine;

        ViewDrawCaret(caret.dXdot, caret.dLine, 1); //    位置移動

        ViewSelMoveCheck(FALSE);

        ViewSelPositionSet(nullptr); //    移動した位置を記録
    }

    return;
}
//-------------------------------------------------------------------------------------------------

// ホイール大回転
VOID Evw_OnMouseWheel(HWND hWnd, INT xPos, INT yPos, INT zDelta, UINT fwKeys)
{
    UINT dCode;

    HWND hChdWnd;
    POINT stPoint;

    TRACE(TEXT("POS[%d x %d] DELTA[%d] K[%X]"), xPos, yPos, zDelta, fwKeys);
    //    fwKeys    SHIFT 0x4, CTRL 0x8

    stPoint.x = xPos;
    stPoint.y = yPos;
    ScreenToClient(ghPrntWnd, &stPoint);
    hChdWnd = ChildWindowFromPointEx(ghPrntWnd, stPoint, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED | CWP_SKIPTRANSPARENT);

    if (ghViewWnd != hChdWnd)
        return;

    if (0 < zDelta)
        dCode = SB_LINEUP;
    else if (0 > zDelta)
        dCode = SB_LINEDOWN;
    else
        dCode = SB_ENDSCROLL;

    //    posを、ホイールフラグにすればいい
    FORWARD_WM_VSCROLL(ghViewWnd, nullptr, dCode, 1, PostMessage);

    return;
}
//-------------------------------------------------------------------------------------------------

// ユニコード空白の挿入
INT ViewInsertUniSpace(UINT dCommando)
{
    INT width;
    TCHAR ch;

    TRACE(TEXT("挿入：ユニコード空白"));

    switch (dCommando)
    {
    case IDM_IN_01SPACE:
        ch = (TCHAR)0x200A;
        break; //    8202
    case IDM_IN_02SPACE:
        ch = (TCHAR)0x2009;
        break; //    8201
    case IDM_IN_03SPACE:
        ch = (TCHAR)0x2006;
        break; //    8198
    case IDM_IN_04SPACE:
        ch = (TCHAR)0x2005;
        break; //    8197
    case IDM_IN_05SPACE:
        ch = (TCHAR)0x2004;
        break; //    8196
    case IDM_IN_08SPACE:
        ch = (TCHAR)0x2002;
        break; //    8194
    case IDM_IN_10SPACE:
        ch = (TCHAR)0x2007;
        break; //    8199
    case IDM_IN_16SPACE:
        ch = (TCHAR)0x2003;
        break; //    8195
    default:
        return 0;
    }

    width = ViewInsertLiteralCharacter(ch);

    return width;
}
//-------------------------------------------------------------------------------------------------

// 台詞用改行の処理する
HRESULT ViewScriptedLineFeed(VOID)
{
    VIEW_EDIT_CHANGESET stChangeSet{};
    INT iTgtDot, iLastDot;
    INT iPrvDot, iChkDot;
    BOOLEAN bIsSp, bJump;
    auto caret = ViewCurrentCaret();

    iChkDot = caret.dXdot;
    iTgtDot = 0; //    安全確認

    while (iChkDot)
    {
        //    文字列の開始地点を探す。iTgtDotがその位置のはず
        DocLineStateCheckWithDot(iChkDot, caret.dLine, &iTgtDot, &iLastDot, nullptr, nullptr, &bIsSp);

        //    チェック部分が空白ではなく、先頭までイッてしまったら
        if (0 == iTgtDot && !(bIsSp))
        {
            break;
        } //    すなわち先頭部分まで移動

        //    該当箇所が最初からスペースだったら、キャレット位置を基準点にする・壱行空け用
        if (bIsSp)
        {
            iTgtDot = iChkDot;
            break;
        }

        DocLetterShiftPos(iTgtDot, caret.dLine, -1, &iPrvDot, &bJump); //    壱文字戻る
        //    戻ったところも空白かどうかチェック
        DocLineStateCheckWithDot(iPrvDot, caret.dLine, &iChkDot, &iLastDot, nullptr, nullptr, &bIsSp);
        if (bIsSp)
        {
            break;
        } //    引き続き空白なら、そこまでとする。

        iChkDot = iPrvDot; //    一つ戻った所からチェック続行
    }

    TRACE(TEXT("TEXT START D[%d] L[%d]"), iTgtDot, caret.dLine);

    const HRESULT hr = ViewScriptedLineFeedAtDot(iTgtDot, &stChangeSet);
    if (SUCCEEDED(hr))
    {
        ViewChangeSetApply(stChangeSet);
    }

    return hr;
}
//-------------------------------------------------------------------------------------------------

// テンプレからの文字列を挿入
INT ViewInsertTmpleString(LPCTSTR ptText)
{
    INT dDot;
    auto caret = ViewCurrentCaret();

    dDot = caret.dXdot;

    ViewInsertStringWithStyle(ptText, 0, &caret.dMozi);

    dDot = ViewCurrentCaret().dXdot - dDot;

    return dDot;
}
//-------------------------------------------------------------------------------------------------

// ブラシモードの設定をうける
HRESULT ViewBrushStyleSetting(UINT bBrushOn, LPCTSTR ptPattern)
{
    gbPalBucketMode = bBrushOn;

    //    メニューのcheckTOGGLE
    SendMessage(ghPrntWnd, WMP_BRUSH_TOGGLE, (WPARAM)bBrushOn, (LPARAM)IDM_BRUSH_STYLE);

    if (ptPattern)
    {
        StringCchCopy(gatBrushPtn, SUB_STRING, ptPattern);
    }

    OperationOnStatusBar();

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ブラシモードなら、ブラシ的処理をする
HRESULT ViewBrushFilling(VOID)
{
    VIEW_EDIT_CHANGESET stChangeSet{};
    auto caret = ViewCurrentCaret();

    if (!(gbPalBucketMode))
        return S_FALSE;

    //    選択範囲があるなら、そっちを優先して塗りつぶす
    if (ViewEditFillSelection(gatBrushPtn))
        return S_OK;

    const HRESULT hr = ViewBrushFillAtCaret(&stChangeSet);
    if (SUCCEEDED(hr))
    {
        ViewChangeSetApply(stChangeSet);
    }

    return hr;
}

static HRESULT ViewBrushFillAtCaret(VIEW_EDIT_CHANGESET *pstChangeSet)
{
    INT dTgDot;
    INT dLeft, dRight, iBgnMozi, iCntMozi;
    BOOLEAN bSpace, bFirst = TRUE;
    LPTSTR ptBuff;
    auto caret = ViewCurrentCaret();

    //    クルッコ位置が空白かどうか
    dTgDot = DocLineStateCheckWithDot(caret.dXdot, caret.dLine, &dLeft, &dRight, &iBgnMozi, &iCntMozi, &bSpace);
    if (!(bSpace))
        return S_FALSE;

    // 塗り潰し文字列作成
    ptBuff = BrushStringMake(dTgDot, gatBrushPtn);
    if (!(ptBuff))
    {
        NotifyBalloonExist(TEXT("ブラシを選んでおいてね"), TEXT("操作ミス"), NIIF_INFO);
        return E_OUTOFMEMORY;
    }

    //    その範囲の空白を削除して
    DocRangeDeleteByMozi(dLeft, caret.dLine, iBgnMozi, (iBgnMozi + iCntMozi), &bFirst);
    //    ブラシ文字列をぶち込む
    DocInsertString(&dLeft, &caret.dLine, nullptr, ptBuff, 0, bFirst);
    bFirst = FALSE;

    //    再度調整・キャレット移動はこの後
    caret.dMozi = DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);

    ViewChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    ViewChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);
    pstChangeSet->bRenewPageInfo = TRUE;

    FREE(ptBuff);

    return S_OK;
}

static HRESULT ViewScriptedLineFeedAtDot(INT dTargetDot, VIEW_EDIT_CHANGESET *pstChangeSet)
{
    INT dLines;
    INT iLineDot;
    INT iPadDot;
    BOOLEAN bFirst = TRUE;
    LPTSTR ptSpace;
    auto caret = ViewCurrentCaret();

    dLines = DocNowFilePageLineCount();
    if ((dLines - 1) <= caret.dLine)
    {
        DocAdditionalLine(1, &bFirst);
        ViewChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    }

    caret.dLine++;

    iLineDot = DocLineParamGet(caret.dLine, nullptr, nullptr);
    if (dTargetDot <= iLineDot)
    {
        caret.dXdot = dTargetDot;
        DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);
        ViewChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
        ViewChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);
        return S_OK;
    }

    iPadDot = dTargetDot - iLineDot;
    ptSpace = DocPaddingSpaceMake(iPadDot);
    if (!(ptSpace))
    {
        return E_OUTOFMEMORY;
    }
    caret.dXdot = iLineDot;
    DocInsertString(&caret.dXdot, &caret.dLine, nullptr, ptSpace, 0, bFirst);
    FREE(ptSpace);

    ViewChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    ViewChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 指定한 폭 안에서만 브러시 문자열을 만들고 남는 폭은 공백으로 유지한다
LPTSTR BrushStringMake(INT dDotLen, LPTSTR ptPattern)
{
    INT dPtnDot, dCnt, dAmr, i, wid;
    UINT_PTR cchSize;
    LPTSTR ptBuff, ptPadd = nullptr;
    wstring wsBuff;

    // 塗り潰し文字列作成
    wsBuff.clear();

    //    ブラシの幅確認して
    dPtnDot = ViewStringWidthGet(ptPattern);
    if (0 >= dPtnDot || 0 >= dDotLen)
    {
        return nullptr;
    }
    //    なんかいろいろおかしかったら終わる

    dCnt = dDotLen / dPtnDot;          //    必要な数を確認
    dAmr = dDotLen - (dCnt * dPtnDot); //    余り

    //    規定の文字列作成
    for (i = 0; dCnt > i; i++)
    {
        wsBuff += ptPattern;
    }

    //    あまりを出来る限り埋める
    i = 0;
    while (0 < dAmr) //    余り幅が無くなったら終わり
    {
        if (0 == ptPattern[i])
            break; //    それ以上パヤーンがなかったら

        wid = ViewLetterWidthGet(ptPattern[i]); //    順番に見ていく
        if (wid > dAmr)
            break;
        wsBuff += ptPattern[i];
        dAmr -= wid; //    壱文字毎の幅で埋めていく
        i++;
    }

    if (0 < dAmr)
    {
        ptPadd = DocPaddingSpaceMake(dAmr);
        if (ptPadd)
        {
            wsBuff += ptPadd;
            FREE(ptPadd);
        }
    }

    cchSize = wsBuff.size() + 8; //    とりあえず余裕しておく
    ptBuff = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
    ZeroMemory(ptBuff, cchSize * sizeof(TCHAR));
    StringCchCopy(ptBuff, cchSize, wsBuff.c_str());

    return ptBuff;
}
//-------------------------------------------------------------------------------------------------

// WM_IME_COMPOSITIONメッセージ
VOID Evw_OnImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HIMC hImc;
    LONG cbSize;
    LPTSTR ptBuffer;
    UINT_PTR cchSize;

    BOOLEAN bSelect = FALSE;
    UINT bSqSel = 0;
    auto caret = ViewCurrentCaret();

    TRACE(TEXT("WM_IME_COMPOSITION[0x%X][0x%X]"), wParam, lParam);

    bSelect = IsSelecting(&bSqSel);
    gdSqFillCnt = 0;

    if ((GCS_RESULTSTR & lParam) && bSelect && bSqSel)
    {
        hImc = ImmGetContext(ghViewWnd); //    IMEハンドル確保

        cbSize = ImmGetCompositionString(hImc, GCS_RESULTSTR, nullptr, 0);
        //    得られるのはバイト数・文字数じゃない

        //    確定した文字列を確保
        cbSize += 2;
        ptBuffer = (LPTSTR)malloc(cbSize);
        ZeroMemory(ptBuffer, cbSize);
        ImmGetCompositionString(hImc, GCS_RESULTSTR, ptBuffer, cbSize);
        TRACE(TEXT("COMPOSITION MOZI[%d][%s]"), cbSize, ptBuffer);
        ImmReleaseContext(ghViewWnd, hImc);

        //    塗り潰し処理
        ViewEditFillSelection(ptBuffer);

        StringCchLength(ptBuffer, cbSize, &cchSize);
        gdSqFillCnt = cchSize;

        FREE(ptBuffer);
    }
    
    return;
}

BOOLEAN ViewLayoutCommandForward(INT id, HWND hMainWindow)
{
    auto caret = ViewCurrentCaret();
    const UINT dSquareSelect = ViewSquareSelectModeGet();

    UNREFERENCED_PARAMETER(hMainWindow);

    switch (id)
    {
    case IDM_RIGHT_GUIDE_SET:
        DocRightGuideline(nullptr);
        return TRUE;

    case IDM_DEL_LASTSPACE:
        DocLastSpaceErase(&caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_INS_TOPSPACE:
        DocTopLetterInsert(TEXT('　'), &caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_DEL_TOPSPACE:
        DocTopSpaceErase(&caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_DEL_LASTLETTER:
        DocLastLetterErase(&caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_INCREMENT_DOT:
        DocSpaceShiftProc(VK_RIGHT, &caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_DECREMENT_DOT:
        DocSpaceShiftProc(VK_LEFT, &caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_INCR_DOT_LINES:
        DocPositionShift(VK_RIGHT, &caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_DECR_DOT_LINES:
        DocPositionShift(VK_LEFT, &caret.dXdot, caret.dLine);
        return TRUE;

#ifdef DOT_SPLIT_MODE
    case IDM_DOT_SPLIT_RIGHT:
        DocCentreWidthShift(VK_RIGHT, &caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_DOT_SPLIT_LEFT:
        DocCentreWidthShift(VK_LEFT, &caret.dXdot, caret.dLine);
        return TRUE;
#else
    case IDM_DOT_SPLIT_RIGHT:
    case IDM_DOT_SPLIT_LEFT:
        MessageBox(hMainWindow, TEXT("まだ出来てないよ"), TEXT("Coming Soon ! !"), MB_OK);
        return TRUE;
#endif

    case IDM_DOTDIFF_LOCK:
        gdAutoDiffBase = DocDiffAdjBaseSet(caret.dLine);
        ViewRulerRedraw(-1, -1);
        return TRUE;

    case IDM_DOTDIFF_ADJT:
        DocDiffAdjExec(&caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_HEADHALF_EXCHANGE:
        DocHeadHalfSpaceExchange(hMainWindow);
        return TRUE;

    case IDM_FILL_ZENSP:
        DocScreenFill(TEXT("　"));
        return TRUE;

    case IDM_MIRROR_INVERSE:
        DocInverseTransform(dSquareSelect, 1, &caret.dXdot, caret.dLine);
        return TRUE;

    case IDM_UPSET_INVERSE:
        DocInverseTransform(dSquareSelect, 0, &caret.dXdot, caret.dLine);
        return TRUE;

    default:
        return FALSE;
    }
}
//-------------------------------------------------------------------------------------------------
