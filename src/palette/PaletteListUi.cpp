#include "Palette.h"

VOID PaletteListComboReload(HWND hComboWnd, const vector<JSON_TEMPLATE_GROUP> &vcGroups)
{
    ComboBox_ResetContent(hComboWnd);

    for (size_t i = 0; vcGroups.size() > i; i++)
    {
        ComboBox_AddString(hComboWnd, vcGroups[i].wsName.c_str());
    }
}

VOID PaletteListResizeColumns(HWND hLvWnd, UINT dColumnCount)
{
    RECT rect;
    INT width;

    if (!(hLvWnd) || 0 == dColumnCount)
        return;

    GetClientRect(hLvWnd, &rect);
    width = rect.right / (INT)dColumnCount;
    for (UINT i = 0; dColumnCount > i; i++)
    {
        ListView_SetColumnWidth(hLvWnd, i, width);
    }
}

HRESULT PaletteListPopulate(HWND hLvWnd, const vector<wstring> &vcItems, UINT dColumnCount)
{
    LVITEM stLvi;
    TCHAR atItem[SUB_STRING];

    if (!(hLvWnd) || 0 == dColumnCount)
        return E_INVALIDARG;

    ListView_DeleteAllItems(hLvWnd);

    ZeroMemory(atItem, sizeof(atItem));
    stLvi = {};
    stLvi.mask = LVIF_TEXT;
    stLvi.pszText = atItem;

    for (size_t i = 0; vcItems.size() > i; i++)
    {
        StringCchCopy(atItem, SUB_STRING, vcItems[i].c_str());

        stLvi.iItem = (INT)(i / dColumnCount);
        stLvi.iSubItem = (INT)(i % dColumnCount);
        if (0 == stLvi.iSubItem)
            ListView_InsertItem(hLvWnd, &stLvi);
        else
            ListView_SetItem(hLvWnd, &stLvi);
    }

    PaletteListResizeColumns(hLvWnd, dColumnCount);
    return S_OK;
}

UINT PaletteListGridFluctuate(HWND hLvWnd, INT dFluct)
{
    INT clmCount, clmNew, i;
    LVCOLUMN stLvColm;

    if (0 == dFluct)
        return 0;

    clmCount = Header_GetItemCount(ListView_GetHeader(hLvWnd));

    if (0 > dFluct && 1 >= clmCount)
        return 0;

    clmNew = clmCount + dFluct;

    if (0 > dFluct)
    {
        for (i = clmCount; clmNew < i; i--)
        {
            ListView_DeleteColumn(hLvWnd, (i - 1));
        }
    }

    if (0 < dFluct)
    {
        ZeroMemory(&stLvColm, sizeof(LVCOLUMN));
        stLvColm.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        stLvColm.fmt = LVCFMT_LEFT;
        stLvColm.pszText = const_cast<LPTSTR>(TEXT("Item"));
        stLvColm.cx = 10;
        for (i = clmCount; clmNew > i; i++)
        {
            stLvColm.iSubItem = i;
            ListView_InsertColumn(hLvWnd, i, &stLvColm);
        }
    }

    return clmNew;
}

VOID PaletteListClearSelection(HWND hLvWnd)
{
    ListView_SetItemState(hLvWnd, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
}

BOOLEAN PaletteListHitTest(HWND hLvWnd, UINT dColumnCount, POINT stPoint, PALETTE_LIST_HIT *pstHit)
{
    LVHITTESTINFO stHitTestInfo;

    if (!(hLvWnd) || !(pstHit) || 0 == dColumnCount)
        return FALSE;

    ZeroMemory(pstHit, sizeof(PALETTE_LIST_HIT));
    pstHit->iItem = -1;
    pstHit->iSubItem = -1;
    pstHit->iIndex = -1;

    ZeroMemory(&stHitTestInfo, sizeof(LVHITTESTINFO));
    stHitTestInfo.pt = stPoint;
    ListView_SubItemHitTest(hLvWnd, &stHitTestInfo);

    pstHit->iItem = stHitTestInfo.iItem;
    pstHit->iSubItem = stHitTestInfo.iSubItem;
    if (0 > pstHit->iItem || 0 > pstHit->iSubItem)
        return FALSE;

    pstHit->iIndex = pstHit->iItem * (INT)dColumnCount + pstHit->iSubItem;
    return TRUE;
}

BOOLEAN PaletteListCursorHitTest(HWND hLvWnd, UINT dColumnCount, PALETTE_LIST_HIT *pstHit)
{
    POINT stPoint;

    GetCursorPos(&stPoint);
    ScreenToClient(hLvWnd, &stPoint);
    return PaletteListHitTest(hLvWnd, dColumnCount, stPoint, pstHit);
}

LPCTSTR PaletteListGroupItemGet(const vector<JSON_TEMPLATE_GROUP> &vcGroups, UINT dGroupIndex, INT iItemIndex)
{
    if (dGroupIndex >= vcGroups.size() || 0 > iItemIndex)
        return nullptr;

    const vector<wstring> &vcItems = vcGroups.at(dGroupIndex).vcItems;
    if (iItemIndex >= (INT)vcItems.size())
        return nullptr;

    return vcItems.at(iItemIndex).c_str();
}

BOOLEAN PaletteListBuildTooltipText(HWND hLvWnd, const vector<JSON_TEMPLATE_GROUP> &vcGroups, UINT dGroupIndex, UINT dColumnCount, LPTSTR ptBuffer, UINT cchBuffer)
{
    PALETTE_LIST_HIT stHit;
    LPCTSTR ptItem;
    INT iDot;

    if (!(ptBuffer) || 0 == cchBuffer)
        return FALSE;

    ptBuffer[0] = TEXT('\0');
    if (!(PaletteListCursorHitTest(hLvWnd, dColumnCount, &stHit)))
        return FALSE;

    ptItem = PaletteListGroupItemGet(vcGroups, dGroupIndex, stHit.iIndex);
    if (!(ptItem))
        return FALSE;

    iDot = ViewStringWidthGet(ptItem);
    StringCchPrintf(ptBuffer, cchBuffer, TEXT("%s [%d Dot]"), ptItem, iDot);
    return TRUE;
}

HRESULT PaletteListGroupsLoad(BOOLEAN bBrush, vector<JSON_TEMPLATE_GROUP> *pGroups)
{
    TCHAR atFileName[MAX_PATH];

    if (!(pGroups))
        return E_POINTER;

    pGroups->clear();

    StringCchCopy(atFileName, MAX_PATH, ExePathGet());
    PathAppend(atFileName, TEMPLATE_DIR);
    PathAppend(atFileName, AA_PALETTE_FILE);

    return AppLoadPaletteGroups(atFileName, bBrush, pGroups);
}