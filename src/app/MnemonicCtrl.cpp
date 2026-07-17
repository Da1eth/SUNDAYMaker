#include "Sunday.h"
#include "AppUiContextInternal.h"
#include "MenuMnemonic.h"
#include "UiText.h"

VOID CntxEditBuild(VOID);

namespace
{
struct MNEMONIC_ROW
{
    wstring wsName;
    UINT dItemId{};
    UINT_PTR dParent{};
    MENU_MNEMONIC_SCOPE eScope{MENU_MNEMONIC_MAIN};
    TCHAR cMain{};
    TCHAR cContext{};
    BOOL bContext{};
    BOOL bContextPresent{};
    UINT_PTR dContextParent{};
    BOOL bHeader{};
};

vector<MNEMONIC_ROW> gvcMnemonicRows;
vector<MNEMONIC_ROW> gvcMnemonicMainRows;
vector<MNEMONIC_ROW> gvcMnemonicOtherRows;
INT giMnemonicSelection = -1;
BOOL gbMnemonicSync = FALSE;
INT giMnemonicTab = 0;

LRESULT CALLBACK MnemonicEditSubclassProc(HWND hWnd, UINT message,
                                          WPARAM wParam, LPARAM lParam,
                                          UINT_PTR, DWORD_PTR)
{
    switch (message)
    {
    case WM_CHAR:
        if (VK_BACK == wParam)
        {
            SetWindowText(hWnd, TEXT(""));
            return 0;
        }
        if (TEXT(' ') <= wParam)
        {
            TCHAR atText[2] = {(TCHAR)_totupper((TCHAR)wParam), 0};
            SetWindowText(hWnd, atText);
            SendMessage(hWnd, EM_SETSEL, 0, -1);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (VK_DELETE == wParam)
        {
            SetWindowText(hWnd, TEXT(""));
            return 0;
        }
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, MnemonicEditSubclassProc, 0);
        break;
    }

    return DefSubclassProc(hWnd, message, wParam, lParam);
}

wstring MenuTextClean(LPCTSTR ptText)
{
    wstring wsText = ptText ? ptText : TEXT("");
    size_t dPos = wsText.rfind(TEXT("(&"));
    if (wstring::npos != dPos && dPos + 3 < wsText.size() &&
        TEXT(')') == wsText[dPos + 3])
        wsText.erase(dPos);
    return wsText;
}

TCHAR MenuTextMnemonic(LPCTSTR ptText)
{
    if (!ptText)
        return 0;
    LPCTSTR p = _tcsstr(ptText, TEXT("(&"));
    return p && p[2] ? (TCHAR)_totupper(p[2]) : 0;
}

UINT PopupStableId(const wstring &wsName)
{
    if (wsName == TEXT("파일")) return 60001;
    if (wsName == TEXT("편집")) return 60002;
    if (wsName == TEXT("삽입")) return 60003;
    if (wsName == TEXT("변형")) return 60004;
    if (wsName == TEXT("표시")) return 60005;
    if (wsName == ORR_UI_LABEL_OPEN_HISTORY) return 60006;
    if (wsName == ORR_UI_LABEL_MN_UNISPACE) return 60007;
    if (wsName == ORR_UI_LABEL_MN_COLOUR_SEL) return 60008;
    if (wsName == ORR_UI_LABEL_MN_INSFRAME_SEL) return 60009;
    if (wsName == ORR_UI_LABEL_MN_USER_REFS) return 60010;
    if (wsName == TEXT("도트 조정")) return 60011;
    return 0;
}

BOOL ContextCommandParentFind(HMENU hMenu, UINT dCommandId,
                              UINT_PTR *pdParent)
{
    if (!hMenu)
        return FALSE;
    const INT iCount = GetMenuItemCount(hMenu);
    for (INT i = 0; i < iCount; i++)
    {
        MENUITEMINFO stMii{};
        stMii.cbSize = sizeof(stMii);
        stMii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_FTYPE;
        if (!GetMenuItemInfo(hMenu, i, TRUE, &stMii) ||
            (stMii.fType & MFT_SEPARATOR))
            continue;
        if (!stMii.hSubMenu && stMii.wID == dCommandId)
        {
            *pdParent = (UINT_PTR)hMenu;
            return TRUE;
        }
        if (stMii.hSubMenu &&
            ContextCommandParentFind(stMii.hSubMenu, dCommandId, pdParent))
            return TRUE;
    }
    return FALSE;
}

void RowsAppendMenu(HMENU hMenu, MENU_MNEMONIC_SCOPE eScope, UINT dDepth,
                    UINT_PTR dParent)
{
    const INT iCount = GetMenuItemCount(hMenu);
    for (INT i = 0; i < iCount; i++)
    {
        MENUITEMINFO stMii{};
        TCHAR atText[MAX_STRING]{};
        stMii.cbSize = sizeof(stMii);
        stMii.fMask = MIIM_ID | MIIM_STRING | MIIM_SUBMENU | MIIM_FTYPE;
        stMii.dwTypeData = atText;
        stMii.cch = MAX_STRING;
        if (!GetMenuItemInfo(hMenu, i, TRUE, &stMii) ||
            (stMii.fType & MFT_SEPARATOR))
            continue;

        MNEMONIC_ROW stRow;
        stRow.wsName.assign(dDepth * 2, TEXT(' '));
        const wstring wsClean = MenuTextClean(atText);
        stRow.wsName += wsClean;
        stRow.dItemId = stMii.hSubMenu ? PopupStableId(wsClean) : stMii.wID;
        stRow.dParent = dParent;
        stRow.eScope = eScope;
        stRow.cMain = MenuTextMnemonic(atText);
        stRow.bContext =
            MENU_MNEMONIC_MAIN == eScope && !stMii.hSubMenu &&
            nullptr != AppCommandFind(stMii.wID);
        if (stRow.bContext)
        {
            stRow.cContext = MenuMnemonicValueGet(
                MENU_MNEMONIC_EDITOR_CONTEXT, stMii.wID, stRow.cMain);
            stRow.bContextPresent = ContextCommandParentFind(
                CntxMenuGet(), stMii.wID, &stRow.dContextParent);
        }

        if (stRow.dItemId)
            gvcMnemonicRows.push_back(stRow);

        if (stMii.hSubMenu)
            RowsAppendMenu(stMii.hSubMenu, eScope, dDepth + 1,
                           (UINT_PTR)stMii.hSubMenu);
    }
}

void RowsAppendOtherGroup(LPCTSTR ptName, UINT dResource,
                          MENU_MNEMONIC_SCOPE eScope)
{
    MNEMONIC_ROW stHeader;
    stHeader.wsName = TEXT("[");
    stHeader.wsName += ptName;
    stHeader.wsName += TEXT("]");
    stHeader.bHeader = TRUE;
    gvcMnemonicRows.push_back(stHeader);

    HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(dResource));
    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        MenuMnemonicApplyScoped(hSubMenu, eScope);
        RowsAppendMenu(hSubMenu, eScope, 1, (UINT_PTR)hSubMenu);
        DestroyMenu(hMenu);
    }
}

