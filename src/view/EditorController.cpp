// 편집 컨트롤러

#include "EditorController.h"
#include "ViewCentralInternal.h"
#include "Palette.h"

EDIT_CHANGESET *g_pActiveChangeSet = nullptr;

UINT gbPalBucketMode;
static TCHAR gatBrushPtn[SUB_STRING];

static VOID CtrlRedrawChangedLines(EDIT_CHANGESET *pstChangeSet, INT dLine, INT iLines, INT bCrLf);
static VOID CtrlReplayUndoRedo(EDIT_CHANGESET *pstChangeSet, BOOLEAN bRedo);
static VOID CtrlDeleteAtCaret(EDIT_CHANGESET *pstChangeSet, BOOLEAN bBackward);
static INT CtrlInsertLiteralCharacter(EDIT_CHANGESET *pstChangeSet, TCHAR ch);
static INT CtrlInsertStringWithStyle(EDIT_CHANGESET *pstChangeSet, LPCTSTR ptText, UINT dStyle, PINT pdMozi);
static HRESULT CtrlBrushFillAtCaret(EDIT_CHANGESET *pstChangeSet);
static HRESULT CtrlScriptedLineFeed(VOID);
static HRESULT CtrlScriptedLineFeedAtDot(INT dTargetDot, EDIT_CHANGESET *pstChangeSet);

// ChangeSet 적용

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

// 내부 도우미

static VOID CtrlRedrawChangedLines(EDIT_CHANGESET *pstChangeSet, INT dLine, INT iLines, INT bCrLf)
{
    if (0 < bCrLf)
    {
        EditChangeSetRequestRedrawRange(pstChangeSet, dLine, iLines);
        return;
    }

    EditChangeSetRequestRedrawLine(pstChangeSet, dLine);
}

static VOID CtrlReplayUndoRedo(EDIT_CHANGESET *pstChangeSet, BOOLEAN bRedo)
{
    auto caret = ViewCurrentCaret();

    DocPageSelStateToggle(-1);

    const INT dCrLf = bRedo ? DocRedoExecute(&caret.dXdot, &caret.dLine)
                            : DocUndoExecute(&caret.dXdot, &caret.dLine);
    if (dCrLf)
    {
        EditChangeSetRequestRedrawAll(pstChangeSet);
    }
    else
    {
        EditChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    }

    EditChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);
}

static VOID CtrlDeleteAtCaret(EDIT_CHANGESET *pstChangeSet, BOOLEAN bBackward)
{
    UINT bSqSel = 0;
    const BOOLEAN bSelect = IsSelecting(&bSqSel);
    const INT iLines = DocNowFilePageLineCount();
    auto caret = ViewCurrentCaret();

    const INT bCrLf = bSelect ? DocSelectedDelete(&caret.dXdot, &caret.dLine, bSqSel, TRUE)
                              : (bBackward ? DocInputBkSpace(&caret.dXdot, &caret.dLine)
                                           : DocInputDelete(caret.dXdot, caret.dLine));

    CtrlRedrawChangedLines(pstChangeSet, caret.dLine, iLines, bCrLf);
    EditChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);
    pstChangeSet->bRenewPageInfo = TRUE;
}

static INT CtrlInsertLiteralCharacter(EDIT_CHANGESET *pstChangeSet, TCHAR ch)
{
    auto caret = ViewCurrentCaret();

    const INT width = DocInsertLetter(&caret.dXdot, caret.dLine, ch);
    caret.dMozi = DocLetterPosGetAdjust(&caret.dXdot, caret.dLine, 0);
    EditChangeSetRequestCaretMove(pstChangeSet, caret.dXdot, caret.dLine);
    EditChangeSetRequestRedrawLine(pstChangeSet, caret.dLine);
    pstChangeSet->bRenewPageInfo = TRUE;

    return width;
}

static INT CtrlInsertStringWithStyle(EDIT_CHANGESET *pstChangeSet, LPCTSTR ptText, UINT dStyle, PINT pdMozi)
{
    auto caret = ViewCurrentCaret();

    const INT dCrLf = DocInsertString(&caret.dXdot, &caret.dLine, pdMozi ? pdMozi : &caret.dMozi, ptText, dStyle, TRUE);
    pstChangeSet->bRenewPageInfo = TRUE;

    return dCrLf;
}

