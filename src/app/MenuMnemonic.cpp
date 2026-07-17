#include "Sunday.h"
#include "MenuMnemonic.h"
#include "UiText.h"

namespace
{

struct MnemonicEntry
{
    UINT dCommandId;
    TCHAR cMnemonic;
};

struct SubmenuMnemonicEntry
{
    UINT dItemId;
    LPCTSTR ptLabel;
    TCHAR cMnemonic;
};

constexpr UINT MNID_MAIN_FILE = 60001;
constexpr UINT MNID_MAIN_EDIT = 60002;
constexpr UINT MNID_MAIN_INSERT = 60003;
constexpr UINT MNID_MAIN_TRANSFORM = 60004;
constexpr UINT MNID_MAIN_VIEW = 60005;
constexpr UINT MNID_OPEN_HISTORY = 60006;
constexpr UINT MNID_UNISPACE = 60007;
constexpr UINT MNID_COLOUR = 60008;
constexpr UINT MNID_FRAME = 60009;
constexpr UINT MNID_USER = 60010;
constexpr UINT MNID_DOT_ADJUST = 60011;

// 니모닉 테이블
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
    {IDM_MNEMONICKEY_EDIT_DLG_OPEN, TEXT('V')},
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

    // 삽입 - 유니코드 공백
    {IDM_IN_01SPACE, TEXT('1')},
    {IDM_IN_02SPACE, TEXT('2')},
    {IDM_IN_03SPACE, TEXT('3')},
    {IDM_IN_04SPACE, TEXT('4')},
    {IDM_IN_05SPACE, TEXT('5')},
    {IDM_IN_08SPACE, TEXT('8')},
    {IDM_IN_10SPACE, TEXT('A')},
    {IDM_IN_16SPACE, TEXT('F')},

    // 삽입 - 컬러 태그
    {IDM_INSTAG_SPO, TEXT('W')},
    {IDM_INSTAG_COLOUR, TEXT('C')},
    {IDM_INSTAG_GRADIENT, TEXT('G')},

    // 삽입 - 말풍선/유저
    {IDM_FRMINSBOX_OPEN, TEXT('I')},
    {IDM_INSFRAME_EDIT, TEXT('Z')},
    {IDM_PALETTE_EDIT_OPEN, TEXT('P')},

    // 변형
    {IDM_RIGHT_GUIDE_SET, TEXT('R')},
    {IDM_INS_TOPSPACE, TEXT('I')},
    {IDM_DEL_TOPSPACE, TEXT('U')},
    {IDM_DEL_LASTSPACE, TEXT('G')},
    {IDM_DEL_LASTLETTER, TEXT('E')},
    {IDM_FILL_SPACE, TEXT('Q')},
    {IDM_FILL_ZENSP, TEXT('B')},
    {IDM_HEADHALF_EXCHANGE, TEXT('L')},
    {IDM_MIRROR_INVERSE, TEXT('M')},
    {IDM_UPSET_INVERSE, TEXT('N')},
    {IDM_INCREMENT_DOT, TEXT('D')},
    {IDM_DECREMENT_DOT, TEXT('O')},
    {IDM_INCR_DOT_LINES, TEXT('K')},
    {IDM_DECR_DOT_LINES, TEXT('J')},
    {IDM_DOT_SPLIT_LEFT, TEXT('O')},
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

    // IDM_FILE_CLOSE는 'C'가 이미 등록, 팝업에서는 'Q' 사용 → 팝업별 오버라이드 불가하므로 별도 처리 불필요
};

constexpr SubmenuMnemonicEntry gstSubmenuMnemonicTable[] = {
    {MNID_MAIN_FILE, TEXT("파일"), TEXT('F')},
    {MNID_MAIN_EDIT, TEXT("편집"), TEXT('E')},
    {MNID_MAIN_INSERT, TEXT("삽입"), TEXT('I')},
    {MNID_MAIN_TRANSFORM, TEXT("변형"), TEXT('P')},
    {MNID_MAIN_VIEW, TEXT("표시"), TEXT('N')},
    {MNID_OPEN_HISTORY, ORR_UI_LABEL_OPEN_HISTORY, TEXT('H')},
    {MNID_UNISPACE, ORR_UI_LABEL_MN_UNISPACE, TEXT('S')},
    {MNID_COLOUR, ORR_UI_LABEL_MN_COLOUR_SEL, TEXT('C')},
    {MNID_FRAME, ORR_UI_LABEL_MN_INSFRAME_SEL, TEXT('F')},
    {MNID_USER, ORR_UI_LABEL_MN_USER_REFS, TEXT('U')},
    {MNID_DOT_ADJUST, TEXT("도트 조정"), TEXT('A')},
};

