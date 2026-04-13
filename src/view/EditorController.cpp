// 편집 컨트롤러 — Doc 쓰기 함수 조합과 View 갱신을 한곳으로 모은다.
// ViewKeyButton.cpp 에서 추출하여, View ↔ Doc 의 직접 결합을 제거한다.

#include "EditorController.h"
#include "ViewCentralInternal.h"
#include "Palette.h"

// ────── 내부 상태 ──────

UINT gbPalBucketMode; // 브러시 모드 플래그 (비영이면 브러시 활성)
static TCHAR gatBrushPtn[SUB_STRING]; // 브러시 패턴 문자열

// ────── 내부 도우미 선언 ──────

static VOID CtrlRedrawChangedLines(INT dLine, INT iLines, INT bCrLf);
static INT CtrlInsertLiteralCharacter(TCHAR ch);
static INT CtrlInsertStringWithStyle(LPCTSTR ptText, UINT dStyle, PINT pdMozi);
static HRESULT CtrlBrushFillAtCaret(EDIT_CHANGESET *pstChangeSet);
static HRESULT CtrlScriptedLineFeed(VOID);
static HRESULT CtrlScriptedLineFeedAtDot(INT dTargetDot, EDIT_CHANGESET *pstChangeSet);

// ────── ChangeSet 인프라 ──────

VOID EditChangeSetApply(const EDIT_CHANGESET &stChangeSet)
{
    switch (stChangeSet.dRedrawKind)
    {
    case EDIT_CHANGESET_REDRAW_ALL:
        ViewRedrawSetLine(-1);
        break;

    case EDIT_CHANGESET_REDRAW_RANGE:
        for (INT iLine = stChangeSet.dRedrawTopLine; stChangeSet.dRedrawBottomLine >= iLine; ++iLine)
        {
            ViewRedrawSetLine(iLine);
        }
        break;

    case EDIT_CHANGESET_REDRAW_LINE:
        ViewRedrawSetLine(stChangeSet.dRedrawTopLine);
        break;

    default:
        break;
    }

    switch (stChangeSet.dCaretKind)
    {
    case EDIT_CHANGESET_CARET_MOVE:
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

VOID EditChangeSetRequestRedrawLine(EDIT_CHANGESET *pstChangeSet, INT dLine)
{
    if (!(pstChangeSet))
    {
        return;
    }

    if (EDIT_CHANGESET_REDRAW_ALL == pstChangeSet->dRedrawKind)
    {
        return;
    }

    if (EDIT_CHANGESET_REDRAW_RANGE == pstChangeSet->dRedrawKind)
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

    if (EDIT_CHANGESET_REDRAW_LINE == pstChangeSet->dRedrawKind)
    {
        if (pstChangeSet->dRedrawTopLine == dLine)
        {
            return;
        }

        pstChangeSet->dRedrawKind = EDIT_CHANGESET_REDRAW_RANGE;
        pstChangeSet->dRedrawTopLine = min(pstChangeSet->dRedrawTopLine, dLine);
        pstChangeSet->dRedrawBottomLine = max(pstChangeSet->dRedrawBottomLine, dLine);
        return;
    }

    pstChangeSet->dRedrawKind = EDIT_CHANGESET_REDRAW_LINE;
    pstChangeSet->dRedrawTopLine = dLine;
    pstChangeSet->dRedrawBottomLine = dLine;
}

VOID EditChangeSetRequestRedrawRange(EDIT_CHANGESET *pstChangeSet, INT dTopLine, INT dBottomLine)
{
    if (!(pstChangeSet))
    {
        return;
    }

    if (EDIT_CHANGESET_REDRAW_ALL == pstChangeSet->dRedrawKind)
    {
        return;
    }

    if (EDIT_CHANGESET_REDRAW_RANGE == pstChangeSet->dRedrawKind)
    {
        pstChangeSet->dRedrawTopLine = min(pstChangeSet->dRedrawTopLine, dTopLine);
        pstChangeSet->dRedrawBottomLine = max(pstChangeSet->dRedrawBottomLine, dBottomLine);
        return;
    }

    if (EDIT_CHANGESET_REDRAW_LINE == pstChangeSet->dRedrawKind)
    {
        pstChangeSet->dRedrawKind = EDIT_CHANGESET_REDRAW_RANGE;
        pstChangeSet->dRedrawTopLine = min(pstChangeSet->dRedrawTopLine, dTopLine);
        pstChangeSet->dRedrawBottomLine = max(pstChangeSet->dRedrawBottomLine, dBottomLine);
        return;
    }

    pstChangeSet->dRedrawKind = EDIT_CHANGESET_REDRAW_RANGE;
    pstChangeSet->dRedrawTopLine = dTopLine;
    pstChangeSet->dRedrawBottomLine = dBottomLine;
}

VOID EditChangeSetRequestRedrawAll(EDIT_CHANGESET *pstChangeSet)
{
    if (!(pstChangeSet))
    {
        return;
    }

    pstChangeSet->dRedrawKind = EDIT_CHANGESET_REDRAW_ALL;
    pstChangeSet->dRedrawTopLine = -1;
    pstChangeSet->dRedrawBottomLine = -1;
}

VOID EditChangeSetRequestCaretMove(EDIT_CHANGESET *pstChangeSet, INT dXdot, INT dLine)
{
    if (!(pstChangeSet))
    {
        return;
    }

    pstChangeSet->dCaretKind = EDIT_CHANGESET_CARET_MOVE;
    pstChangeSet->dCaretXdot = dXdot;
    pstChangeSet->dCaretLine = dLine;
}

// ────── 내부 도우미 ──────

static VOID CtrlRedrawChangedLines(INT dLine, INT iLines, INT bCrLf)
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

static INT CtrlInsertLiteralCharacter(TCHAR ch)
{
    auto caret = ViewCurrentCaret();

    const INT width = DocInsertLetter(&caret.dXdot, caret.dLine, ch);
    caret.dMozi = DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);
    ViewRedrawSetLine(caret.dLine);
    DocPageInfoRenew(-1, 1);

    return width;
}

static INT CtrlInsertStringWithStyle(LPCTSTR ptText, UINT dStyle, PINT pdMozi)
{
    auto caret = ViewCurrentCaret();

    const INT dCrLf = DocInsertString(&caret.dXdot, &caret.dLine, pdMozi ? pdMozi : &caret.dMozi, ptText, dStyle, TRUE);
    DocPageInfoRenew(-1, 1);

    return dCrLf;
}

// ────── 편집 명령 ──────

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
    EDIT_CHANGESET stChangeSet{};
    auto caret = ViewCurrentCaret();
    const INT rslt = DocSelectedBrushFilling(ptPattern, &caret.dXdot, &caret.dLine);

    if (rslt)
    {
        stChangeSet.bRenewPageInfo = TRUE;
        EditChangeSetRequestCaretMove(&stChangeSet, caret.dXdot, caret.dLine);
        EditChangeSetApply(stChangeSet);
    }

    return rslt;
}