// 편집 명령

VOID ViewEditUndoForward(VOID)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    CtrlReplayUndoRedo(&stChangeSet, FALSE);
    EditChangeSetApply(stChangeSet);
}

VOID ViewEditRedoForward(VOID)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    CtrlReplayUndoRedo(&stChangeSet, TRUE);
    EditChangeSetApply(stChangeSet);
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
    EditChangeSetScope scope(&stChangeSet);
    auto caret = ViewCurrentCaret();
    const INT rslt = DocSelectedBrushFilling(ptPattern, &caret.dXdot, &caret.dLine);

    if (rslt)
    {
        stChangeSet.bRenewPageInfo = TRUE;
        EditChangeSetRequestCaretMove(&stChangeSet, caret.dXdot, caret.dLine);
    }

    EditChangeSetApply(stChangeSet);

    return rslt;
}

VOID ViewEditDeleteForward(VOID)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    CtrlDeleteAtCaret(&stChangeSet, FALSE);
    EditChangeSetApply(stChangeSet);
}

VOID ViewEditDeleteBackward(VOID)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    CtrlDeleteAtCaret(&stChangeSet, TRUE);
    EditChangeSetApply(stChangeSet);
    gdXmemory = ViewCurrentCaret().dXdot;
}

HRESULT ViewEditInsertLineBreak(BOOLEAN bScripted)
{
    if (bScripted)
    {
        return CtrlScriptedLineFeed();
    }

    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
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
    EditChangeSetRequestRedrawLine(&stChangeSet, caret.dLine);

    caret.dXdot = 0;
    caret.dMozi = 0;
    caret.dLine++;
    EditChangeSetRequestCaretMove(&stChangeSet, caret.dXdot, caret.dLine);
    gdXmemory = caret.dXdot;

    const INT iLines = DocPageParamGet(nullptr, nullptr);
    EditChangeSetRequestRedrawRange(&stChangeSet, caret.dLine, iLines);

    EditChangeSetApply(stChangeSet);

    return S_OK;
}

INT ViewEditInputCharacter(TCHAR ch)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
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

    const INT width = CtrlInsertLiteralCharacter(&stChangeSet, ch);
    gdXmemory = caret.dXdot;

    if (bCrLf)
    {
        EditChangeSetRequestRedrawRange(&stChangeSet, caret.dLine, iLines);
    }
    else
    {
        EditChangeSetRequestRedrawLine(&stChangeSet, caret.dLine);
    }

    stChangeSet.bRenewPageInfo = TRUE;
    EditChangeSetApply(stChangeSet);

    return width;
}

INT ViewEditPasteFromClipboard(UINT bSquareMode)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    auto caret = ViewCurrentCaret();

    const INT result = DocInputFromClipboard(&caret.dXdot, &caret.dLine, &caret.dMozi, bSquareMode);

    EditChangeSetApply(stChangeSet);

    return result;
}

HRESULT ViewEditCopyPageAll(VOID)
{
    return DocPageAllCopy(D_UNI | D_ENTITY);
}

// 삽입 명령

HRESULT ViewFrameInsert(INT dMode)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);

    const HRESULT hr = DocFrameInsert(dMode, ViewSquareSelectModeGet());

    EditChangeSetApply(stChangeSet);
    return hr;
}

HRESULT ViewInsertSpoTag(VOID)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);

    const HRESULT hr = DocFrameInsert(-1, ViewSquareSelectModeGet());

    EditChangeSetApply(stChangeSet);
    return hr;
}

// 삽입 보조

INT ViewInsertUniSpace(UINT dCommando)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    INT width;
    TCHAR ch;

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

    width = CtrlInsertLiteralCharacter(&stChangeSet, ch);

    EditChangeSetApply(stChangeSet);

    return width;
}

INT ViewInsertTmpleString(LPCTSTR ptText)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    INT dDot;
    auto caret = ViewCurrentCaret();

    dDot = caret.dXdot;

    CtrlInsertStringWithStyle(&stChangeSet, ptText, 0, &caret.dMozi);

    EditChangeSetApply(stChangeSet);

    dDot = ViewCurrentCaret().dXdot - dDot;

    return dDot;
}

// 대사용 개행

static HRESULT CtrlScriptedLineFeed(VOID)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
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

