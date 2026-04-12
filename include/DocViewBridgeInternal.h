#pragma once

#include "Sunday.h"

// Doc 이 View 구현 세부를 직접 알지 않도록 UI 반영 호출을 한곳으로 모은다.
inline VOID DocViewRefreshLine(INT dLine)
{
    ViewRedrawSetLine(dLine);
}

inline VOID DocViewRefreshAll()
{
    DocViewRefreshLine(-1);
}

inline VOID DocViewRefreshLines(INT dTopLine, INT dBottomLine)
{
    if (0 > dTopLine || dTopLine > dBottomLine)
    {
        return;
    }

    for (INT iLine = dTopLine; dBottomLine >= iLine; ++iLine)
    {
        DocViewRefreshLine(iLine);
    }
}

inline VOID DocViewRefreshRect(LPRECT pRect)
{
    ViewRedrawSetRect(pRect);
}

inline VOID DocViewMoveCaret(INT dXdot, INT dLine)
{
    ViewDrawCaret(dXdot, dLine, TRUE);
}

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