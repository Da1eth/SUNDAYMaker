#pragma once

#include "Sunday.h"
#include "ChangeSet.h"

// Doc 이 View 구현 세부를 직접 알지 않도록 UI 반영 호출을 한곳으로 모은다.
// ChangeSet 스코프가 활성 상태이면, 다시 그리기·캐럿 이동은 즉시 실행하지 않고
// 활성 ChangeSet 에 누적한다. 데이터 변경이 수반되는 호출(선택 초기화 등)은
// 즉시 실행하되, 내부에서 발생하는 다시 그리기 요청은 스코프에 의해 누적된다.

inline VOID DocViewRefreshLine(INT dLine)
{
    if (g_pActiveChangeSet)
    {
        EditChangeSetRequestRedrawLine(g_pActiveChangeSet, dLine);
        return;
    }
    ViewRedrawSetLine(dLine);
}

inline VOID DocViewRefreshAll()
{
    if (g_pActiveChangeSet)
    {
        EditChangeSetRequestRedrawAll(g_pActiveChangeSet);
        return;
    }
    DocViewRefreshLine(-1);
}

inline VOID DocViewRefreshLines(INT dTopLine, INT dBottomLine)
{
    if (0 > dTopLine || dTopLine > dBottomLine)
    {
        return;
    }

    if (g_pActiveChangeSet)
    {
        EditChangeSetRequestRedrawRange(g_pActiveChangeSet, dTopLine, dBottomLine);
        return;
    }

    for (INT iLine = dTopLine; dBottomLine >= iLine; ++iLine)
    {
        ViewRedrawSetLine(iLine);
    }
}

inline VOID DocViewRefreshRect(LPRECT pRect)
{
    if (g_pActiveChangeSet && pRect)
    {
        // Rect 좌표를 줄 번호로 변환 (Doc 좌표계: top = line * LINE_HEIGHT)
        INT dTopLine = pRect->top / LINE_HEIGHT;
        INT dBottomLine = pRect->bottom / LINE_HEIGHT;
        if (dTopLine == dBottomLine)
        {
            EditChangeSetRequestRedrawLine(g_pActiveChangeSet, dTopLine);
        }
        else
        {
            EditChangeSetRequestRedrawRange(g_pActiveChangeSet, dTopLine, dBottomLine);
        }
        return;
    }
    ViewRedrawSetRect(pRect);
}

inline VOID DocViewMoveCaret(INT dXdot, INT dLine)
{
    if (g_pActiveChangeSet)
    {
        EditChangeSetRequestCaretMove(g_pActiveChangeSet, dXdot, dLine);
        return;
    }
    ViewDrawCaret(dXdot, dLine, TRUE);
}

// 아래 함수들은 데이터 변경이 수반되므로 스코프 여부와 관계없이 즉시 실행한다.
// 내부에서 발생하는 DocViewRefreshLine/Rect 호출은 스코프에 의해 자동 누적된다.

inline VOID DocViewResetCaret(INT dXdot, INT dLine)
{
    ViewPosResetCaret(dXdot, dLine);
}

inline VOID DocViewSelectionMoveCheck(BOOLEAN bSelecting)
{
    ViewSelMoveCheck(bSelecting);
}

inline VOID DocViewSelectionPositionSet(LPPOINT pPoint)
{
    ViewSelPositionSet(pPoint);
}

inline VOID DocViewClearSelection()
{
    // 즉시 실행 — 문서 데이터(CT_SELECT 스타일 비트)를 변경한다.
    // 내부의 DocPageSelStateToggle → DocViewRefreshRect 호출은 스코프에 의해 누적.
    ViewSelPageAll(-1);
}

inline VOID DocViewFocusEditor()
{
    ViewFocusSet();
}

inline VOID DocViewResetEditState()
{
    ViewEditReset();
}