VOID ViewEditDeleteForward(VOID)
{
    UINT bSqSel = 0;
    const BOOLEAN bSelect = IsSelecting(&bSqSel);
    const INT iLines = DocNowFilePageLineCount();
    auto caret = ViewCurrentCaret();

    const INT bCrLf = bSelect ? DocSelectedDelete(&caret.dXdot, &caret.dLine, bSqSel, TRUE)
                              : DocInputDelete(caret.dXdot, caret.dLine);

    CtrlRedrawChangedLines(caret.dLine, iLines, bCrLf);
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

    CtrlRedrawChangedLines(caret.dLine, iLines, bCrLf);
    ViewDrawCaret(caret.dXdot, caret.dLine, 1);
    gdXmemory = caret.dXdot;
}

HRESULT ViewEditInsertLineBreak(BOOLEAN bScripted)
{
    if (bScripted)
    {
        return CtrlScriptedLineFeed();
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

HRESULT ViewEditCopyPageAll(VOID)
{
    return DocPageAllCopy(D_UNI | D_ENTITY);
}

// ────── 삽입 명령 ──────

HRESULT ViewFrameInsert(INT dMode)
{
    return DocFrameInsert(dMode, ViewSquareSelectModeGet());
}

HRESULT ViewInsertSpoTag(VOID)
{
    return DocFrameInsert(-1, ViewSquareSelectModeGet());
}

// ────── 삽입 보조 ──────

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

    width = CtrlInsertLiteralCharacter(ch);

    return width;
}

INT ViewInsertTmpleString(LPCTSTR ptText)
{
    INT dDot;
    auto caret = ViewCurrentCaret();

    dDot = caret.dXdot;

    CtrlInsertStringWithStyle(ptText, 0, &caret.dMozi);

    dDot = ViewCurrentCaret().dXdot - dDot;

    return dDot;
}

// ────── 대사용 개행 ──────

static HRESULT CtrlScriptedLineFeed(VOID)
{
    EDIT_CHANGESET stChangeSet{};
    INT iTgtDot, iLastDot;
    INT iPrvDot, iChkDot;
    BOOLEAN bIsSp, bJump;
    auto caret = ViewCurrentCaret();

    iChkDot = caret.dXdot;
    iTgtDot = 0;

    while (iChkDot)
    {
        DocLineStateCheckWithDot(iChkDot, caret.dLine, &iTgtDot, &iLastDot, nullptr, nullptr, &bIsSp);

        if (0 == iTgtDot && !(bIsSp))
        {
            break;
        }

        if (bIsSp)
        {
            iTgtDot = iChkDot;
            break;
        }

        DocLetterShiftPos(iTgtDot, caret.dLine, -1, &iPrvDot, &bJump);
        DocLineStateCheckWithDot(iPrvDot, caret.dLine, &iChkDot, &iLastDot, nullptr, nullptr, &bIsSp);
        if (bIsSp)
        {
            break;
        }

        iChkDot = iPrvDot;
    }

    TRACE(TEXT("TEXT START D[%d] L[%d]"), iTgtDot, caret.dLine);

    const HRESULT hr = CtrlScriptedLineFeedAtDot(iTgtDot, &stChangeSet);
    if (SUCCEEDED(hr))
    {
        EditChangeSetApply(stChangeSet);
    }

    return hr;
}

static HRESULT CtrlScriptedLineFeedAtDot(INT dTargetDot, EDIT_CHANGESET *pstChangeSet)
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
        EditChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    }

    caret.dLine++;

    iLineDot = DocLineParamGet(caret.dLine, nullptr, nullptr);
    if (dTargetDot <= iLineDot)
    {
        caret.dXdot = dTargetDot;
        DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);
        EditChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
        EditChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);
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

    EditChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    EditChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);

    return S_OK;
}