void RowsBuild(HWND hDlg)
{
    gvcMnemonicRows.clear();
    if (0 == giMnemonicTab)
    {
        HMENU hMain = GetMenu(ghMainWnd);
        MenuMnemonicApplyScoped(hMain, MENU_MNEMONIC_MAIN);
        RowsAppendMenu(hMain, MENU_MNEMONIC_MAIN, 0, (UINT_PTR)hMain);
        return;
    }

    RowsAppendOtherGroup(TEXT("페이지 목록"), IDC_PGLVPOPUPMENU,
                         MENU_MNEMONIC_PAGE_LIST);
    RowsAppendOtherGroup(TEXT("레이어 박스"), IDM_LAYERBOX_POPUP,
                         MENU_MNEMONIC_LAYER_BOX);
    RowsAppendOtherGroup(TEXT("문서 탭 우클릭"), IDM_MULTIFILE_POPUP,
                         MENU_MNEMONIC_DOCUMENT_TAB);

    MNEMONIC_ROW stHeader;
    stHeader.wsName = TEXT("[그 외]");
    stHeader.bHeader = TRUE;
    gvcMnemonicRows.push_back(stHeader);
    const UINT adOther[] = {IDM_REBER_DORESET, IDM_LINE_BRUSH_TMPL_VIEW};
    for (UINT dId : adOther)
    {
        MNEMONIC_ROW stRow;
        stRow.wsName = TEXT("  ");
        stRow.wsName += AppCommandLabelGet(dId);
        stRow.dItemId = dId;
        stRow.eScope = MENU_MNEMONIC_OTHER;
        stRow.dParent = 1;
        stRow.cMain = MenuMnemonicValueGet(MENU_MNEMONIC_OTHER, dId, 0);
        gvcMnemonicRows.push_back(stRow);
    }
    const struct { UINT dId; LPCTSTR ptName; TCHAR cDefault; } astTray[] = {
        {60101, TEXT("트레이로 최소화"), TEXT('M')},
        {60102, TEXT("창 복구"), TEXT('R')},
        {60103, TEXT("트레이 메뉴 종료"), TEXT('X')},
    };
    for (const auto &stTray : astTray)
    {
        MNEMONIC_ROW stRow;
        stRow.wsName = TEXT("  ");
        stRow.wsName += stTray.ptName;
        stRow.dItemId = stTray.dId;
        stRow.eScope = MENU_MNEMONIC_OTHER;
        stRow.dParent = 2;
        stRow.cMain = MenuMnemonicValueGet(MENU_MNEMONIC_OTHER, stTray.dId,
                                            stTray.cDefault);
        gvcMnemonicRows.push_back(stRow);
    }
    UNREFERENCED_PARAMETER(hDlg);
}

