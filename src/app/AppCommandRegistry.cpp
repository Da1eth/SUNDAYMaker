#include "Sunday.h"
#include "UiText.h"

namespace
{
constexpr UINT APP_COMMAND_FRAME_LIMIT = 20;
constexpr UINT APP_COMMAND_USER_ITEM_LIMIT = 16;

template <size_t N>
constexpr UINT AppCommandArrayCount(const APP_COMMAND_ITEM (&)[N])
{
    return static_cast<UINT>(N);
}

template <size_t N>
const APP_COMMAND_ITEM *AppCommandFindInArray(const APP_COMMAND_ITEM (&stItems)[N],
                                              UINT dCommandId)
{
    for (const APP_COMMAND_ITEM &stItem : stItems)
    {
        if (stItem.dCommandId == dCommandId)
            return &stItem;
    }

    return nullptr;
}

UINT AppCommandFrameIdAt(UINT dIndex)
{
    return IDM_INSFRAME_ALPHA + dIndex;
}

UINT AppCommandUserItemIdAt(UINT dIndex)
{
    return IDM_USER_ITEM_ALPHA + dIndex;
}

const APP_COMMAND_ITEM *AppCommandDynamicItem(UINT dCommandId)
{
    static APP_COMMAND_ITEM stDynamicItem{};

    stDynamicItem.dCommandId = dCommandId;
    stDynamicItem.ptFallbackLabel = nullptr;

    return &stDynamicItem;
}

HRESULT AppCommandDefaultNameCopy(UINT dCommandId, LPTSTR ptText, size_t cchText)
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

HRESULT AppCommandDynamicNameCopy(UINT dCommandId, LPTSTR ptText, size_t cchText)
{
    TCHAR atName[SUB_STRING];
    const INT iFrameIndex = AppCommandFrameIndex(dCommandId);
    const INT iUserItemIndex = AppCommandUserItemIndex(dCommandId);

    if (0 <= iFrameIndex)
    {
        ZeroMemory(atName, sizeof(atName));
        if (SUCCEEDED(FrameNameLoad((UINT)iFrameIndex, atName, SUB_STRING)) &&
            0 != atName[0])
        {
            StringCchPrintf(ptText, cchText, TEXT("말풍선 : %s"), atName);
        }
        else
        {
            StringCchCopy(ptText, cchText, TEXT("말풍선 : (미등록)"));
        }

        return S_OK;
    }

    if (0 <= iUserItemIndex)
    {
        ZeroMemory(atName, sizeof(atName));
        if (SUCCEEDED(UserItemNameGet((UINT)iUserItemIndex, atName, SUB_STRING)) &&
            0 != atName[0])
        {
            StringCchPrintf(ptText, cchText, TEXT("유저 아이템 : %s"), atName);
        }
        else
        {
            StringCchCopy(ptText, cchText, TEXT("유저 아이템 : (미등록)"));
        }

        return S_OK;
    }

    return E_FAIL;
}

constexpr APP_COMMAND_ITEM gatCommandItemsBeforeFrame[] = {
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
};

constexpr APP_COMMAND_ITEM gatCommandItemsBetweenFrameAndUser[] = {
    {IDM_MN_USER_REFS, nullptr},
};

constexpr APP_COMMAND_ITEM gatCommandItemsAfterUser[] = {
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

constexpr UINT AppCommandFixedItemCountInternal()
{
    return AppCommandArrayCount(gatCommandItemsBeforeFrame) +
           AppCommandArrayCount(gatCommandItemsBetweenFrameAndUser) +
           AppCommandArrayCount(gatCommandItemsAfterUser);
}

constexpr UINT AppCommandItemCountInternal()
{
    return AppCommandFixedItemCountInternal() + APP_COMMAND_FRAME_LIMIT +
           APP_COMMAND_USER_ITEM_LIMIT;
}
}

UINT AppCommandCount(VOID)
{
    return AppCommandItemCountInternal();
}

const APP_COMMAND_ITEM *AppCommandAt(UINT dIndex)
{
    const UINT dBeforeFrameCount = AppCommandArrayCount(gatCommandItemsBeforeFrame);
    const UINT dBetweenCount = AppCommandArrayCount(gatCommandItemsBetweenFrameAndUser);

    if (AppCommandItemCountInternal() <= dIndex)
        return nullptr;

    if (dBeforeFrameCount > dIndex)
        return &(gatCommandItemsBeforeFrame[dIndex]);

    dIndex -= dBeforeFrameCount;
    if (APP_COMMAND_FRAME_LIMIT > dIndex)
        return AppCommandDynamicItem(AppCommandFrameIdAt(dIndex));

    dIndex -= APP_COMMAND_FRAME_LIMIT;
    if (dBetweenCount > dIndex)
        return &(gatCommandItemsBetweenFrameAndUser[dIndex]);

    dIndex -= dBetweenCount;
    if (APP_COMMAND_USER_ITEM_LIMIT > dIndex)
        return AppCommandDynamicItem(AppCommandUserItemIdAt(dIndex));

    return &(gatCommandItemsAfterUser[dIndex - APP_COMMAND_USER_ITEM_LIMIT]);
}

const APP_COMMAND_ITEM *AppCommandFind(UINT dCommandId)
{
    if (IDM_INSFRAME_ALPHA <= dCommandId && dCommandId <= IDM_INSFRAME_TANGO)
        return AppCommandDynamicItem(dCommandId);

    if (IDM_USER_ITEM_ALPHA <= dCommandId && dCommandId <= IDM_USER_ITEM_PAPA)
        return AppCommandDynamicItem(dCommandId);

    if (const APP_COMMAND_ITEM *pstItem =
            AppCommandFindInArray(gatCommandItemsBeforeFrame, dCommandId))
        return pstItem;

    if (const APP_COMMAND_ITEM *pstItem =
            AppCommandFindInArray(gatCommandItemsBetweenFrameAndUser, dCommandId))
        return pstItem;

    if (const APP_COMMAND_ITEM *pstItem =
            AppCommandFindInArray(gatCommandItemsAfterUser, dCommandId))
        return pstItem;

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
    if (SUCCEEDED(AppCommandDynamicNameCopy(dCommandId, ptText, cchText)))
        return S_OK;

    return AppCommandDefaultNameCopy(dCommandId, ptText, cchText);
}

HRESULT AppCommandDisplayNameCopy(UINT dCommandId, LPTSTR ptText, size_t cchText)
{
    if (SUCCEEDED(AppCommandDynamicNameCopy(dCommandId, ptText, cchText)))
        return S_OK;

    return AppCommandDefaultNameCopy(dCommandId, ptText, cchText);
}

BOOLEAN AppCommandAllowedInAccel(UINT dCommandId)
{
    if (IDM_INSFRAME_ALPHA <= dCommandId && dCommandId <= IDM_INSFRAME_TANGO)
        return (dCommandId - IDM_INSFRAME_ALPHA) < APP_COMMAND_FRAME_LIMIT;

    if (IDM_USER_ITEM_ALPHA <= dCommandId && dCommandId <= IDM_USER_ITEM_PAPA)
        return (dCommandId - IDM_USER_ITEM_ALPHA) < APP_COMMAND_USER_ITEM_LIMIT;

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