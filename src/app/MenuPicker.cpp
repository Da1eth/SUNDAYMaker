
#include "Sunday.h"

#define MENU_PICKER_DYNAMIC_FIRST 50000
#define MENU_PICKER_PAGE_UP 49998
#define MENU_PICKER_PAGE_DOWN 49999

static UINT MenuPickerMenuCommandIdGet(UINT dMode, UINT dIndex)
{
    switch (dMode)
    {
    case MENU_PICKER_FRAME:
        return MENU_DYNAMIC_FRAME_FIRST + dIndex;

    case MENU_PICKER_USERITEM:
        return MENU_DYNAMIC_USER_FIRST + dIndex;

    default:
        return 0;
    }
}

static UINT MenuPickerItemCountGet(UINT dMode)
{
    switch (dMode)
    {
    case MENU_PICKER_FRAME:
        return FrameCountGet();
    case MENU_PICKER_USERITEM:
        return UserItemCountGet();
    default:
        return 0;
    }
}

static HRESULT MenuPickerItemTextGet(UINT dMode, UINT dIndex, LPTSTR ptText, UINT_PTR cchText)
{
    if (MENU_PICKER_FRAME == dMode)
    {
        return FrameNameLoad(dIndex, ptText, cchText);
    }

    if (MENU_PICKER_USERITEM == dMode)
    {
        return UserItemNameGet(dIndex, ptText, cchText);
    }

    return E_INVALIDARG;
}

static VOID MenuPickerCommitSelection(HWND hWnd, UINT dMode, UINT dIndex)
{
    switch (dMode)
    {
    case MENU_PICKER_FRAME:
        ViewFrameInsert(dIndex);
        break;

    case MENU_PICKER_USERITEM:
        UserItemInsert(hWnd, dIndex);
        break;

    default:
        break;
    }
}

static UINT MenuPickerPopupBuild(HMENU hPopupMenu, UINT dMode, UINT dStartIndex)
{
    UINT dCount, dEndIndex, i;
    TCHAR atText[MAX_STRING];

    dCount = MenuPickerItemCountGet(dMode);
    if (0 == dCount)
    {
        AppendMenu(hPopupMenu, MF_GRAYED | MF_STRING, 0, TEXT("(항목 없음)"));
        return 0;
    }

    if (0 < dStartIndex)
    {
        AppendMenu(hPopupMenu, MF_STRING, MENU_PICKER_PAGE_UP, TEXT("←"));
        AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
    }

    dEndIndex = dStartIndex + MENU_PICKER_VISIBLE_MAX;
    if (dCount < dEndIndex)
        dEndIndex = dCount;

    for (i = dStartIndex; dEndIndex > i; i++)
    {
        ZeroMemory(atText, sizeof(atText));
        if (FAILED(MenuPickerItemTextGet(dMode, i, atText, MAX_STRING)))
        {
            continue;
        }
        AppendMenu(hPopupMenu, MF_STRING, MENU_PICKER_DYNAMIC_FIRST + i, atText);
    }

    if (dCount > dEndIndex)
    {
        AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(hPopupMenu, MF_STRING, MENU_PICKER_PAGE_DOWN, TEXT("→"));
    }

    return dEndIndex - dStartIndex;
}

static HRESULT MenuPickerAppendItems(HMENU hMenu, UINT dMode, UINT dStartIndex, UINT dEndIndex)
{
    UINT i;
    TCHAR atText[MAX_STRING];

    if (!(hMenu))
        return E_INVALIDARG;

    for (i = dStartIndex; dEndIndex > i; i++)
    {
        ZeroMemory(atText, sizeof(atText));
        if (FAILED(MenuPickerItemTextGet(dMode, i, atText, MAX_STRING)))
        {
            continue;
        }

        AppendMenu(hMenu, MF_STRING, MenuPickerMenuCommandIdGet(dMode, i), atText);
    }

    return S_OK;
}

HRESULT MenuPickerCreatePagedMenu(HMENU *phMenu, UINT dMode, UINT dGroupSize)
{
    HMENU hMenu;
    HMENU hPageMenu;
    UINT dCount;
    UINT dStartIndex;
    UINT dEndIndex;
    TCHAR atLabel[MIN_STRING];

    if (!(phMenu))
        return E_INVALIDARG;

    if (0 == dGroupSize)
        return E_INVALIDARG;

    *phMenu = nullptr;

    hMenu = CreatePopupMenu();
    if (!(hMenu))
        return E_OUTOFMEMORY;

    dCount = MenuPickerItemCountGet(dMode);
    if (0 == dCount)
    {
        AppendMenu(hMenu, MF_GRAYED | MF_STRING, 0, TEXT("(항목 없음)"));
        *phMenu = hMenu;
        return S_OK;
    }

    if (dGroupSize >= dCount)
    {
        MenuPickerAppendItems(hMenu, dMode, 0, dCount);
        *phMenu = hMenu;
        return S_OK;
    }

    for (dStartIndex = 0; dCount > dStartIndex; dStartIndex += dGroupSize)
    {
        dEndIndex = dStartIndex + dGroupSize;
        if (dCount < dEndIndex)
            dEndIndex = dCount;

        hPageMenu = CreatePopupMenu();
        if (!(hPageMenu))
        {
            DestroyMenu(hMenu);
            return E_OUTOFMEMORY;
        }

        MenuPickerAppendItems(hPageMenu, dMode, dStartIndex, dEndIndex);
        StringCchPrintf(atLabel, MIN_STRING, TEXT("%u - %u"), dStartIndex + 1, dEndIndex);
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hPageMenu, atLabel);
    }

    *phMenu = hMenu;
    return S_OK;
}

HRESULT MenuPickerInitialise(HINSTANCE hInstance)
{
    UNREFERENCED_PARAMETER(hInstance);
    return S_OK;
}

HRESULT MenuPickerShow(HWND hWnd, UINT dMode, INT xPos, INT yPos)
{
    HMENU hPopupMenu;
    UINT dCount;
    UINT dMaxStartIndex;
    UINT dStartIndex;
    UINT dCommand;

    dStartIndex = 0;

    do
    {
        dCount = MenuPickerItemCountGet(dMode);
        dMaxStartIndex = 0;
        if (MENU_PICKER_VISIBLE_MAX < dCount)
        {
            dMaxStartIndex = ((dCount - 1) / MENU_PICKER_VISIBLE_MAX) * MENU_PICKER_VISIBLE_MAX;
        }

        hPopupMenu = CreatePopupMenu();
        MenuPickerPopupBuild(hPopupMenu, dMode, dStartIndex);
        dCommand = TrackPopupMenuEx(hPopupMenu,
                                    TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_VERTICAL,
                                    xPos, yPos, hWnd, nullptr);
        DestroyMenu(hPopupMenu);

        if (MENU_PICKER_PAGE_UP == dCommand)
        {
            dStartIndex -= min(dStartIndex, (UINT)MENU_PICKER_VISIBLE_MAX);
            continue;
        }
        if (MENU_PICKER_PAGE_DOWN == dCommand)
        {
            dStartIndex += MENU_PICKER_VISIBLE_MAX;
            if (dMaxStartIndex < dStartIndex)
                dStartIndex = dMaxStartIndex;
            continue;
        }
        if (MENU_PICKER_DYNAMIC_FIRST <= dCommand)
        {
            MenuPickerCommitSelection(hWnd, dMode, dCommand - MENU_PICKER_DYNAMIC_FIRST);
        }

        break;
    } while (TRUE);

    return S_OK;
}