void CurrentRowsCache()
{
    if (0 == giMnemonicTab)
        gvcMnemonicMainRows = gvcMnemonicRows;
    else
        gvcMnemonicOtherRows = gvcMnemonicRows;
}

void RowsLoadTab(HWND hDlg)
{
    vector<MNEMONIC_ROW> &vcCache =
        0 == giMnemonicTab ? gvcMnemonicMainRows : gvcMnemonicOtherRows;
    if (vcCache.empty())
    {
        RowsBuild(hDlg);
        vcCache = gvcMnemonicRows;
    }
    else
        gvcMnemonicRows = vcCache;
}

void EditSetChar(HWND hDlg, INT id, TCHAR cValue)
{
    TCHAR atText[2] = {cValue, 0};
    SetDlgItemText(hDlg, id, atText);
}

TCHAR EditGetChar(HWND hDlg, INT id)
{
    TCHAR atText[4]{};
    GetDlgItemText(hDlg, id, atText, 4);
    return atText[0] ? (TCHAR)_totupper(atText[0]) : 0;
}

void SelectionApplyInput(HWND hDlg, BOOL bContext)
{
    if (0 > giMnemonicSelection ||
        (size_t)giMnemonicSelection >= gvcMnemonicRows.size())
        return;
    MNEMONIC_ROW &stRow = gvcMnemonicRows[giMnemonicSelection];
    if (stRow.bHeader)
        return;
    if (bContext)
    {
        if (!stRow.bContext)
            return;
        stRow.cContext = EditGetChar(hDlg, IDE_MNEMONIC_CONTEXT);
    }
    else
        stRow.cMain = EditGetChar(hDlg, IDE_MNEMONIC_MAIN);

    TCHAR atValue[2] = {bContext ? stRow.cContext : stRow.cMain, 0};
    ListView_SetItemText(GetDlgItem(hDlg, IDLV_MNEMONIC_LIST),
                         giMnemonicSelection, bContext ? 2 : 1, atValue);
}