constexpr INT giMnemonicCount =
    static_cast<INT>(sizeof(gstMnemonicTable) / sizeof(gstMnemonicTable[0]));

constexpr INT giSubmenuMnemonicCount =
    static_cast<INT>(sizeof(gstSubmenuMnemonicTable) /
                     sizeof(gstSubmenuMnemonicTable[0]));

LPCTSTR ScopeNameGet(MENU_MNEMONIC_SCOPE eScope)
{
    switch (eScope)
    {
    case MENU_MNEMONIC_MAIN: return TEXT("Main");
    case MENU_MNEMONIC_EDITOR_CONTEXT: return TEXT("EditorContext");
    case MENU_MNEMONIC_PAGE_LIST: return TEXT("PageList");
    case MENU_MNEMONIC_LAYER_BOX: return TEXT("LayerBox");
    case MENU_MNEMONIC_DOCUMENT_TAB: return TEXT("DocumentTab");
    default: return TEXT("Other");
    }
}

TCHAR MnemonicSettingGet(MENU_MNEMONIC_SCOPE eScope, UINT dCommandId,
                         TCHAR cDefault)
{
    TCHAR atKey[48];
    TCHAR atValue[8];
    LPCTSTR ptPath = CntxIniPathGet();

    if (!ptPath || !ptPath[0])
        return cDefault;

    StringCchPrintf(atKey, 48, TEXT("%s_%u"), ScopeNameGet(eScope), dCommandId);
    GetPrivateProfileString(TEXT("Mnemonic"), atKey, TEXT(""), atValue, 8,
                            ptPath);
    if (!atValue[0])
        return cDefault;
    if (TEXT('-') == atValue[0])
        return 0;
    return (TCHAR)_totupper(atValue[0]);
}

void MnemonicSettingPut(MENU_MNEMONIC_SCOPE eScope, UINT dCommandId,
                        TCHAR cMnemonic)
{
    TCHAR atKey[48];
    TCHAR atValue[2] = {cMnemonic ? (TCHAR)_totupper(cMnemonic) : TEXT('-'), 0};
    LPCTSTR ptPath = CntxIniPathGet();
    if (!ptPath || !ptPath[0])
        return;
    StringCchPrintf(atKey, 48, TEXT("%s_%u"), ScopeNameGet(eScope), dCommandId);
    WritePrivateProfileString(TEXT("Mnemonic"), atKey, atValue, ptPath);
}

TCHAR MnemonicDefaultCharGet(UINT dCommandId)
{
    for (INT i = 0; giMnemonicCount > i; i++)
    {
        if (gstMnemonicTable[i].dCommandId == dCommandId)
            return gstMnemonicTable[i].cMnemonic;
    }
    return 0;
}

TCHAR MnemonicCharGet(MENU_MNEMONIC_SCOPE eScope, UINT dCommandId)
{
    TCHAR cDefault = MnemonicDefaultCharGet(dCommandId);
    if (MENU_MNEMONIC_DOCUMENT_TAB == eScope && IDM_FILE_CLOSE == dCommandId)
        cDefault = TEXT('Q');
    return MnemonicSettingGet(eScope, dCommandId, cDefault);
}

