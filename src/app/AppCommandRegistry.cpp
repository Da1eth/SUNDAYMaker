#include "Sunday.h"
#include "UiText.h"

namespace
{
constexpr APP_COMMAND_ITEM gatCommandItems[] = {
    {0, TEXT("- 구분선 -")},
    {IDM_NEWFILE, nullptr},
    {IDM_OPEN, nullptr},
    {IDM_OVERWRITESAVE, nullptr},
    {IDM_RENAMESAVE, nullptr},
    {IDM_IMAGE_SAVE, nullptr},
    {IDM_GENERAL_OPTION, nullptr},
    {IDM_UNDO, nullptr},
    {IDM_REDO, nullptr},
    {IDM_CUT, nullptr},
    {IDM_COPY, nullptr},
    {IDM_SJISCOPY_ALL, nullptr},
    {IDM_PASTE, nullptr},
    {IDM_DELETE, nullptr},
    {IDM_ALLSEL, nullptr},
    {IDM_SQSELECT, nullptr},
    {IDM_SQUARE_PASTE, nullptr},
    {IDM_LAYERBOX, nullptr},
    {IDM_EXTRACTION_MODE, nullptr},
    {IDM_MN_UNISPACE, nullptr},
    {IDM_IN_01SPACE, nullptr},
    {IDM_IN_02SPACE, nullptr},
    {IDM_IN_03SPACE, nullptr},
    {IDM_IN_04SPACE, nullptr},
    {IDM_IN_05SPACE, nullptr},
    {IDM_IN_08SPACE, nullptr},
    {IDM_IN_10SPACE, nullptr},
    {IDM_IN_16SPACE, nullptr},
    {IDM_MN_COLOUR_SEL, nullptr},
    {IDM_INSTAG_SPO, nullptr},
    {IDM_INSTAG_COLOUR, nullptr},
    {IDM_INSTAG_GRADIENT, nullptr},
    {IDM_MN_INSFRAME_SEL, nullptr},
    {IDM_INSFRAME_EDIT, nullptr},
    {IDM_FRMINSBOX_OPEN, nullptr},
    {IDM_INSFRAME_ALPHA, TEXT("말풍선 1")},
    {IDM_INSFRAME_BRAVO, TEXT("말풍선 2")},
    {IDM_INSFRAME_CHARLIE, TEXT("말풍선 3")},
    {IDM_INSFRAME_DELTA, TEXT("말풍선 4")},
    {IDM_INSFRAME_ECHO, TEXT("말풍선 5")},
    {IDM_INSFRAME_FOXTROT, TEXT("말풍선 6")},
    {IDM_INSFRAME_GOLF, TEXT("말풍선 7")},
    {IDM_INSFRAME_HOTEL, TEXT("말풍선 8")},
    {IDM_INSFRAME_INDIA, TEXT("말풍선 9")},
    {IDM_INSFRAME_JULIETTE, TEXT("말풍선 10")},
    {IDM_INSFRAME_KILO, TEXT("말풍선 11")},
    {IDM_INSFRAME_LIMA, TEXT("말풍선 12")},
    {IDM_INSFRAME_MIKE, TEXT("말풍선 13")},
    {IDM_INSFRAME_NOVEMBER, TEXT("말풍선 14")},
    {IDM_INSFRAME_OSCAR, TEXT("말풍선 15")},
    {IDM_INSFRAME_PAPA, TEXT("말풍선 16")},
    {IDM_INSFRAME_QUEBEC, TEXT("말풍선 17")},
    {IDM_INSFRAME_ROMEO, TEXT("말풍선 18")},
    {IDM_INSFRAME_SIERRA, TEXT("말풍선 19")},
    {IDM_INSFRAME_TANGO, TEXT("말풍선 20")},
    {IDM_MN_USER_REFS, nullptr},
    {IDM_USER_ITEM_ALPHA, TEXT("유저 제작 템플릿 1")},
    {IDM_USER_ITEM_BRAVO, TEXT("유저 제작 템플릿 2")},
    {IDM_USER_ITEM_CHARLIE, TEXT("유저 제작 템플릿 3")},
    {IDM_USER_ITEM_DELTA, TEXT("유저 제작 템플릿 4")},
    {IDM_USER_ITEM_ECHO, TEXT("유저 제작 템플릿 5")},
    {IDM_USER_ITEM_FOXTROT, TEXT("유저 제작 템플릿 6")},
    {IDM_USER_ITEM_GOLF, TEXT("유저 제작 템플릿 7")},
    {IDM_USER_ITEM_HOTEL, TEXT("유저 제작 템플릿 8")},
    {IDM_USER_ITEM_INDIA, TEXT("유저 제작 템플릿 9")},
    {IDM_USER_ITEM_JULIETTE, TEXT("유저 제작 템플릿 10")},
    {IDM_USER_ITEM_KILO, TEXT("유저 제작 템플릿 11")},
    {IDM_USER_ITEM_LIMA, TEXT("유저 제작 템플릿 12")},
    {IDM_USER_ITEM_MIKE, TEXT("유저 제작 템플릿 13")},
    {IDM_USER_ITEM_NOVEMBER, TEXT("유저 제작 템플릿 14")},
    {IDM_USER_ITEM_OSCAR, TEXT("유저 제작 템플릿 15")},
    {IDM_USER_ITEM_PAPA, TEXT("유저 제작 템플릿 16")},
    {IDM_RIGHT_GUIDE_SET, nullptr},
    {IDM_INS_TOPSPACE, nullptr},
    {IDM_DEL_TOPSPACE, nullptr},
    {IDM_DEL_LASTSPACE, nullptr},
    {IDM_DEL_LASTLETTER, nullptr},
    {IDM_FILL_SPACE, nullptr},
    {IDM_FILL_ZENSP, nullptr},
    {IDM_HEADHALF_EXCHANGE, nullptr},
    {IDM_MIRROR_INVERSE, nullptr},
    {IDM_UPSET_INVERSE, nullptr},
    {IDM_INCREMENT_DOT, nullptr},
    {IDM_DECREMENT_DOT, nullptr},
    {IDM_INCR_DOT_LINES, nullptr},
    {IDM_DECR_DOT_LINES, nullptr},
    {IDM_DOT_SPLIT_LEFT, nullptr},
    {IDM_DOT_SPLIT_RIGHT, nullptr},
    {IDM_DOTDIFF_LOCK, nullptr},
    {IDM_DOTDIFF_ADJT, nullptr},
    {IDM_SPACE_VIEW_TOGGLE, nullptr},
    {IDM_GRID_VIEW_TOGGLE, nullptr},
    {IDM_RIGHT_RULER_TOGGLE, nullptr},
    {IDM_UNDER_RULER_TOGGLE, nullptr},
    {IDM_PAGELIST_VIEW, nullptr},
    {IDM_INSERT_PALETTE, nullptr},
    {IDM_BRUSH_PALETTE, nullptr},
    {IDM_TRACE_MODE_ON, nullptr},
    {IDM_ON_PREVIEW, nullptr},
    {IDM_PAGEL_DUPLICATE, nullptr},
    {IDM_PAGEL_DELETE, nullptr},
    {IDM_PAGEL_INSERT, nullptr},
    {IDM_PAGEL_ADD, nullptr},
    {IDM_PAGEL_DOWNSINK, nullptr},
    {IDM_PAGEL_UPFLOW, nullptr},
    {IDM_PAGEL_RENAME, nullptr},
    {IDM_TRC_VIEWTOGGLE, nullptr},
    {IDM_TMPLT_GROUP_PREV, nullptr},
    {IDM_TMPLT_GROUP_NEXT, nullptr},
    {IDM_WINDOW_CHANGE, nullptr},
    {IDM_WINDOW_CHG_RVRS, nullptr},
    {IDM_FILE_CLOSE, nullptr},
    {IDM_FILE_PREV, nullptr},
    {IDM_FILE_NEXT, nullptr},
    {IDM_PAGE_PREV, nullptr},
    {IDM_PAGE_NEXT, nullptr},
    {IDM_NOW_PAGE_REFRESH, nullptr},
    {IDM_FIND_DLG_OPEN, nullptr},
    {IDM_FIND_HIGHLIGHT_OFF, nullptr},
    {IDM_FIND_JUMP_NEXT, nullptr},
    {IDM_FIND_JUMP_PREV, nullptr},
    {IDM_FIND_TARGET_SET, nullptr},
};

constexpr UINT gadUniSpaceCommands[] = {
    IDM_IN_01SPACE,
    IDM_IN_02SPACE,
    IDM_IN_03SPACE,
    IDM_IN_04SPACE,
    IDM_IN_05SPACE,
    IDM_IN_08SPACE,
    IDM_IN_10SPACE,
    IDM_IN_16SPACE,
};

constexpr UINT gadColourCommands[] = {
    IDM_INSTAG_SPO,
    IDM_INSTAG_COLOUR,
    IDM_INSTAG_GRADIENT,
};

constexpr UINT AppCommandItemCountInternal()
{
    return static_cast<UINT>(sizeof(gatCommandItems) / sizeof(gatCommandItems[0]));
}
}

