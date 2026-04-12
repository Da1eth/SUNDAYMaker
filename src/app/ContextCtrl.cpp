#include "Sunday.h"

VOID CntxEditBuild(VOID);
INT_PTR CALLBACK CntxEditDlgProc(HWND, UINT, WPARAM, LPARAM);
VOID CntxDlgLvInit(HWND);
VOID CntxDlgAllListUp(HWND);
VOID CntxDlgBuildListUp(HWND);
VOID CntxDlgItemAdd(HWND);
VOID CntxDlgItemDel(HWND);
VOID CntxDlgItemSpinUp(HWND);
VOID CntxDlgItemSpinDown(HWND);

namespace
{
struct ContextMenuState
{
    HINSTANCE hInstance{};
    array<TCHAR, MAX_PATH> atIniPath{};
    HMENU hPopupMenu{};
    vector<APP_COMMAND_ITEM> vcItems;
    vector<APP_COMMAND_ITEM> vcEditItems;

    void ResetPopupMenu()
    {
        if (hPopupMenu)
        {
            DestroyMenu(hPopupMenu);
            hPopupMenu = nullptr;
        }
    }

    void Clear()
    {
        ResetPopupMenu();
        vcItems.clear();
        vcEditItems.clear();
        hInstance = nullptr;
        atIniPath[0] = 0;
    }
};

constexpr UINT gadDefItem[] = {
    IDM_CUT,
    IDM_COPY,
    IDM_PASTE,
    IDM_ALLSEL,
    0,
    IDM_SJISCOPY_ALL,
    0,
    IDM_SQSELECT,
    0,
    IDM_LAYERBOX,
    IDM_FRMINSBOX_OPEN,
    0,
    IDM_RIGHT_GUIDE_SET,
    IDM_INS_TOPSPACE,
    IDM_DEL_TOPSPACE,
    IDM_DEL_LASTSPACE,
    IDM_DEL_LASTLETTER,
    IDM_FILL_SPACE,
    0,
    IDM_INCR_DOT_LINES,
    IDM_DECR_DOT_LINES,
    0,
    IDM_SPACE_VIEW_TOGGLE,
    IDM_GRID_VIEW_TOGGLE,
    IDM_RIGHT_RULER_TOGGLE,
    0,
};

ContextMenuState gstContextMenuState;

LPCTSTR ContextItemTextGet(const APP_COMMAND_ITEM &stItem)
{
    return AppCommandLabelGet(stItem.dCommandId);
}

void ContextCommandTextBuild(UINT dCommandId, LPTSTR ptText, size_t cchText)
{
    AppCommandDisplayNameCopy(dCommandId, ptText, cchText);
}

void ContextSubmenuAppendCommands(HMENU hSubMenu, const UINT *pdCommands,
                                  UINT dCount)
{
    for (UINT i = 0; dCount > i; i++)
    {
        TCHAR atText[MAX_STRING];

        ContextCommandTextBuild(pdCommands[i], atText, MAX_STRING);
        AppendMenu(hSubMenu, MF_STRING, pdCommands[i], atText);
    }
}

void ContextPopupAppendItem(HMENU hPopupMenu, const APP_COMMAND_ITEM &stItem)
{
    TCHAR atText[MAX_STRING];
    HMENU hSubMenu;
    const UINT *pdCommands = nullptr;
    UINT dCount = 0;

    if (0 == stItem.dCommandId)
    {
        AppendMenu(hPopupMenu, MF_SEPARATOR, 0, nullptr);
        return;
    }

    ContextCommandTextBuild(stItem.dCommandId, atText, MAX_STRING);

    if (AppCommandGetSubmenuCommands(stItem.dCommandId, &pdCommands, &dCount))
    {
        hSubMenu = CreatePopupMenu();
        ContextSubmenuAppendCommands(hSubMenu, pdCommands, dCount);
        AppendMenu(hPopupMenu, MF_POPUP, (UINT_PTR)hSubMenu, atText);
        return;
    }

    switch (stItem.dCommandId)
    {
    case IDM_MN_INSFRAME_SEL:
        if (FAILED(MenuPickerCreatePagedMenu(&hSubMenu, MENU_PICKER_FRAME,
                                             MENU_PICKER_MENU_GROUP_MAX)))
        {
            AppendMenu(hPopupMenu, MF_GRAYED | MF_STRING, 0, atText);
            return;
        }

        AppendMenu(hPopupMenu, MF_POPUP, (UINT_PTR)hSubMenu, atText);
        return;

    case IDM_MN_USER_REFS:
        if (FAILED(MenuPickerCreatePagedMenu(&hSubMenu, MENU_PICKER_USERITEM,
                                             MENU_PICKER_MENU_GROUP_MAX)))
        {
            AppendMenu(hPopupMenu, MF_GRAYED | MF_STRING, 0, atText);
            return;
        }

        AppendMenu(hPopupMenu, MF_POPUP, (UINT_PTR)hSubMenu, atText);
        return;

    default:
        AppendMenu(hPopupMenu, MF_STRING, stItem.dCommandId, atText);
        return;
    }
}

void ContextItemsLoadDefault(vector<APP_COMMAND_ITEM> *pItems)
{
    pItems->clear();

    for (UINT dCommandId : gadDefItem)
    {
        const APP_COMMAND_ITEM *pstItem = AppCommandFind(dCommandId);

        if (nullptr != pstItem)
            pItems->push_back(*pstItem);
    }
}

void ContextItemsLoadFromSettings(vector<APP_COMMAND_ITEM> *pItems)
{
    const LPCTSTR ptIniPath = gstContextMenuState.atIniPath.data();
    const UINT dCount =
        GetPrivateProfileInt(TEXT("Context"), TEXT("Count"), 0, ptIniPath);

    pItems->clear();
    if (1 > dCount)
    {
        ContextItemsLoadDefault(pItems);
        return;
    }

    for (UINT i = 0; dCount > i; i++)
    {
        TCHAR atKeyName[MIN_STRING];
        const APP_COMMAND_ITEM *pstItem;

        StringCchPrintf(atKeyName, MIN_STRING, TEXT("CmdID%u"), i);
        pstItem = AppCommandFind(
            GetPrivateProfileInt(TEXT("Context"), atKeyName, 0, ptIniPath));
        if (nullptr != pstItem)
            pItems->push_back(*pstItem);
    }

    if (pItems->empty())
        ContextItemsLoadDefault(pItems);
}

HRESULT ContextItemsSave(const vector<APP_COMMAND_ITEM> &vcItems)
{
    TCHAR atKeyName[MIN_STRING];
    TCHAR atBuff[MIN_STRING];
    const LPCTSTR ptIniPath = gstContextMenuState.atIniPath.data();

    ZeroMemory(atBuff, sizeof(atBuff));
    WritePrivateProfileSection(TEXT("Context"), atBuff, ptIniPath);

    for (size_t i = 0; vcItems.size() > i; i++)
    {
        StringCchPrintf(atKeyName, MIN_STRING, TEXT("CmdID%u"), (UINT)i);
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), vcItems[i].dCommandId);
        WritePrivateProfileString(TEXT("Context"), atKeyName, atBuff, ptIniPath);
    }

    StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), (UINT)vcItems.size());
    WritePrivateProfileString(TEXT("Context"), TEXT("Count"), atBuff, ptIniPath);

    return S_OK;
}