void SelectionClear(HWND hDlg, BOOL bContext)
{
    if (0 > giMnemonicSelection ||
        (size_t)giMnemonicSelection >= gvcMnemonicRows.size())
        return;
    MNEMONIC_ROW &stRow = gvcMnemonicRows[giMnemonicSelection];
    if (stRow.bHeader || (bContext && !stRow.bContext))
        return;

    if (bContext)
        stRow.cContext = 0;
    else
        stRow.cMain = 0;
    EditSetChar(hDlg, bContext ? IDE_MNEMONIC_CONTEXT : IDE_MNEMONIC_MAIN, 0);
    TCHAR atValue[2]{};
    ListView_SetItemText(GetDlgItem(hDlg, IDLV_MNEMONIC_LIST),
                         giMnemonicSelection, bContext ? 2 : 1, atValue);
}

void SelectionShow(HWND hDlg, INT iRow)
{
    giMnemonicSelection = iRow;
    gbMnemonicSync = TRUE;
    if (0 <= iRow && (size_t)iRow < gvcMnemonicRows.size() &&
        !gvcMnemonicRows[iRow].bHeader)
    {
        const MNEMONIC_ROW &stRow = gvcMnemonicRows[iRow];
        EditSetChar(hDlg, IDE_MNEMONIC_MAIN, stRow.cMain);
        EditSetChar(hDlg, IDE_MNEMONIC_CONTEXT, stRow.cContext);
        EnableWindow(GetDlgItem(hDlg, IDE_MNEMONIC_MAIN), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_SET_MAIN), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_CLEAR_MAIN), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDE_MNEMONIC_CONTEXT), stRow.bContext);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_SET_CONTEXT),
                     stRow.bContext);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_CLEAR_CONTEXT),
                     stRow.bContext);
    }
    else
    {
        EditSetChar(hDlg, IDE_MNEMONIC_MAIN, 0);
        EditSetChar(hDlg, IDE_MNEMONIC_CONTEXT, 0);
        EnableWindow(GetDlgItem(hDlg, IDE_MNEMONIC_MAIN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_SET_MAIN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_CLEAR_MAIN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDE_MNEMONIC_CONTEXT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_SET_CONTEXT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDB_MNEMONIC_CLEAR_CONTEXT), FALSE);
    }
    gbMnemonicSync = FALSE;
}

void ListFill(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDLV_MNEMONIC_LIST);
    ListView_DeleteAllItems(hList);
    for (size_t i = 0; i < gvcMnemonicRows.size(); i++)
    {
        const MNEMONIC_ROW &stRow = gvcMnemonicRows[i];
        LVITEM stItem{};
        stItem.mask = LVIF_TEXT | LVIF_PARAM;
        stItem.iItem = (INT)i;
        stItem.pszText = const_cast<LPTSTR>(stRow.wsName.c_str());
        stItem.lParam = (LPARAM)i;
        ListView_InsertItem(hList, &stItem);
        if (!stRow.bHeader)
        {
            TCHAR atValue[2] = {stRow.cMain, 0};
            ListView_SetItemText(hList, (INT)i, 1, atValue);
            atValue[0] = stRow.bContext ? stRow.cContext : 0;
            ListView_SetItemText(hList, (INT)i, 2, atValue);
        }
    }
    giMnemonicSelection = -1;
    SelectionShow(hDlg, -1);
}

BOOL HasDuplicate(BOOL bContext)
{
    for (size_t i = 0; i < gvcMnemonicRows.size(); i++)
    {
        const MNEMONIC_ROW &a = gvcMnemonicRows[i];
        TCHAR ca = bContext ? a.cContext : a.cMain;
        if (!ca || a.bHeader ||
            (bContext && (!a.bContext || !a.bContextPresent)))
            continue;
        for (size_t j = i + 1; j < gvcMnemonicRows.size(); j++)
        {
            const MNEMONIC_ROW &b = gvcMnemonicRows[j];
            TCHAR cb = bContext ? b.cContext : b.cMain;
            if (!cb || b.bHeader ||
                (bContext && (!b.bContext || !b.bContextPresent)))
                continue;
            const UINT_PTR dParentA =
                bContext ? a.dContextParent : a.dParent;
            const UINT_PTR dParentB =
                bContext ? b.dContextParent : b.dParent;
            if (dParentA == dParentB && _totupper(ca) == _totupper(cb))
                return TRUE;
        }
    }
    return FALSE;
}