UINT AppCommandCount(VOID)
{
    return AppCommandItemCountInternal();
}

const APP_COMMAND_ITEM *AppCommandAt(UINT dIndex)
{
    if (AppCommandItemCountInternal() <= dIndex)
        return nullptr;

    return &(gatCommandItems[dIndex]);
}

const APP_COMMAND_ITEM *AppCommandFind(UINT dCommandId)
{
    for (const APP_COMMAND_ITEM &stItem : gatCommandItems)
    {
        if (stItem.dCommandId == dCommandId)
            return &stItem;
    }

    return nullptr;
}

LPCTSTR AppCommandLabelGet(UINT dCommandId)
{
    const APP_COMMAND_ITEM *pstItem;
    LPCTSTR ptLabel;

    ptLabel = UiTextGetLabel(dCommandId);
    if (nullptr != ptLabel)
        return ptLabel;

    pstItem = AppCommandFind(dCommandId);
    if (nullptr != pstItem && nullptr != pstItem->ptFallbackLabel)
        return pstItem->ptFallbackLabel;

    return TEXT("");
}

HRESULT AppCommandNameCopy(UINT dCommandId, LPTSTR ptText, size_t cchText)
{
    LPCTSTR ptSource;

    ptSource = AppCommandLabelGet(dCommandId);
    if (0 != ptSource[0])
    {
        StringCchCopy(ptText, cchText, ptSource);
        return S_OK;
    }

    StringCchCopy(ptText, cchText, TEXT("（이름 불명）"));
    return E_FAIL;
}

