#pragma once

#include "Sunday.h"
#include "ChangeSet.h"

// 편집 컨트롤러 — View 입력 핸들러가 이 API 만 호출하고,
// Doc 호출·ChangeSet 조립·View 갱신은 여기서만 수행한다.

// 편집 명령
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
HRESULT ViewEditCopyPageAll(VOID);

// 삽입 보조
HRESULT ViewFrameInsert(INT dMode);
HRESULT ViewInsertSpoTag(VOID);
INT ViewInsertUniSpace(UINT dCommando);
INT ViewInsertTmpleString(LPCTSTR ptText);

// 브러시
HRESULT ViewBrushStyleSetting(UINT bBrushOn, LPCTSTR ptPattern);
HRESULT EditorCtrlBrushFilling(VOID);

// 레이아웃 조작 명령
BOOLEAN ViewLayoutCommandForward(INT id, HWND hMainWindow);