BOOL HasAnyDuplicate()
{
    const vector<MNEMONIC_ROW> vcCurrent = gvcMnemonicRows;
    gvcMnemonicRows = gvcMnemonicMainRows;
    const BOOL bMainDuplicate = HasDuplicate(FALSE) || HasDuplicate(TRUE);
    gvcMnemonicRows = gvcMnemonicOtherRows;
    const BOOL bOtherDuplicate = HasDuplicate(FALSE);
    gvcMnemonicRows = vcCurrent;
    return bMainDuplicate || bOtherDuplicate;
}

void RowsSave()
{
    for (const MNEMONIC_ROW &stRow : gvcMnemonicRows)
    {
        if (!stRow.bHeader && stRow.dItemId)
        {
            MenuMnemonicValueSet(stRow.eScope, stRow.dItemId, stRow.cMain);
            if (stRow.bContext)
                MenuMnemonicValueSet(MENU_MNEMONIC_EDITOR_CONTEXT,
                                     stRow.dItemId, stRow.cContext);
        }
    }
}

void AllRowsSave()
{
    vector<MNEMONIC_ROW> vcCurrent = gvcMnemonicRows;
    gvcMnemonicRows = gvcMnemonicMainRows;
    RowsSave();
    gvcMnemonicRows = gvcMnemonicOtherRows;
    RowsSave();
    gvcMnemonicRows = vcCurrent;
}

void ColumnsInit(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDLV_MNEMONIC_LIST);
    RECT stClient{};
    GetClientRect(hList, &stClient);
    const INT iClientWidth = stClient.right - stClient.left;
    const INT iMnemonicWidth = 74;
    const INT iNameWidth =
        max(112, iClientWidth - (iMnemonicWidth * 2) - 18);

    ListView_SetExtendedListViewStyle(
        hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
    LVCOLUMN stColumn{};
    stColumn.mask = LVCF_TEXT | LVCF_WIDTH;
    stColumn.cx = iNameWidth;
    stColumn.pszText = const_cast<LPTSTR>(TEXT("메뉴 이름"));
    ListView_InsertColumn(hList, 0, &stColumn);
    stColumn.cx = iMnemonicWidth;
    stColumn.pszText = const_cast<LPTSTR>(TEXT("상단 메뉴"));
    ListView_InsertColumn(hList, 1, &stColumn);
    stColumn.cx = iMnemonicWidth;
    stColumn.pszText = const_cast<LPTSTR>(TEXT("우클릭 메뉴"));
    ListView_InsertColumn(hList, 2, &stColumn);
}

