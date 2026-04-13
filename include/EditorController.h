#pragma once

#include "Sunday.h"

// 편집 컨트롤러 — View 가 Doc 쓰기 함수를 직접 조합하지 않도록 중간 계층을 둔다.
// ViewKeyButton.cpp 등 입력 핸들러는 이 API 만 호출하고,
// Doc 호출·ChangeSet 조립·View 갱신은 여기서만 수행한다.

// ────── ChangeSet 인프라 ──────

enum EDIT_CHANGESET_REDRAW_KIND
{
    EDIT_CHANGESET_REDRAW_NONE = 0,
    EDIT_CHANGESET_REDRAW_LINE,
    EDIT_CHANGESET_REDRAW_RANGE,
    EDIT_CHANGESET_REDRAW_ALL,
};

enum EDIT_CHANGESET_CARET_KIND
{
    EDIT_CHANGESET_CARET_NONE = 0,
    EDIT_CHANGESET_CARET_MOVE,
};

struct EDIT_CHANGESET
{
    UINT bRenewPageInfo{};
    EDIT_CHANGESET_REDRAW_KIND dRedrawKind{EDIT_CHANGESET_REDRAW_NONE};
    INT dRedrawTopLine{};
    INT dRedrawBottomLine{};
    EDIT_CHANGESET_CARET_KIND dCaretKind{EDIT_CHANGESET_CARET_NONE};
    INT dCaretXdot{};
    INT dCaretLine{};
};

VOID EditChangeSetApply(const EDIT_CHANGESET &stChangeSet);
VOID EditChangeSetRequestRedrawLine(EDIT_CHANGESET *pstChangeSet, INT dLine);
VOID EditChangeSetRequestRedrawRange(EDIT_CHANGESET *pstChangeSet, INT dTopLine, INT dBottomLine);
VOID EditChangeSetRequestRedrawAll(EDIT_CHANGESET *pstChangeSet);
VOID EditChangeSetRequestCaretMove(EDIT_CHANGESET *pstChangeSet, INT dXdot, INT dLine);

// ────── 편집 명령 (기존 ViewEdit* 이름 유지) ──────

VOID ViewEditUndoForward(VOID);
VOID ViewEditRedoForward(VOID);
VOID ViewEditCopySelection(UINT dSquareMode);
VOID ViewEditExecuteExtraction(HINSTANCE hInstance);
VOID ViewEditCutSelection(UINT dSquareMode);
INT ViewEditFillSelection(LPTSTR ptPattern);
VOID ViewEditDeleteForward(VOID);
VOID ViewEditDeleteBackward(VOID);
HRESULT ViewEditInsertLineBreak(BOOLEAN bScripted);
INT ViewEditInputCharacter(TCHAR ch);
INT ViewEditPasteFromClipboard(UINT bSquareMode);

// ────── 삽입 보조 ──────

INT ViewInsertUniSpace(UINT dCommando);
INT ViewInsertTmpleString(LPCTSTR ptText);

// ────── 브러시 ──────

HRESULT ViewBrushStyleSetting(UINT bBrushOn, LPCTSTR ptPattern);
HRESULT EditorCtrlBrushFilling(VOID);

// ────── 레이아웃 조작 명령 ──────

BOOLEAN ViewLayoutCommandForward(INT id, HWND hMainWindow);
