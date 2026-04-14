#include "Sunday.h"
#include "MenuMnemonic.h"

namespace
{

struct MnemonicEntry
{
    UINT dCommandId;
    TCHAR cMnemonic;
};

// 옛날 코드(OrinrinEditor.rc) 기준 니모닉 테이블
constexpr MnemonicEntry gstMnemonicTable[] = {
    // 파일
    {IDM_NEWFILE, TEXT('N')},
    {IDM_OPEN, TEXT('O')},
    {IDM_OPEN_HISTORY, TEXT('H')},
    {IDM_OVERWRITESAVE, TEXT('S')},
    {IDM_RENAMESAVE, TEXT('M')},
    {IDM_IMAGE_SAVE, TEXT('I')},
    {IDM_FILE_CLOSE, TEXT('C')},
    {IDM_GENERAL_OPTION, TEXT('G')},
    {IDM_MENUEDIT_DLG_OPEN, TEXT('E')},
    {IDM_COLOUR_EDIT_OPEN, TEXT('L')},
    {IDM_ACCELKEY_EDIT_DLG_OPEN, TEXT('K')},
    {IDM_ABOUT, TEXT('A')},
    {IDM_EXIT, TEXT('Q')},

    // 편집
    {IDM_UNDO, TEXT('U')},
    {IDM_REDO, TEXT('R')},
    {IDM_CUT, TEXT('T')},
    {IDM_COPY, TEXT('C')},
    {IDM_PASTE, TEXT('P')},
    {IDM_DELETE, TEXT('D')},
    {IDM_SJISCOPY_ALL, TEXT('S')},
    {IDM_ALLSEL, TEXT('A')},
    {IDM_SQSELECT, TEXT('B')},
    {IDM_SQUARE_PASTE, TEXT('Q')},
    {IDM_LAYERBOX, TEXT('L')},
    {IDM_EXTRACTION_MODE, TEXT('E')},
    {IDM_FIND_DLG_OPEN, TEXT('F')},
    {IDM_FIND_HIGHLIGHT_OFF, TEXT('H')},

    // 삽입 - 유니코드 공백
    {IDM_IN_01SPACE, TEXT('1')},
    {IDM_IN_02SPACE, TEXT('2')},
    {IDM_IN_03SPACE, TEXT('3')},
    {IDM_IN_04SPACE, TEXT('4')},
    {IDM_IN_05SPACE, TEXT('5')},
    {IDM_IN_08SPACE, TEXT('8')},
    {IDM_IN_10SPACE, TEXT('1')},
    {IDM_IN_16SPACE, TEXT('1')},

    // 삽입 - 컬러 태그
    {IDM_INSTAG_SPO, TEXT('W')},
    {IDM_INSTAG_COLOUR, TEXT('C')},
    {IDM_INSTAG_GRADIENT, TEXT('G')},

    // 삽입 - 말풍선/유저
    {IDM_FRMINSBOX_OPEN, TEXT('I')},
    {IDM_INSFRAME_EDIT, TEXT('Z')},

    // 변형
    {IDM_RIGHT_GUIDE_SET, TEXT('R')},
    {IDM_INS_TOPSPACE, TEXT('I')},
    {IDM_DEL_TOPSPACE, TEXT('U')},
    {IDM_DEL_LASTSPACE, TEXT('G')},
    {IDM_DEL_LASTLETTER, TEXT('E')},
    {IDM_FILL_SPACE, TEXT('F')},
    {IDM_FILL_ZENSP, TEXT('B')},
    {IDM_HEADHALF_EXCHANGE, TEXT('L')},
    {IDM_MIRROR_INVERSE, TEXT('M')},
    {IDM_UPSET_INVERSE, TEXT('N')},
    {IDM_INCREMENT_DOT, TEXT('D')},
    {IDM_DECREMENT_DOT, TEXT('O')},
    {IDM_INCR_DOT_LINES, TEXT('J')},
    {IDM_DECR_DOT_LINES, TEXT('K')},
    {IDM_DOT_SPLIT_LEFT, TEXT('Q')},
    {IDM_DOT_SPLIT_RIGHT, TEXT('P')},
    {IDM_DOTDIFF_LOCK, TEXT('R')},
    {IDM_DOTDIFF_ADJT, TEXT('D')},

    // 표시
    {IDM_SPACE_VIEW_TOGGLE, TEXT('W')},
    {IDM_GRID_VIEW_TOGGLE, TEXT('H')},
    {IDM_RIGHT_RULER_TOGGLE, TEXT('M')},
    {IDM_UNDER_RULER_TOGGLE, TEXT('S')},
    {IDM_PAGELIST_VIEW, TEXT('L')},
    {IDM_INSERT_PALETTE, TEXT('T')},
    {IDM_BRUSH_PALETTE, TEXT('F')},
    {IDM_TRACE_MODE_ON, TEXT('R')},
    {IDM_ON_PREVIEW, TEXT('P')},

    // 페이지 목록 팝업
    {IDM_PAGEL_AATIP_TOGGLE, TEXT('T')},
    {IDM_PAGEL_ADD, TEXT('N')},
    {IDM_PAGEL_INSERT, TEXT('I')},
    {IDM_PAGEL_DUPLICATE, TEXT('C')},
    {IDM_PAGEL_DELETE, TEXT('D')},
    {IDM_PAGEL_UPFLOW, TEXT('W')},
    {IDM_PAGEL_DOWNSINK, TEXT('S')},
    {IDM_PAGEL_RENAME, TEXT('R')},
    {IDM_TOPMOST_TOGGLE, TEXT('Z')},

    // 레이어 박스 팝업
    {IDM_LYB_COPY, TEXT('C')},
    {IDM_LYB_TRANCE_RELEASE, TEXT('R')},
    {IDM_LYB_TRANCE_ALL, TEXT('T')},

    // 멀티파일 팝업 (IDM_OVERWRITESAVE 등은 파일 섹션에서 이미 등록)
    // IDM_FILE_CLOSE는 'C'가 이미 등록, 팝업에서는 'Q' 사용 → 팝업별 오버라이드 불가하므로 별도 처리 불필요
};

constexpr INT giMnemonicCount =
    static_cast<INT>(sizeof(gstMnemonicTable) / sizeof(gstMnemonicTable[0]));

TCHAR MnemonicCharGet(UINT dCommandId)
{
    for (INT i = 0; giMnemonicCount > i; i++)
    {
        if (gstMnemonicTable[i].dCommandId == dCommandId)
            return gstMnemonicTable[i].cMnemonic;
    }
    return 0;
}

BOOL TextAlreadyHasMnemonic(LPCTSTR ptText)
{
    if (!ptText)
        return TRUE;

    for (LPCTSTR p = ptText; TEXT('\0') != *p; p++)
    {
        if (TEXT('&') == *p)
            return TRUE;
    }
    return FALSE;
}

void MenuMnemonicApplyRecursive(HMENU hMenu)
{
    INT iCount = GetMenuItemCount(hMenu);
    TCHAR atText[MAX_STRING];
    MENUITEMINFO stMii;

    for (INT i = 0; iCount > i; i++)
    {
        ZeroMemory(&stMii, sizeof(stMii));
        stMii.cbSize = sizeof(MENUITEMINFO);
        stMii.fMask = MIIM_ID | MIIM_STRING | MIIM_SUBMENU | MIIM_FTYPE;
        stMii.dwTypeData = atText;
        stMii.cch = MAX_STRING;

        if (!GetMenuItemInfo(hMenu, i, TRUE, &stMii))
            continue;

        // 서브메뉴가 있으면 재귀
        if (stMii.hSubMenu)
            MenuMnemonicApplyRecursive(stMii.hSubMenu);

        // 세퍼레이터는 건너뛰기
        if (stMii.fType & MFT_SEPARATOR)
            continue;

        // 이미 니모닉이 있으면 건너뛰기
        if (TextAlreadyHasMnemonic(atText))
            continue;

        TCHAR cMnemonic = MnemonicCharGet(stMii.wID);
        if (0 == cMnemonic)
            continue;

        // "텍스트" → "텍스트(&X)" 형태로 변환
        size_t cchLen = 0;
        StringCchLength(atText, MAX_STRING, &cchLen);

        if (cchLen + 4 >= MAX_STRING)
            continue;

        TCHAR atSuffix[8];
        StringCchPrintf(atSuffix, 8, TEXT("(&%c)"), cMnemonic);
        StringCchCat(atText, MAX_STRING, atSuffix);

        stMii.fMask = MIIM_STRING;
        stMii.dwTypeData = atText;
        SetMenuItemInfo(hMenu, i, TRUE, &stMii);
    }
}
} // namespace

void MenuMnemonicApply(HMENU hMenu)
{
    if (!hMenu)
        return;
    MenuMnemonicApplyRecursive(hMenu);
}