INT_PTR CALLBACK MnemonicDlgProc(HWND hDlg, UINT message, WPARAM wParam,
                                 LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        TCITEM stTab{};
        stTab.mask = TCIF_TEXT;
        stTab.pszText = const_cast<LPTSTR>(TEXT("메인 메뉴"));
        TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_MNEMONIC_TAB), 0, &stTab);
        stTab.pszText = const_cast<LPTSTR>(TEXT("기타"));
        TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_MNEMONIC_TAB), 1, &stTab);
        SendDlgItemMessage(hDlg, IDE_MNEMONIC_MAIN, EM_SETLIMITTEXT, 1, 0);
        SendDlgItemMessage(hDlg, IDE_MNEMONIC_CONTEXT, EM_SETLIMITTEXT, 1, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDE_MNEMONIC_MAIN),
                          MnemonicEditSubclassProc, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDE_MNEMONIC_CONTEXT),
                          MnemonicEditSubclassProc, 0, 0);
        ColumnsInit(hDlg);
        giMnemonicTab = 0;
        gvcMnemonicMainRows.clear();
        gvcMnemonicOtherRows.clear();
        RowsLoadTab(hDlg);
        ListFill(hDlg);
        return TRUE;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pHdr = (LPNMHDR)lParam;
        if (IDC_MNEMONIC_TAB == pHdr->idFrom && TCN_SELCHANGE == pHdr->code)
        {
            CurrentRowsCache();
            giMnemonicTab =
                TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_MNEMONIC_TAB));
            RowsLoadTab(hDlg);
            ListFill(hDlg);
            ShowWindow(GetDlgItem(hDlg, IDE_MNEMONIC_CONTEXT),
                       0 == giMnemonicTab ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDB_MNEMONIC_CLEAR_CONTEXT),
                       0 == giMnemonicTab ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDB_MNEMONIC_SET_CONTEXT),
                       0 == giMnemonicTab ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDS_MNEMONIC_CONTEXT_LABEL),
                       0 == giMnemonicTab ? SW_SHOW : SW_HIDE);
            SetWindowText(GetDlgItem(hDlg, IDS_MNEMONIC_MAIN_LABEL),
                          0 == giMnemonicTab ? TEXT("상단 메뉴") :
                                              TEXT("니모닉"));
            return TRUE;
        }
        if (IDLV_MNEMONIC_LIST == pHdr->idFrom &&
            LVN_ITEMCHANGED == pHdr->code)
        {
            LPNMLISTVIEW pLv = (LPNMLISTVIEW)lParam;
            if ((pLv->uNewState & LVIS_SELECTED) &&
                !(pLv->uOldState & LVIS_SELECTED))
            {
                SelectionShow(hDlg, pLv->iItem);
            }
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDB_MNEMONIC_SET_MAIN:
            SelectionApplyInput(hDlg, FALSE);
            return TRUE;
        case IDB_MNEMONIC_SET_CONTEXT:
            SelectionApplyInput(hDlg, TRUE);
            return TRUE;
        case IDB_MNEMONIC_CLEAR_MAIN:
            SelectionClear(hDlg, FALSE);
            return TRUE;
        case IDB_MNEMONIC_CLEAR_CONTEXT:
            SelectionClear(hDlg, TRUE);
            return TRUE;
        case IDB_MNEMONIC_INIT:
            if (IDOK == MessageBox(hDlg,
                TEXT("현재 탭의 니모닉을 기본값으로 되돌립니다."),
                TEXT("설정 초기화"), MB_OKCANCEL | MB_ICONQUESTION))
            {
                for (MNEMONIC_ROW &stRow : gvcMnemonicRows)
                {
                    if (!stRow.bHeader)
                    {
                        stRow.cMain = MenuMnemonicDefaultGet(stRow.eScope,
                                                            stRow.dItemId);
                        if (stRow.bContext)
                            stRow.cContext = MenuMnemonicDefaultGet(
                                MENU_MNEMONIC_EDITOR_CONTEXT, stRow.dItemId);
                    }
                }
                CurrentRowsCache();
                ListFill(hDlg);
            }
            return TRUE;
        case IDOK:
            CurrentRowsCache();
            if (HasAnyDuplicate() &&
                IDYES != MessageBox(hDlg,
                    TEXT("같은 메뉴 단계에 중복된 니모닉이 있습니다.\r\n그래도 저장하시겠습니까?"),
                    TEXT("니모닉 중복"), MB_YESNO | MB_ICONWARNING))
                return TRUE;
            AllRowsSave();
            EndDialog(hDlg, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    UNREFERENCED_PARAMETER(wParam);
    return FALSE;
}
}

HRESULT MenuMnemonicDlgOpen(HWND hWnd)
{
    if (IDOK == DialogBoxParam(CntxInstanceGet(),
                              MAKEINTRESOURCE(IDD_MNEMONIC_KEY_DLG), hWnd,
                              MnemonicDlgProc, 0))
    {
        MenuMnemonicApplyScoped(GetMenu(ghMainWnd), MENU_MNEMONIC_MAIN);
        DrawMenuBar(ghMainWnd);
        CntxEditBuild();
    }
    return S_OK;
}