BOOLEAN AppCommandAllowedInAccel(UINT dCommandId)
{
    if (IDM_INSFRAME_ALPHA <= dCommandId && dCommandId <= IDM_INSFRAME_TANGO)
        return (dCommandId - IDM_INSFRAME_ALPHA) < 10;

    if (IDM_USER_ITEM_ALPHA <= dCommandId && dCommandId <= IDM_USER_ITEM_PAPA)
        return (dCommandId - IDM_USER_ITEM_ALPHA) < 10;

    return TRUE;
}

BOOLEAN AppCommandHasChildMenu(UINT dCommandId)
{
    switch (dCommandId)
    {
    case IDM_MN_UNISPACE:
    case IDM_MN_COLOUR_SEL:
    case IDM_MN_INSFRAME_SEL:
    case IDM_MN_USER_REFS:
        return TRUE;

    default:
        return FALSE;
    }
}

BOOLEAN AppCommandGetSubmenuCommands(UINT dCommandId, const UINT **ppCommands,
                                     UINT *pdCount)
{
    if (ppCommands)
        *ppCommands = nullptr;
    if (pdCount)
        *pdCount = 0;

    switch (dCommandId)
    {
    case IDM_MN_UNISPACE:
        if (ppCommands)
            *ppCommands = gadUniSpaceCommands;
        if (pdCount)
            *pdCount = static_cast<UINT>(sizeof(gadUniSpaceCommands) / sizeof(gadUniSpaceCommands[0]));
        return TRUE;

    case IDM_MN_COLOUR_SEL:
        if (ppCommands)
            *ppCommands = gadColourCommands;
        if (pdCount)
            *pdCount = static_cast<UINT>(sizeof(gadColourCommands) / sizeof(gadColourCommands[0]));
        return TRUE;

    default:
        return FALSE;
    }
}

INT AppCommandFrameIndex(UINT dCommandId)
{
    if (IDM_INSFRAME_ALPHA <= dCommandId && dCommandId <= IDM_INSFRAME_TANGO)
        return static_cast<INT>(dCommandId - IDM_INSFRAME_ALPHA);

    return -1;
}

INT AppCommandUserItemIndex(UINT dCommandId)
{
    if (IDM_USER_ITEM_ALPHA <= dCommandId && dCommandId <= IDM_USER_ITEM_PAPA)
        return static_cast<INT>(dCommandId - IDM_USER_ITEM_ALPHA);

    return -1;
}