void ContextListViewInitSingle(HWND hListWnd, LPCTSTR ptTitle)
{
    RECT rect;
    LVCOLUMN stColumn;

    GetClientRect(hListWnd, &rect);

    ListView_SetExtendedListViewStyle(
        hListWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

    while (0 < Header_GetItemCount(ListView_GetHeader(hListWnd)))
    {
        ListView_DeleteColumn(hListWnd, 0);
    }

    ZeroMemory(&stColumn, sizeof(LVCOLUMN));
    stColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    stColumn.fmt = LVCFMT_LEFT;
    stColumn.iSubItem = 0;
    stColumn.pszText = const_cast<LPTSTR>(ptTitle);
    stColumn.cx = rect.right - 23;
    ListView_InsertColumn(hListWnd, 0, &stColumn);
}

INT ContextSelectedIndexGet(HWND hListWnd)
{
    return ListView_GetNextItem(hListWnd, -1, LVNI_ALL | LVNI_SELECTED);
}

void ContextSelectedIndexSet(HWND hListWnd, INT iIndex)
{
    if (0 > iIndex)
        return;

    ListView_SetItemState(hListWnd, iIndex, LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hListWnd, iIndex, FALSE);
}
}

UINT CntxItemCountGet(VOID)
{
    return AppCommandCount();
}

UINT CntxItemCommandGet(UINT dIndex)
{
    const APP_COMMAND_ITEM *pstItem = AppCommandAt(dIndex);
    return pstItem ? pstItem->dCommandId : 0;
}

LPCTSTR CntxItemLabelGet(UINT dIndex)
{
    const APP_COMMAND_ITEM *pstItem = AppCommandAt(dIndex);
    return pstItem ? ContextItemTextGet(*pstItem) : TEXT("");
}

