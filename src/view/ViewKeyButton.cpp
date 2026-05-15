#include "EditorController.h"
#include "ViewCentralInternal.h"
#include "Palette.h"

extern UINT gbUniPad; // パディングにユニコードをつかって、ドットを見せないようにする

BOOLEAN gbShiftOn; // シフトが押されている
BOOLEAN gbCtrlOn;  // コントロールが押されている
BOOLEAN gbAltOn;   // アルタが押されている

POINT gstCursor; // 文字を考慮しない、Cursorのドット＆行位置

static UINT gdSqFillCnt; // 矩形選択を、IME文字列で塗りつぶした時の文字数

static UINT gbLDoubleClick; // ダブルクルックした

static POINT gstLClicken; // 左クルックした位置

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
        //    TABはこっちに来る

        if (VK_RETURN == ch) //    改行
        {
            ViewEditInsertLineBreak(gbShiftOn);
        }

        if (VK_BACK == ch) //    BackSpace
        {
            ViewEditDeleteBackward();
        }

        return;
    }

    if (0 < gdSqFillCnt) //    IME経由で矩形塗り潰しが発生している
    {
        gdSqFillCnt--;
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
        ViewSelAreaSelect(nullptr);

        gbLDoubleClick = TRUE;
        return; //    これ以降は処理しなくて良いはず
    }

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
        EditorCtrlBrushFilling(); //    ブラシする

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
        return;
    }

    dX = x;
    dY = y;
    ViewPositionTransform(&dX, &dY, 0); //    ここで、ドキュメント位置に変更

    dDot = dX;
    dLine = dY / LINE_HEIGHT;

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
        ImmReleaseContext(ghViewWnd, hImc);

        //    塗り潰し処理
        ViewEditFillSelection(ptBuffer);

        StringCchLength(ptBuffer, cbSize, &cchSize);
        gdSqFillCnt = cchSize;

        FREE(ptBuffer);
    }
    
    return;
}
//-------------------------------------------------------------------------------------------------