// ────── 브러시 ──────

HRESULT ViewBrushStyleSetting(UINT bBrushOn, LPCTSTR ptPattern)
{
    gbPalBucketMode = bBrushOn;

    SendMessage(ghPrntWnd, WMP_BRUSH_TOGGLE, (WPARAM)bBrushOn, (LPARAM)IDM_BRUSH_STYLE);

    if (ptPattern)
    {
        StringCchCopy(gatBrushPtn, SUB_STRING, ptPattern);
    }

    OperationOnStatusBar();

    return S_OK;
}

HRESULT EditorCtrlBrushFilling(VOID)
{
    EDIT_CHANGESET stChangeSet{};

    if (!(gbPalBucketMode))
        return S_FALSE;

    // 선택 범위가 있으면 선택 영역을 우선 채움
    if (ViewEditFillSelection(gatBrushPtn))
        return S_OK;

    const HRESULT hr = CtrlBrushFillAtCaret(&stChangeSet);
    if (SUCCEEDED(hr))
    {
        EditChangeSetApply(stChangeSet);
    }

    return hr;
}

static HRESULT CtrlBrushFillAtCaret(EDIT_CHANGESET *pstChangeSet)
{
    INT dTgDot;
    INT dLeft, dRight, iBgnMozi, iCntMozi;
    BOOLEAN bSpace, bFirst = TRUE;
    LPTSTR ptBuff;
    auto caret = ViewCurrentCaret();

    dTgDot = DocLineStateCheckWithDot(caret.dXdot, caret.dLine, &dLeft, &dRight, &iBgnMozi, &iCntMozi, &bSpace);
    if (!(bSpace))
        return S_FALSE;

    ptBuff = BrushStringMake(dTgDot, gatBrushPtn);
    if (!(ptBuff))
    {
        NotifyBalloonExist(TEXT("ブラシを選んでおいてね"), TEXT("操作ミス"), NIIF_INFO);
        return E_OUTOFMEMORY;
    }

    DocRangeDeleteByMozi(dLeft, caret.dLine, iBgnMozi, (iBgnMozi + iCntMozi), &bFirst);
    DocInsertString(&dLeft, &caret.dLine, nullptr, ptBuff, 0, bFirst);
    bFirst = FALSE;

    caret.dMozi = DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);

    EditChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    EditChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);
    pstChangeSet->bRenewPageInfo = TRUE;

    FREE(ptBuff);

    return S_OK;
}

// ────── 레이아웃 조작 명령 ──────

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

// ────── 브러시 문자열 생성 ──────

LPTSTR BrushStringMake(INT dDotLen, LPTSTR ptPattern)
{
    INT dPtnDot, dCnt, dAmr, i, wid;
    UINT_PTR cchSize;
    LPTSTR ptBuff, ptPadd = nullptr;
    wstring wsBuff;

    wsBuff.clear();

    dPtnDot = ViewStringWidthGet(ptPattern);
    if (0 >= dPtnDot || 0 >= dDotLen)
    {
        return nullptr;
    }

    dCnt = dDotLen / dPtnDot;
    dAmr = dDotLen - (dCnt * dPtnDot);

    for (i = 0; dCnt > i; i++)
    {
        wsBuff += ptPattern;
    }

    i = 0;
    while (0 < dAmr)
    {
        if (0 == ptPattern[i])
            break;

        wid = ViewLetterWidthGet(ptPattern[i]);
        if (wid > dAmr)
            break;
        wsBuff += ptPattern[i];
        dAmr -= wid;
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

    cchSize = wsBuff.size() + 8;
    ptBuff = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
    ZeroMemory(ptBuff, cchSize * sizeof(TCHAR));
    StringCchCopy(ptBuff, cchSize, wsBuff.c_str());

    return ptBuff;
}