HRESULT CntxCommandNameCopy(UINT dCommandID, LPTSTR ptText, size_t cchText)
{
    return AppCommandNameCopy(dCommandID, ptText, cchText);
}

HINSTANCE CntxInstanceGet(VOID)
{
    return gstContextMenuState.hInstance;
}

LPCTSTR CntxIniPathGet(VOID)
{
    return gstContextMenuState.atIniPath.data();
}

HRESULT CntxEditInitialise(LPTSTR ptCurrent, HINSTANCE hInstance)
{
    if (!(ptCurrent) || !(hInstance))
    {
        gstContextMenuState.Clear();
        return S_OK;
    }

    gstContextMenuState.hInstance = hInstance;
    StringCchCopy(gstContextMenuState.atIniPath.data(), MAX_PATH, ptCurrent);
    PathAppend(gstContextMenuState.atIniPath.data(), SETTINGS_CONTEXT_MENU_INI_FILE);

    ContextItemsLoadFromSettings(&(gstContextMenuState.vcItems));
    CntxEditBuild();

    return S_OK;
}

HMENU CntxMenuGet(VOID)
{
    return gstContextMenuState.hPopupMenu;
}

VOID CntxEditBuild(VOID)
{
    gstContextMenuState.ResetPopupMenu();
    gstContextMenuState.hPopupMenu = CreatePopupMenu();

    for (const APP_COMMAND_ITEM &stItem : gstContextMenuState.vcItems)
    {
        ContextPopupAppendItem(gstContextMenuState.hPopupMenu, stItem);
    }
}

HRESULT CntxEditDlgOpen(HWND hWnd)
{
    const INT_PTR iRslt = DialogBoxParam(
        gstContextMenuState.hInstance, MAKEINTRESOURCE(IDD_CONTEXT_ITEM_DLG),
        hWnd, CntxEditDlgProc, 0);

    if (IDOK != iRslt)
        return E_ABORT;

    gstContextMenuState.vcItems = gstContextMenuState.vcEditItems;
    ContextItemsSave(gstContextMenuState.vcItems);
    CntxEditBuild();

    return S_OK;
}