// 브러시

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
    EditChangeSetScope scope(&stChangeSet);

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

    if (!(gatBrushPtn[0]))
    {
        NotifyBalloonExist(TEXT("브러시를 먼저 선택해 주세요"), TEXT("입력 확인"), NIIF_INFO);
        return S_FALSE;
    }

    dTgDot = DocLineStateCheckWithDot(caret.dXdot, caret.dLine, &dLeft, &dRight, &iBgnMozi, &iCntMozi, &bSpace);
    if (!(bSpace))
        return S_FALSE;

    ptBuff = BrushStringMake(dTgDot, gatBrushPtn);
    if (!(ptBuff))
    {
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

// 레이아웃 조작 명령

BOOLEAN ViewLayoutCommandForward(INT id, HWND hMainWindow)
{
    EDIT_CHANGESET stChangeSet{};
    EditChangeSetScope scope(&stChangeSet);
    auto caret = ViewCurrentCaret();
    const UINT dSquareSelect = ViewSquareSelectModeGet();

    UNREFERENCED_PARAMETER(hMainWindow);

    switch (id)
    {
    case IDM_RIGHT_GUIDE_SET:
        DocRightGuideline(nullptr);
        break;

    case IDM_DEL_LASTSPACE:
        DocLastSpaceErase(&caret.dXdot, caret.dLine);
        break;

    case IDM_INS_TOPSPACE:
        DocTopLetterInsert(TEXT('　'), &caret.dXdot, caret.dLine);
        break;

    case IDM_DEL_TOPSPACE:
        DocTopSpaceErase(&caret.dXdot, caret.dLine);
        break;

    case IDM_DEL_LASTLETTER:
        DocLastLetterErase(&caret.dXdot, caret.dLine);
        break;

    case IDM_INCREMENT_DOT:
        DocSpaceShiftProc(VK_RIGHT, &caret.dXdot, caret.dLine);
        break;

    case IDM_DECREMENT_DOT:
        DocSpaceShiftProc(VK_LEFT, &caret.dXdot, caret.dLine);
        break;

    case IDM_INCR_DOT_LINES:
        DocPositionShift(VK_RIGHT, &caret.dXdot, caret.dLine);
        break;

    case IDM_DECR_DOT_LINES:
        DocPositionShift(VK_LEFT, &caret.dXdot, caret.dLine);
        break;

    case IDM_DOT_SPLIT_RIGHT:
        DocCentreWidthShift(VK_RIGHT, &caret.dXdot, caret.dLine);
        break;

    case IDM_DOT_SPLIT_LEFT:
        DocCentreWidthShift(VK_LEFT, &caret.dXdot, caret.dLine);
        break;

    case IDM_DOTDIFF_LOCK:
        gdAutoDiffBase = DocDiffAdjBaseSet(caret.dLine);
        ViewRulerRedraw(-1, -1);
        EditChangeSetApply(stChangeSet);
        return TRUE;

    case IDM_DOTDIFF_ADJT:
        DocDiffAdjExec(&caret.dXdot, caret.dLine);
        break;

    case IDM_HEADHALF_EXCHANGE:
        DocHeadHalfSpaceExchange(hMainWindow);
        break;

    case IDM_FILL_ZENSP:
        DocScreenFill(TEXT("　"));
        break;

    case IDM_MIRROR_INVERSE:
        DocInverseTransform(dSquareSelect, 1, &caret.dXdot, caret.dLine);
        break;

    case IDM_UPSET_INVERSE:
        DocInverseTransform(dSquareSelect, 0, &caret.dXdot, caret.dLine);
        break;

    default:
        return FALSE;
    }

    EditChangeSetApply(stChangeSet);
    return TRUE;
}

// 브러시 문자열 생성

LPTSTR BrushStringMake(INT dDotLen, LPTSTR ptPattern)
{
    INT dPtnDot, dCnt, dAmr, i, wid;
    UINT_PTR cchSize;
    LPTSTR ptBuff, ptPadd = nullptr;
    wstring wsBuff;

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

    cchSize = wsBuff.size() + 1;
    ptBuff = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
    if (!(ptBuff))
    {
        return nullptr;
    }
    StringCchCopy(ptBuff, cchSize, wsBuff.c_str());

    return ptBuff;
}