TCHAR SubmenuMnemonicCharGet(MENU_MNEMONIC_SCOPE eScope, LPCTSTR ptText)
{
    if (!ptText)
        return 0;

    for (INT i = 0; giSubmenuMnemonicCount > i; i++)
    {
        if (0 == lstrcmp(ptText, gstSubmenuMnemonicTable[i].ptLabel))
            return MnemonicSettingGet(eScope,
                                      gstSubmenuMnemonicTable[i].dItemId,
                                      gstSubmenuMnemonicTable[i].cMnemonic);
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

void MenuMnemonicApplyRecursive(HMENU hMenu, MENU_MNEMONIC_SCOPE eScope)
{
    INT iCount = GetMenuItemCount(hMenu);
    TCHAR atText[MAX_STRING];
    TCHAR atAccelerator[MAX_STRING];
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
            MenuMnemonicApplyRecursive(stMii.hSubMenu, eScope);

        // 세퍼레이터는 건너뛰기
        if (stMii.fType & MFT_SEPARATOR)
            continue;

        // 메뉴의 단축키 표시는 "\tCtrl+..." 꼴로 니모닉 뒤에 붙는다.
        // 니모닉만 교체할 수 있도록 잠시 분리했다가 마지막에 다시 붙인다.
        atAccelerator[0] = 0;
        LPTSTR ptAccelerator = StrChr(atText, TEXT('\t'));
        if (ptAccelerator)
        {
            StringCchCopy(atAccelerator, MAX_STRING, ptAccelerator);
            *ptAccelerator = 0;
        }

        // 다시 적용할 때 기존 "(&X)" 접미사를 제거한다.
        BOOL bRemovedMnemonic = FALSE;
        size_t cchText = lstrlen(atText);
        if (4 <= cchText && TEXT('(') == atText[cchText - 4] &&
            TEXT('&') == atText[cchText - 3] &&
            TEXT(')') == atText[cchText - 1])
        {
            atText[cchText - 4] = 0;
            bRemovedMnemonic = TRUE;
        }
        else if (TextAlreadyHasMnemonic(atText))
            continue;

        TCHAR cMnemonic = MnemonicCharGet(eScope, stMii.wID);
        if (0 == cMnemonic && stMii.hSubMenu)
            cMnemonic = SubmenuMnemonicCharGet(eScope, atText);
        if (0 == cMnemonic && !bRemovedMnemonic)
            continue;

        // "텍스트" → "텍스트(&X)" 형태로 변환
        size_t cchLen = 0;
        StringCchLength(atText, MAX_STRING, &cchLen);

        if (cchLen + 4 >= MAX_STRING)
            continue;

        if (cMnemonic)
        {
            TCHAR atSuffix[8];
            StringCchPrintf(atSuffix, 8, TEXT("(&%c)"), cMnemonic);
            StringCchCat(atText, MAX_STRING, atSuffix);
        }
        StringCchCat(atText, MAX_STRING, atAccelerator);

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
    MenuMnemonicApplyRecursive(hMenu, MENU_MNEMONIC_MAIN);
}

void MenuMnemonicApplyScoped(HMENU hMenu, MENU_MNEMONIC_SCOPE eScope)
{
    if (!hMenu)
        return;
    MenuMnemonicApplyRecursive(hMenu, eScope);
}

TCHAR MenuMnemonicValueGet(MENU_MNEMONIC_SCOPE eScope, UINT dItemId,
                           TCHAR cDefault)
{
    return MnemonicSettingGet(eScope, dItemId, cDefault);
}

void MenuMnemonicValueSet(MENU_MNEMONIC_SCOPE eScope, UINT dItemId,
                          TCHAR cMnemonic)
{
    MnemonicSettingPut(eScope, dItemId, cMnemonic);
}

TCHAR MenuMnemonicDefaultGet(MENU_MNEMONIC_SCOPE eScope, UINT dItemId)
{
    if (MENU_MNEMONIC_OTHER == eScope)
    {
        if (60101 == dItemId) return TEXT('M');
        if (60102 == dItemId) return TEXT('R');
        if (60103 == dItemId) return TEXT('X');
    }
    TCHAR cDefault = MnemonicDefaultCharGet(dItemId);
    for (INT i = 0; giSubmenuMnemonicCount > i; i++)
    {
        if (gstSubmenuMnemonicTable[i].dItemId == dItemId)
        {
            cDefault = gstSubmenuMnemonicTable[i].cMnemonic;
            break;
        }
    }
    if (MENU_MNEMONIC_DOCUMENT_TAB == eScope && IDM_FILE_CLOSE == dItemId)
        return TEXT('Q');
    return cDefault;
}

void MenuMnemonicTextBuild(LPCTSTR ptLabel, MENU_MNEMONIC_SCOPE eScope,
                           UINT dItemId, TCHAR cDefault, LPTSTR ptText,
                           size_t cchText)
{
    StringCchCopy(ptText, cchText, ptLabel ? ptLabel : TEXT(""));
    const TCHAR cMnemonic =
        MnemonicSettingGet(eScope, dItemId, cDefault);
    if (cMnemonic)
    {
        TCHAR atSuffix[8];
        StringCchPrintf(atSuffix, 8, TEXT("(&%c)"), cMnemonic);
        StringCchCat(ptText, cchText, atSuffix);
    }
}