INT_PTR CALLBACK CntxEditDlgProc(HWND hDlg, UINT message, WPARAM wParam,
                                 LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        gstContextMenuState.vcEditItems = gstContextMenuState.vcItems;
        CntxDlgLvInit(hDlg);
        CntxDlgAllListUp(hDlg);
        CntxDlgBuildListUp(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;

        case IDB_MENUITEM_ADD:
            CntxDlgItemAdd(hDlg);
            return (INT_PTR)TRUE;

        case IDB_MENUITEM_DEL:
            CntxDlgItemDel(hDlg);
            return (INT_PTR)TRUE;

        case IDB_MENUITEM_SPINUP:
            CntxDlgItemSpinUp(hDlg);
            return (INT_PTR)TRUE;

        case IDB_MENUITEM_SPINDOWN:
            CntxDlgItemSpinDown(hDlg);
            return (INT_PTR)TRUE;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return (INT_PTR)FALSE;
}

VOID CntxDlgLvInit(HWND hDlg)
{
    ContextListViewInitSingle(GetDlgItem(hDlg, IDLV_MENU_ALLITEM), TEXT("메뉴 항목"));
    ContextListViewInitSingle(GetDlgItem(hDlg, IDLV_MENU_BUILDX), TEXT("구성된 메뉴"));
}

VOID CntxDlgAllListUp(HWND hDlg)
{
    HWND hLvWnd;
    LVITEM stItem;
    TCHAR atText[SUB_STRING];

    hLvWnd = GetDlgItem(hDlg, IDLV_MENU_ALLITEM);
    ListView_DeleteAllItems(hLvWnd);

    ZeroMemory(&stItem, sizeof(LVITEM));
    stItem.mask = LVIF_TEXT;
    stItem.pszText = atText;

    for (UINT i = 0; AppCommandCount() > i; i++)
    {
        const APP_COMMAND_ITEM *pstItem = AppCommandAt(i);

        if (!(pstItem))
            continue;

        AppCommandDisplayNameCopy(pstItem->dCommandId, atText, SUB_STRING);
        if (AppCommandHasChildMenu(pstItem->dCommandId))
            StringCchCat(atText, SUB_STRING, TEXT(" (하위 메뉴 포함)"));

        stItem.iItem = i;
        ListView_InsertItem(hLvWnd, &stItem);
    }
}

VOID CntxDlgBuildListUp(HWND hDlg)
{
    HWND hLvWnd;
    LVITEM stItem;
    TCHAR atText[SUB_STRING];

    hLvWnd = GetDlgItem(hDlg, IDLV_MENU_BUILDX);
    ListView_DeleteAllItems(hLvWnd);

    ZeroMemory(&stItem, sizeof(LVITEM));
    stItem.mask = LVIF_TEXT;
    stItem.pszText = atText;

    for (size_t i = 0; gstContextMenuState.vcEditItems.size() > i; i++)
    {
        const APP_COMMAND_ITEM &stContextItem = gstContextMenuState.vcEditItems[i];

        if (0 == stContextItem.dCommandId)
        {
            StringCchCopy(atText, SUB_STRING, TEXT("---------------"));
        }
        else
        {
            AppCommandDisplayNameCopy(stContextItem.dCommandId, atText,
                                      SUB_STRING);
            if (AppCommandHasChildMenu(stContextItem.dCommandId))
                StringCchCat(atText, SUB_STRING, TEXT("　　[＞"));
        }

        stItem.iItem = (INT)i;
        ListView_InsertItem(hLvWnd, &stItem);
    }
}

VOID CntxDlgItemAdd(HWND hDlg)
{
    HWND hListWnd;
    HWND hBuildWnd;
    INT iSourceIndex;
    INT iInsertIndex;
    const APP_COMMAND_ITEM *pstItem;

    hListWnd = GetDlgItem(hDlg, IDLV_MENU_ALLITEM);
    hBuildWnd = GetDlgItem(hDlg, IDLV_MENU_BUILDX);

    iSourceIndex = ContextSelectedIndexGet(hListWnd);
    if (0 > iSourceIndex)
        return;

    pstItem = AppCommandAt((UINT)iSourceIndex);
    if (!(pstItem))
        return;

    iInsertIndex = ContextSelectedIndexGet(hBuildWnd);
    if (0 > iInsertIndex ||
        (size_t)(iInsertIndex + 1) >= gstContextMenuState.vcEditItems.size())
    {
        gstContextMenuState.vcEditItems.push_back(*pstItem);
        iInsertIndex = (INT)gstContextMenuState.vcEditItems.size() - 1;
    }
    else
    {
        gstContextMenuState.vcEditItems.insert(
            gstContextMenuState.vcEditItems.begin() + iInsertIndex + 1, *pstItem);
        iInsertIndex++;
    }

    CntxDlgBuildListUp(hDlg);
    ContextSelectedIndexSet(hBuildWnd, iInsertIndex);
}

VOID CntxDlgItemDel(HWND hDlg)
{
    HWND hBuildWnd;
    INT iIndex;

    hBuildWnd = GetDlgItem(hDlg, IDLV_MENU_BUILDX);
    iIndex = ContextSelectedIndexGet(hBuildWnd);
    if (0 > iIndex)
        return;

    gstContextMenuState.vcEditItems.erase(
        gstContextMenuState.vcEditItems.begin() + iIndex);

    CntxDlgBuildListUp(hDlg);
    if (!(gstContextMenuState.vcEditItems.empty()))
    {
        if ((INT)gstContextMenuState.vcEditItems.size() <= iIndex)
            iIndex = (INT)gstContextMenuState.vcEditItems.size() - 1;
        ContextSelectedIndexSet(hBuildWnd, iIndex);
    }
}

VOID CntxDlgItemSpinUp(HWND hDlg)
{
    HWND hBuildWnd;
    INT iIndex;

    hBuildWnd = GetDlgItem(hDlg, IDLV_MENU_BUILDX);
    iIndex = ContextSelectedIndexGet(hBuildWnd);
    if (0 >= iIndex)
        return;

    iter_swap(gstContextMenuState.vcEditItems.begin() + iIndex,
              gstContextMenuState.vcEditItems.begin() + iIndex - 1);

    CntxDlgBuildListUp(hDlg);
    ContextSelectedIndexSet(hBuildWnd, iIndex - 1);
}

VOID CntxDlgItemSpinDown(HWND hDlg)
{
    HWND hBuildWnd;
    INT iIndex;

    hBuildWnd = GetDlgItem(hDlg, IDLV_MENU_BUILDX);
    iIndex = ContextSelectedIndexGet(hBuildWnd);
    if (0 > iIndex ||
        (size_t)(iIndex + 1) >= gstContextMenuState.vcEditItems.size())
    {
        return;
    }

    iter_swap(gstContextMenuState.vcEditItems.begin() + iIndex,
              gstContextMenuState.vcEditItems.begin() + iIndex + 1);

    CntxDlgBuildListUp(hDlg);
    ContextSelectedIndexSet(hBuildWnd, iIndex + 1);
}