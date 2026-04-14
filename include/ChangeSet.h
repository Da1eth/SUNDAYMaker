#pragma once

// Doc 의 편집 결과를 View/App 반영 요청으로 표현하는 결과 구조체.
// Doc 함수가 bridge 를 통해 즉시 View 를 부르는 대신,
// ChangeSet 에 요청을 누적하고 Controller 가 한꺼번에 적용한다.

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
    EDIT_CHANGESET_REDRAW_KIND dRedrawKind{EDIT_CHANGESET_REDRAW_NONE};
    INT dRedrawTopLine{};
    INT dRedrawBottomLine{};

    EDIT_CHANGESET_CARET_KIND dCaretKind{EDIT_CHANGESET_CARET_NONE};
    INT dCaretXdot{};
    INT dCaretLine{};

    UINT bRenewPageInfo{};

    // 선택 초기화 (DocViewClearSelection 이 스코프 안에서 호출됨을 표시)
    UINT bClearSelection{};
};

// ChangeSet 요청 도우미

VOID EditChangeSetApply(const EDIT_CHANGESET &stChangeSet);
VOID EditChangeSetRequestRedrawLine(EDIT_CHANGESET *pstChangeSet, INT dLine);
VOID EditChangeSetRequestRedrawRange(EDIT_CHANGESET *pstChangeSet, INT dTopLine, INT dBottomLine);
VOID EditChangeSetRequestRedrawAll(EDIT_CHANGESET *pstChangeSet);
VOID EditChangeSetRequestCaretMove(EDIT_CHANGESET *pstChangeSet, INT dXdot, INT dLine);

// ChangeSet 스코프
// 스코프가 활성 상태일 때, bridge 함수의 다시 그리기·캐럿 이동 호출은
// View 를 직접 부르지 않고 활성 ChangeSet 에 누적된다.

extern EDIT_CHANGESET *g_pActiveChangeSet;

struct EditChangeSetScope
{
    EDIT_CHANGESET *pPrev;

    explicit EditChangeSetScope(EDIT_CHANGESET *pCS)
        : pPrev(g_pActiveChangeSet)
    {
        g_pActiveChangeSet = pCS;
    }

    ~EditChangeSetScope()
    {
        g_pActiveChangeSet = pPrev;
    }

    EditChangeSetScope(const EditChangeSetScope &) = delete;
    EditChangeSetScope &operator=(const EditChangeSetScope &) = delete;
};
