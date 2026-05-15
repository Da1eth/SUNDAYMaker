#include "Palette.h"
#include "AppModuleInternal.h"
#include "AppUiContextInternal.h"

extern HFONT ghAaFont;

namespace
{
constexpr LPCTSTR kEditorClass = TEXT("PALETTE_EDITOR");
constexpr INT kDefaultWidth = 840;
constexpr INT kDefaultHeight = 600;
constexpr INT kMinLeftWidth = 240;
constexpr INT kToolbarWidth = 26;
constexpr INT kButtonWidth = 58;
constexpr INT kButtonGap = 8;
constexpr INT kGroupListWidth = kButtonWidth * 3 + kButtonGap * 2;
constexpr INT kRightPaneWidth = kToolbarWidth + kGroupListWidth;
constexpr INT kSplitterWidth = 6;
constexpr INT kMargin = 6;
constexpr INT kButtonHeight = 22;
constexpr INT kBottomHeight = 40;
constexpr INT kBrushParamBase = 100000;
constexpr LPARAM kSeparatorParam = -1;
constexpr COLORREF kGridLineColor = RGB(220, 220, 220);
constexpr COLORREF kSelectedBackColor = RGB(0, 120, 215);
constexpr COLORREF kSelectedTextColor = RGB(255, 255, 255);
constexpr COLORREF kSeparatorBackColor = RGB(245, 245, 245);

struct PALETTE_EDITOR_STATE
{
    HINSTANCE hInstance{};
    HWND hWnd{};
    HWND hEditWnd{};
    HWND hListWnd{};
    HWND hToolbarWnd{};
    HWND hSplitterWnd{};
    HWND hTooltipWnd{};
    HWND hSaveWnd{};
    HWND hCancelWnd{};
    HWND hCloseWnd{};
    HIMAGELIST hToolbarImages{};
    WNDPROC pfOrigEditProc{};
    WNDPROC pfOrigSplitterProc{};
    vector<JSON_TEMPLATE_GROUP> vcLineGroups;
    vector<JSON_TEMPLATE_GROUP> vcBrushGroups;
    LPARAM rdSelectedParam{};
    INT iSplitterLeft{460};
    BOOLEAN bDragging{};
    BOOLEAN bRegistered{};
};

struct PALETTE_NAME_CONTEXT
{
    wstring wsName;
};

PALETTE_EDITOR_STATE gstEditor;

TBBUTTON gstToolbarButtons[] = {
    {0, IDB_PALETTE_EDIT_ADD, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {1, IDB_PALETTE_EDIT_DEL, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {2, IDB_PALETTE_EDIT_UP, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {3, IDB_PALETTE_EDIT_DOWN, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
    {4, IDB_PALETTE_EDIT_RENAME, TBSTATE_ENABLED | TBSTATE_WRAP, BTNS_BUTTON | BTNS_AUTOSIZE, {0, 0}, 0, 0},
};

constexpr UINT gadToolbarImageIds[] = {
    IDBMP_PAGECREATE,
    IDBMP_PAGEDELETE,
    IDBMP_PAGEUPPERSHIFT,
    IDBMP_PAGEUNDERSHIFT,
    IDBMP_PAGENAMECHANGE,
};

constexpr UINT gadToolbarMaskIds[] = {
    IDBMQ_PAGECREATE,
    IDBMQ_PAGEDELETE,
    IDBMQ_PAGEUPPERSHIFT,
    IDBMQ_PAGEUNDERSHIFT,
    IDBMQ_PAGENAMECHANGE,
};

VOID PaletteEditorPathGet(LPTSTR ptPath, UINT cchPath)
{
    StringCchCopy(ptPath, cchPath, ExePathGet());
    PathAppend(ptPath, TEMPLATE_DIR);
    PathAppend(ptPath, AA_PALETTE_FILE);
}

BOOLEAN PaletteEditorParamIsBrush(LPARAM rdParam)
{
    return kBrushParamBase <= rdParam;
}

INT PaletteEditorParamIndex(LPARAM rdParam)
{
    return PaletteEditorParamIsBrush(rdParam) ? (INT)(rdParam - kBrushParamBase) : (INT)rdParam;
}

LPARAM PaletteEditorGroupParam(BOOLEAN bBrush, INT iIndex)
{
    return bBrush ? (kBrushParamBase + iIndex) : iIndex;
}

JSON_TEMPLATE_GROUP *PaletteEditorGroupByParam(LPARAM rdParam)
{
    const INT iIndex = PaletteEditorParamIndex(rdParam);

    if (kSeparatorParam == rdParam || 0 > iIndex)
        return nullptr;

    if (PaletteEditorParamIsBrush(rdParam))
    {
        if (iIndex >= (INT)gstEditor.vcBrushGroups.size())
            return nullptr;
        return &(gstEditor.vcBrushGroups.at(iIndex));
    }

    if (iIndex >= (INT)gstEditor.vcLineGroups.size())
        return nullptr;
    return &(gstEditor.vcLineGroups.at(iIndex));
}

BOOLEAN PaletteEditorNameExists(const wstring &wsName, const JSON_TEMPLATE_GROUP *pstIgnore)
{
    for (const JSON_TEMPLATE_GROUP &stGroup : gstEditor.vcLineGroups)
    {
        if (&stGroup != pstIgnore && stGroup.wsName == wsName)
            return TRUE;
    }
    for (const JSON_TEMPLATE_GROUP &stGroup : gstEditor.vcBrushGroups)
    {
        if (&stGroup != pstIgnore && stGroup.wsName == wsName)
            return TRUE;
    }
    return FALSE;
}

wstring PaletteEditorUniqueName(LPCTSTR ptBase)
{
    for (UINT i = 1;; i++)
    {
        TCHAR atName[SUB_STRING];
        wstring wsName;

        StringCchPrintf(atName, SUB_STRING, TEXT("%s%u"), ptBase, i);
        wsName = atName;
        if (!(PaletteEditorNameExists(wsName, nullptr)))
            return wsName;
    }
}

VOID PaletteEditorFillEmptyNames(VOID)
{
    for (JSON_TEMPLATE_GROUP &stGroup : gstEditor.vcLineGroups)
    {
        if (stGroup.wsName.empty())
            stGroup.wsName = PaletteEditorUniqueName(TEXT("Nameless"));
    }
    for (JSON_TEMPLATE_GROUP &stGroup : gstEditor.vcBrushGroups)
    {
        if (stGroup.wsName.empty())
            stGroup.wsName = PaletteEditorUniqueName(TEXT("Nameless"));
    }
}

VOID PaletteEditorEnsureGroups(VOID)
{
    if (gstEditor.vcLineGroups.empty())
    {
        JSON_TEMPLATE_GROUP stGroup;
        stGroup.wsName = PaletteEditorUniqueName(TEXT("Nameless"));
        gstEditor.vcLineGroups.push_back(stGroup);
    }
    if (gstEditor.vcBrushGroups.empty())
    {
        JSON_TEMPLATE_GROUP stGroup;
        stGroup.wsName = PaletteEditorUniqueName(TEXT("Nameless"));
        gstEditor.vcBrushGroups.push_back(stGroup);
    }
}

BOOLEAN PaletteEditorValidateNames(HWND hWnd)
{
    for (const JSON_TEMPLATE_GROUP &stGroup : gstEditor.vcLineGroups)
    {
        if (PaletteEditorNameExists(stGroup.wsName, &stGroup))
        {
            MessageBox(hWnd, TEXT("중복 그룹명은 저장할 수 없습니다."), TEXT("팔레트 편집"), MB_OK | MB_ICONWARNING);
            return FALSE;
        }
    }
    for (const JSON_TEMPLATE_GROUP &stGroup : gstEditor.vcBrushGroups)
    {
        if (PaletteEditorNameExists(stGroup.wsName, &stGroup))
        {
            MessageBox(hWnd, TEXT("중복 그룹명은 저장할 수 없습니다."), TEXT("팔레트 편집"), MB_OK | MB_ICONWARNING);
            return FALSE;
        }
    }
    return TRUE;
}

VOID PaletteEditorEditItemsGet(vector<wstring> *pItems)
{
    INT cchText;
    wstring wsText;
    wstring wsLine;

    if (!(pItems) || !(gstEditor.hEditWnd))
        return;

    pItems->clear();
    cchText = GetWindowTextLength(gstEditor.hEditWnd);
    if (0 >= cchText)
        return;

    wsText.resize(cchText + 1);
    GetWindowText(gstEditor.hEditWnd, wsText.data(), cchText + 1);
    wsText.resize(cchText);

    for (wchar_t ch : wsText)
    {
        if (L'\r' == ch)
            continue;
        if (L'\n' == ch)
        {
            if (!(wsLine.empty()))
                pItems->push_back(wsLine);
            wsLine.clear();
            continue;
        }
        wsLine.push_back(ch);
    }

    if (!(wsLine.empty()))
        pItems->push_back(wsLine);
}

VOID PaletteEditorApplyCurrentEdit(VOID)
{
    JSON_TEMPLATE_GROUP *pstGroup = PaletteEditorGroupByParam(gstEditor.rdSelectedParam);
    if (pstGroup)
        PaletteEditorEditItemsGet(&(pstGroup->vcItems));
}

VOID PaletteEditorEditItemsSet(const JSON_TEMPLATE_GROUP &stGroup)
{
    wstring wsText;

    for (size_t i = 0; stGroup.vcItems.size() > i; i++)
    {
        if (0 < i)
            wsText += L"\r\n";
        wsText += stGroup.vcItems.at(i);
    }

    SetWindowText(gstEditor.hEditWnd, wsText.c_str());
}

LPARAM PaletteEditorListParamGet(INT iItem)
{
    LVITEM stLvi{};

    stLvi.mask = LVIF_PARAM;
    stLvi.iItem = iItem;
    if (!(ListView_GetItem(gstEditor.hListWnd, &stLvi)))
        return kSeparatorParam;

    return stLvi.lParam;
}

INT PaletteEditorListItemByParam(LPARAM rdParam)
{
    const INT iCount = ListView_GetItemCount(gstEditor.hListWnd);

    for (INT i = 0; iCount > i; i++)
    {
        if (PaletteEditorListParamGet(i) == rdParam)
            return i;
    }
    return -1;
}

VOID PaletteEditorGroupSelect(LPARAM rdParam, BOOLEAN bApplyEdit)
{
    JSON_TEMPLATE_GROUP *pstGroup;
    INT iItem;

    if (bApplyEdit)
        PaletteEditorApplyCurrentEdit();

    pstGroup = PaletteEditorGroupByParam(rdParam);
    if (!(pstGroup))
        return;

    gstEditor.rdSelectedParam = rdParam;
    PaletteEditorEditItemsSet(*pstGroup);

    iItem = PaletteEditorListItemByParam(rdParam);
    if (0 <= iItem)
    {
        ListView_SetItemState(gstEditor.hListWnd, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_SetItemState(gstEditor.hListWnd, iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(gstEditor.hListWnd, iItem, FALSE);
        InvalidateRect(gstEditor.hListWnd, nullptr, FALSE);
    }
}

VOID PaletteEditorListPopulate(LPARAM rdSelectParam)
{
    LVITEM stItem{};
    INT iRow = 0;

    if (!(gstEditor.hListWnd))
        return;

    ListView_DeleteAllItems(gstEditor.hListWnd);
    stItem.mask = LVIF_TEXT | LVIF_PARAM;

    for (size_t i = 0; gstEditor.vcLineGroups.size() > i; i++)
    {
        stItem.iItem = iRow++;
        stItem.pszText = const_cast<LPTSTR>(gstEditor.vcLineGroups.at(i).wsName.c_str());
        stItem.lParam = PaletteEditorGroupParam(FALSE, (INT)i);
        ListView_InsertItem(gstEditor.hListWnd, &stItem);
    }

    stItem.iItem = iRow++;
    stItem.pszText = const_cast<LPTSTR>(TEXT("==="));
    stItem.lParam = kSeparatorParam;
    ListView_InsertItem(gstEditor.hListWnd, &stItem);

    for (size_t i = 0; gstEditor.vcBrushGroups.size() > i; i++)
    {
        stItem.iItem = iRow++;
        stItem.pszText = const_cast<LPTSTR>(gstEditor.vcBrushGroups.at(i).wsName.c_str());
        stItem.lParam = PaletteEditorGroupParam(TRUE, (INT)i);
        ListView_InsertItem(gstEditor.hListWnd, &stItem);
    }

    PaletteEditorGroupSelect(rdSelectParam, FALSE);
}

HRESULT PaletteEditorLoad(VOID)
{
    TCHAR atPath[MAX_PATH];
    HRESULT hLine;
    HRESULT hBrush;

    PaletteEditorPathGet(atPath, MAX_PATH);
    hLine = AppLoadPaletteGroups(atPath, FALSE, &(gstEditor.vcLineGroups));
    hBrush = AppLoadPaletteGroups(atPath, TRUE, &(gstEditor.vcBrushGroups));

    if (FAILED(hLine))
        gstEditor.vcLineGroups.clear();
    if (FAILED(hBrush))
        gstEditor.vcBrushGroups.clear();

    PaletteEditorFillEmptyNames();
    PaletteEditorEnsureGroups();
    gstEditor.rdSelectedParam = PaletteEditorGroupParam(FALSE, 0);

    if (gstEditor.hListWnd)
        PaletteEditorListPopulate(gstEditor.rdSelectedParam);
    if (gstEditor.hEditWnd)
        PaletteEditorEditItemsSet(gstEditor.vcLineGroups.front());

    return (FAILED(hLine) || FAILED(hBrush)) ? E_FAIL : S_OK;
}

HRESULT PaletteEditorBackup(LPCTSTR ptPath)
{
    TCHAR atBackup[MAX_PATH];

    if (!(ptPath) || !(PathFileExists(ptPath)))
        return S_FALSE;
    if (FAILED(StringCchPrintf(atBackup, MAX_PATH, TEXT("%s.bak"), ptPath)))
        return E_FAIL;

    return CopyFile(ptPath, atBackup, FALSE) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

HRESULT PaletteEditorSave(HWND hWnd)
{
    TCHAR atPath[MAX_PATH];
    HRESULT hRslt;

    PaletteEditorApplyCurrentEdit();
    PaletteEditorFillEmptyNames();
    if (!(PaletteEditorValidateNames(hWnd)))
        return E_ABORT;

    PaletteEditorPathGet(atPath, MAX_PATH);
    PaletteEditorBackup(atPath);

    hRslt = AppWritePaletteGroups(atPath, gstEditor.vcLineGroups, gstEditor.vcBrushGroups);
    if (FAILED(hRslt))
    {
        MessageBox(hWnd, TEXT("팔레트 저장에 실패했습니다."), TEXT("팔레트 편집"), MB_OK | MB_ICONERROR);
        return hRslt;
    }

    PaletteCommonReload();
    PaletteBucketReload();
    MessageBox(hWnd, TEXT("팔레트를 저장했습니다."), TEXT("팔레트 편집"), MB_OK | MB_ICONINFORMATION);
    return S_OK;
}

INT_PTR CALLBACK PaletteEditorNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PALETTE_NAME_CONTEXT *pstContext = reinterpret_cast<PALETTE_NAME_CONTEXT *>(WndTagGet(hDlg));

    switch (message)
    {
    case WM_INITDIALOG:
        pstContext = reinterpret_cast<PALETTE_NAME_CONTEXT *>(lParam);
        WndTagSet(hDlg, lParam);
        SetWindowText(hDlg, TEXT("그룹 이름"));
        Edit_SetText(GetDlgItem(hDlg, IDE_PAGENAME), pstContext->wsName.c_str());
        SetFocus(GetDlgItem(hDlg, IDE_PAGENAME));
        return (INT_PTR)FALSE;

    case WM_COMMAND:
        if (IDOK == LOWORD(wParam))
        {
            const INT cchText = GetWindowTextLength(GetDlgItem(hDlg, IDE_PAGENAME));
            pstContext->wsName.resize(cchText + 1);
            GetDlgItemText(hDlg, IDE_PAGENAME, pstContext->wsName.data(), cchText + 1);
            pstContext->wsName.resize(cchText);
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        if (IDCANCEL == LOWORD(wParam))
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }

    return (INT_PTR)FALSE;
}

BOOLEAN PaletteEditorNameInput(HWND hWnd, wstring *pName)
{
    PALETTE_NAME_CONTEXT stContext;

    if (!(pName))
        return FALSE;

    stContext.wsName = *pName;
    if (IDOK != DialogBoxParam(gstEditor.hInstance, MAKEINTRESOURCE(IDD_PAGE_NAME_DLG), hWnd, PaletteEditorNameDlgProc, reinterpret_cast<LPARAM>(&stContext)))
        return FALSE;

    *pName = stContext.wsName;
    return TRUE;
}

VOID PaletteEditorGroupAdd(VOID)
{
    JSON_TEMPLATE_GROUP stGroup;
    BOOLEAN bBrush;
    INT iIndex;
    LPARAM rdNewParam;

    PaletteEditorApplyCurrentEdit();
    bBrush = PaletteEditorParamIsBrush(gstEditor.rdSelectedParam);
    iIndex = PaletteEditorParamIndex(gstEditor.rdSelectedParam);
    stGroup.wsName = PaletteEditorUniqueName(TEXT("Nameless"));

    if (bBrush)
    {
        if (0 > iIndex || iIndex >= (INT)gstEditor.vcBrushGroups.size())
            iIndex = (INT)gstEditor.vcBrushGroups.size() - 1;
        gstEditor.vcBrushGroups.insert(gstEditor.vcBrushGroups.begin() + iIndex + 1, stGroup);
        rdNewParam = PaletteEditorGroupParam(TRUE, iIndex + 1);
    }
    else
    {
        if (0 > iIndex || iIndex >= (INT)gstEditor.vcLineGroups.size())
            iIndex = (INT)gstEditor.vcLineGroups.size() - 1;
        gstEditor.vcLineGroups.insert(gstEditor.vcLineGroups.begin() + iIndex + 1, stGroup);
        rdNewParam = PaletteEditorGroupParam(FALSE, iIndex + 1);
    }

    PaletteEditorListPopulate(rdNewParam);
}

VOID PaletteEditorGroupDelete(HWND hWnd)
{
    BOOLEAN bBrush;
    INT iIndex;
    LPARAM rdNextParam;

    PaletteEditorApplyCurrentEdit();
    bBrush = PaletteEditorParamIsBrush(gstEditor.rdSelectedParam);
    iIndex = PaletteEditorParamIndex(gstEditor.rdSelectedParam);

    if (bBrush)
    {
        if (1 >= gstEditor.vcBrushGroups.size())
        {
            MessageBox(hWnd, TEXT("각 팔레트 종류에는 최소 한 그룹이 필요합니다."), TEXT("팔레트 편집"), MB_OK | MB_ICONWARNING);
            return;
        }
        gstEditor.vcBrushGroups.erase(gstEditor.vcBrushGroups.begin() + iIndex);
        iIndex = min(iIndex, (INT)gstEditor.vcBrushGroups.size() - 1);
        rdNextParam = PaletteEditorGroupParam(TRUE, iIndex);
    }
    else
    {
        if (1 >= gstEditor.vcLineGroups.size())
        {
            MessageBox(hWnd, TEXT("각 팔레트 종류에는 최소 한 그룹이 필요합니다."), TEXT("팔레트 편집"), MB_OK | MB_ICONWARNING);
            return;
        }
        gstEditor.vcLineGroups.erase(gstEditor.vcLineGroups.begin() + iIndex);
        iIndex = min(iIndex, (INT)gstEditor.vcLineGroups.size() - 1);
        rdNextParam = PaletteEditorGroupParam(FALSE, iIndex);
    }

    PaletteEditorListPopulate(rdNextParam);
}

VOID PaletteEditorGroupMove(INT iDiff)
{
    const BOOLEAN bBrush = PaletteEditorParamIsBrush(gstEditor.rdSelectedParam);
    const INT iIndex = PaletteEditorParamIndex(gstEditor.rdSelectedParam);
    vector<JSON_TEMPLATE_GROUP> &vcGroups = bBrush ? gstEditor.vcBrushGroups : gstEditor.vcLineGroups;
    const INT iNewIndex = iIndex + iDiff;

    PaletteEditorApplyCurrentEdit();
    if (0 > iNewIndex || iNewIndex >= (INT)vcGroups.size())
        return;

    std::swap(vcGroups.at(iIndex), vcGroups.at(iNewIndex));
    PaletteEditorListPopulate(PaletteEditorGroupParam(bBrush, iNewIndex));
}

VOID PaletteEditorGroupRename(HWND hWnd)
{
    JSON_TEMPLATE_GROUP *pstGroup = PaletteEditorGroupByParam(gstEditor.rdSelectedParam);
    wstring wsName;

    if (!(pstGroup))
        return;

    wsName = pstGroup->wsName;
    if (!(PaletteEditorNameInput(hWnd, &wsName)))
        return;

    if (wsName.empty())
        wsName = PaletteEditorUniqueName(TEXT("Nameless"));
    if (PaletteEditorNameExists(wsName, pstGroup))
    {
        MessageBox(hWnd, TEXT("이미 사용 중인 그룹명입니다."), TEXT("팔레트 편집"), MB_OK | MB_ICONWARNING);
        return;
    }

    pstGroup->wsName = wsName;
    PaletteEditorListPopulate(gstEditor.rdSelectedParam);
}

VOID PaletteEditorResize(HWND, UINT, INT cx, INT cy)
{
    const INT panelHeight = max(120, cy - kMargin * 2);
    const INT listHeight = max(80, panelHeight - kBottomHeight);
    const INT buttonTop = kMargin + listHeight + (kBottomHeight - kButtonHeight) / 2;
    TTTOOLINFO stToolInfo{};
    INT rightLeft;
    INT groupLeft;

    if (cx < kMinLeftWidth + kRightPaneWidth + kSplitterWidth + kMargin * 2)
        cx = kMinLeftWidth + kRightPaneWidth + kSplitterWidth + kMargin * 2;

    gstEditor.iSplitterLeft = cx - kRightPaneWidth - kSplitterWidth - kMargin;
    rightLeft = gstEditor.iSplitterLeft + kSplitterWidth;
    groupLeft = rightLeft + kToolbarWidth;

    MoveWindow(gstEditor.hEditWnd, kMargin, kMargin, gstEditor.iSplitterLeft - kMargin, panelHeight, TRUE);
    MoveWindow(gstEditor.hSplitterWnd, gstEditor.iSplitterLeft, kMargin, kSplitterWidth, panelHeight, TRUE);
    MoveWindow(gstEditor.hToolbarWnd, rightLeft, kMargin, kToolbarWidth, listHeight, TRUE);
    MoveWindow(gstEditor.hListWnd, groupLeft, kMargin, kGroupListWidth, listHeight, TRUE);
    ListView_SetColumnWidth(gstEditor.hListWnd, 0, kGroupListWidth - 4);

    if (gstEditor.hTooltipWnd)
    {
        stToolInfo.cbSize = sizeof(TTTOOLINFO);
        stToolInfo.uFlags = TTF_SUBCLASS;
        stToolInfo.hwnd = gstEditor.hListWnd;
        stToolInfo.uId = IDLV_PALETTE_EDIT_GROUPS;
        GetClientRect(gstEditor.hListWnd, &(stToolInfo.rect));
        SendMessage(gstEditor.hTooltipWnd, TTM_NEWTOOLRECT, 0, (LPARAM)&stToolInfo);
    }

    MoveWindow(gstEditor.hSaveWnd, groupLeft, buttonTop, kButtonWidth, kButtonHeight, TRUE);
    MoveWindow(gstEditor.hCancelWnd, groupLeft + kButtonWidth + kButtonGap, buttonTop, kButtonWidth, kButtonHeight, TRUE);
    MoveWindow(gstEditor.hCloseWnd, groupLeft + (kButtonWidth + kButtonGap) * 2, buttonTop, kButtonWidth, kButtonHeight, TRUE);
}

VOID PaletteEditorTooltipText(LPNMTTDISPINFO pstInfo)
{
    POINT stPoint;
    LVHITTESTINFO stHit{};

    ZeroMemory(&(pstInfo->szText), sizeof(pstInfo->szText));
    pstInfo->lpszText = pstInfo->szText;

    switch (pstInfo->hdr.idFrom)
    {
    case IDB_PALETTE_EDIT_ADD:
        StringCchCopy(pstInfo->szText, 80, TEXT("그룹 추가"));
        return;
    case IDB_PALETTE_EDIT_DEL:
        StringCchCopy(pstInfo->szText, 80, TEXT("그룹 삭제"));
        return;
    case IDB_PALETTE_EDIT_UP:
        StringCchCopy(pstInfo->szText, 80, TEXT("그룹을 위로 이동"));
        return;
    case IDB_PALETTE_EDIT_DOWN:
        StringCchCopy(pstInfo->szText, 80, TEXT("그룹을 아래로 이동"));
        return;
    case IDB_PALETTE_EDIT_RENAME:
        StringCchCopy(pstInfo->szText, 80, TEXT("그룹 이름 변경"));
        return;
    default:
        break;
    }

    GetCursorPos(&stPoint);
    ScreenToClient(gstEditor.hListWnd, &stPoint);
    stHit.pt = stPoint;
    ListView_HitTest(gstEditor.hListWnd, &stHit);
    if (0 <= stHit.iItem && kSeparatorParam == PaletteEditorListParamGet(stHit.iItem))
        StringCchCopy(pstInfo->szText, 80, TEXT("lineTemplates와 brushTemplates의 구분선입니다."));
}

LRESULT CALLBACK PaletteEditorSplitterProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return TRUE;
    case WM_LBUTTONDOWN:
        gstEditor.bDragging = TRUE;
        SetCapture(gstEditor.hWnd);
        return 0;
    }

    return CallWindowProc(gstEditor.pfOrigSplitterProc, hWnd, msg, wParam, lParam);
}

VOID PaletteEditorEditGridDraw(HWND hWnd)
{
    HDC hdc;
    RECT rect;
    HPEN hPen;
    HPEN hOldPen;
    TEXTMETRIC stTm{};
    INT iLineHeight;
    INT iLineCount;

    hdc = GetDC(hWnd);
    if (!(hdc))
        return;

    GetClientRect(hWnd, &rect);
    SelectObject(hdc, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));
    GetTextMetrics(hdc, &stTm);
    iLineHeight = max<INT>(stTm.tmHeight + stTm.tmExternalLeading, 1);
    iLineCount = max<INT>((INT)SendMessage(hWnd, EM_GETLINECOUNT, 0, 0), rect.bottom / iLineHeight + 1);

    hPen = CreatePen(PS_SOLID, 1, kGridLineColor);
    hOldPen = (HPEN)SelectObject(hdc, hPen);
    for (INT i = 1; i <= iLineCount; i++)
    {
        const INT y = i * iLineHeight;
        if (y >= rect.bottom)
            break;
        MoveToEx(hdc, 0, y, nullptr);
        LineTo(hdc, rect.right, y);
    }
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    ReleaseDC(hWnd, hdc);
}

LRESULT CALLBACK PaletteEditorEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRslt = CallWindowProc(gstEditor.pfOrigEditProc, hWnd, msg, wParam, lParam);

    if (WM_PAINT == msg)
        PaletteEditorEditGridDraw(hWnd);

    return lRslt;
}

LRESULT PaletteEditorNotify(HWND hWnd, INT idFrom, LPNMHDR pstNmhdr)
{
    if (TTN_GETDISPINFO == pstNmhdr->code)
    {
        PaletteEditorTooltipText((LPNMTTDISPINFO)pstNmhdr);
        return 0;
    }

    if (IDLV_PALETTE_EDIT_GROUPS == idFrom)
    {
        if (NM_CUSTOMDRAW == pstNmhdr->code)
        {
            LPNMLVCUSTOMDRAW pstDraw = (LPNMLVCUSTOMDRAW)pstNmhdr;
            if (CDDS_PREPAINT == pstDraw->nmcd.dwDrawStage)
                return CDRF_NOTIFYITEMDRAW;
            if (CDDS_ITEMPREPAINT == pstDraw->nmcd.dwDrawStage)
            {
                const LPARAM rdParam = PaletteEditorListParamGet((INT)pstDraw->nmcd.dwItemSpec);
                if (kSeparatorParam == rdParam)
                {
                    pstDraw->clrText = RGB(96, 96, 96);
                    pstDraw->clrTextBk = kSeparatorBackColor;
                    return CDRF_DODEFAULT;
                }
                if (rdParam == gstEditor.rdSelectedParam)
                {
                    pstDraw->clrText = kSelectedTextColor;
                    pstDraw->clrTextBk = kSelectedBackColor;
                    return CDRF_DODEFAULT;
                }
            }
        }

        if (LVN_ITEMCHANGING == pstNmhdr->code)
        {
            LPNMLISTVIEW pstLv = (LPNMLISTVIEW)pstNmhdr;
            if ((pstLv->uNewState & LVIS_SELECTED) && kSeparatorParam == PaletteEditorListParamGet(pstLv->iItem))
                return TRUE;
        }
        if (NM_CLICK == pstNmhdr->code)
        {
            LPNMLISTVIEW pstLv = (LPNMLISTVIEW)pstNmhdr;
            LVHITTESTINFO stHit{};
            stHit.pt = pstLv->ptAction;
            ListView_HitTest(gstEditor.hListWnd, &stHit);
            if (0 <= stHit.iItem)
            {
                LPARAM rdParam = PaletteEditorListParamGet(stHit.iItem);
                if (kSeparatorParam != rdParam)
                    PaletteEditorGroupSelect(rdParam, TRUE);
            }
        }
    }

    UNREFERENCED_PARAMETER(hWnd);
    return 0;
}

VOID PaletteEditorCommand(HWND hWnd, INT id, HWND, UINT)
{
    switch (id)
    {
    case IDB_PALETTE_EDIT_SAVE:
    case IDM_OVERWRITESAVE:
        PaletteEditorSave(hWnd);
        return;
    case IDB_PALETTE_EDIT_CANCEL:
        PaletteEditorLoad();
        ShowWindow(hWnd, SW_HIDE);
        return;
    case IDB_PALETTE_EDIT_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return;
    case IDB_PALETTE_EDIT_ADD:
        PaletteEditorGroupAdd();
        return;
    case IDB_PALETTE_EDIT_DEL:
        PaletteEditorGroupDelete(hWnd);
        return;
    case IDB_PALETTE_EDIT_UP:
        PaletteEditorGroupMove(-1);
        return;
    case IDB_PALETTE_EDIT_DOWN:
        PaletteEditorGroupMove(1);
        return;
    case IDB_PALETTE_EDIT_RENAME:
        PaletteEditorGroupRename(hWnd);
        return;
    case IDM_COPY:
    case IDM_CUT:
    case IDM_PASTE:
    case IDM_UNDO:
        AppModelessEditCommandForward(hWnd, id);
        return;
    }
}

LRESULT CALLBACK PaletteEditorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hWnd, WM_COMMAND, PaletteEditorCommand);
        HANDLE_MSG(hWnd, WM_SIZE, PaletteEditorResize);
        HANDLE_MSG(hWnd, WM_NOTIFY, PaletteEditorNotify);

    case WM_MOUSEMOVE:
        if (gstEditor.bDragging)
        {
            RECT rect;
            GetClientRect(hWnd, &rect);
            gstEditor.iSplitterLeft = (INT)(SHORT)LOWORD(lParam);
            PaletteEditorResize(hWnd, SIZE_RESTORED, rect.right, rect.bottom);
            return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (gstEditor.bDragging)
        {
            gstEditor.bDragging = FALSE;
            ReleaseCapture();
            return 0;
        }
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        if (gstEditor.hToolbarImages)
        {
            ImageList_Destroy(gstEditor.hToolbarImages);
            gstEditor.hToolbarImages = nullptr;
        }
        gstEditor.hWnd = nullptr;
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

HRESULT PaletteEditorRegister(HINSTANCE hInstance)
{
    WNDCLASSEX wcex{};

    if (gstEditor.bRegistered)
        return S_OK;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PaletteEditorProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = kEditorClass;

    if (!(RegisterClassEx(&wcex)))
        return HRESULT_FROM_WIN32(GetLastError());

    gstEditor.bRegistered = TRUE;
    return S_OK;
}

VOID PaletteEditorToolbarCreate(HINSTANCE hInstance)
{
    constexpr UINT dButtonCount = static_cast<UINT>(std::size(gstToolbarButtons));

    gstEditor.hToolbarWnd = CreateWindowEx(0, TOOLBARCLASSNAME, TEXT("paletteedittoolbar"),
                                           WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_LEFT | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                           0, 0, 0, 0, gstEditor.hWnd, (HMENU)IDTB_PALETTE_EDIT_GROUPS, hInstance, nullptr);
    gstEditor.hToolbarImages = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, dButtonCount, 1);
    for (UINT i = 0; dButtonCount > i; i++)
    {
        HBITMAP hImg = LoadBitmap(hInstance, MAKEINTRESOURCE(gadToolbarImageIds[i]));
        HBITMAP hMask = LoadBitmap(hInstance, MAKEINTRESOURCE(gadToolbarMaskIds[i]));
        ImageList_Add(gstEditor.hToolbarImages, hImg, hMask);
        DeleteBitmap(hImg);
        DeleteBitmap(hMask);
    }
    SendMessage(gstEditor.hToolbarWnd, TB_SETIMAGELIST, 0, (LPARAM)gstEditor.hToolbarImages);
    SendMessage(gstEditor.hToolbarWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(gstEditor.hToolbarWnd, TB_SETROWS, MAKEWPARAM(dButtonCount, TRUE), 0);
    SendMessage(gstEditor.hToolbarWnd, TB_ADDBUTTONS, dButtonCount, (LPARAM)gstToolbarButtons);
}

HRESULT PaletteEditorCreate(HWND hParentWnd)
{
    RECT parentRect;
    RECT clientRect;
    LVCOLUMN stColumn{};
    TTTOOLINFO stToolInfo{};

    gstEditor.hInstance = CntxInstanceGet();
    if (FAILED(PaletteEditorRegister(gstEditor.hInstance)))
        return E_FAIL;

    GetWindowRect(hParentWnd, &parentRect);
    gstEditor.hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, kEditorClass, TEXT("팔레트 편집"),
                                    WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU,
                                    parentRect.left + 48, parentRect.top + 48, kDefaultWidth, kDefaultHeight,
                                    hParentWnd, nullptr, gstEditor.hInstance, nullptr);
    if (!(gstEditor.hWnd))
        return E_FAIL;

    gstEditor.hEditWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, TEXT(""),
                                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN,
                                        0, 0, 0, 0, gstEditor.hWnd, (HMENU)IDE_PALETTE_EDIT_ITEMS, gstEditor.hInstance, nullptr);
    SetWindowFont(gstEditor.hEditWnd, ghAaFont ? ghAaFont : AppUiFontGet(), TRUE);
    gstEditor.pfOrigEditProc = SubclassWindow(gstEditor.hEditWnd, PaletteEditorEditProc);

    gstEditor.hSplitterWnd = CreateWindowEx(0, WC_STATIC, TEXT(""), WS_CHILD | WS_VISIBLE | SS_ETCHEDVERT,
                                            0, 0, 0, 0, gstEditor.hWnd, nullptr, gstEditor.hInstance, nullptr);
    gstEditor.pfOrigSplitterProc = SubclassWindow(gstEditor.hSplitterWnd, PaletteEditorSplitterProc);

    PaletteEditorToolbarCreate(gstEditor.hInstance);

    gstEditor.hListWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, TEXT("palettegroups"),
                                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                        0, 0, 0, 0, gstEditor.hWnd, (HMENU)IDLV_PALETTE_EDIT_GROUPS, gstEditor.hInstance, nullptr);
    ListView_SetExtendedListViewStyle(gstEditor.hListWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
    SetWindowFont(gstEditor.hListWnd, AppUiFontGet(), TRUE);

    stColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    stColumn.fmt = LVCFMT_LEFT;
    stColumn.pszText = const_cast<LPTSTR>(TEXT("그룹"));
    stColumn.cx = 180;
    ListView_InsertColumn(gstEditor.hListWnd, 0, &stColumn);

    gstEditor.hSaveWnd = CreateWindowEx(0, WC_BUTTON, TEXT("저장"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                        0, 0, 0, 0, gstEditor.hWnd, (HMENU)IDB_PALETTE_EDIT_SAVE, gstEditor.hInstance, nullptr);
    gstEditor.hCancelWnd = CreateWindowEx(0, WC_BUTTON, TEXT("취소"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          0, 0, 0, 0, gstEditor.hWnd, (HMENU)IDB_PALETTE_EDIT_CANCEL, gstEditor.hInstance, nullptr);
    gstEditor.hCloseWnd = CreateWindowEx(0, WC_BUTTON, TEXT("닫기"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         0, 0, 0, 0, gstEditor.hWnd, (HMENU)IDB_PALETTE_EDIT_CLOSE, gstEditor.hInstance, nullptr);
    AppUiFontApply(gstEditor.hSaveWnd, TRUE);
    AppUiFontApply(gstEditor.hCancelWnd, TRUE);
    AppUiFontApply(gstEditor.hCloseWnd, TRUE);

    gstEditor.hTooltipWnd = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
                                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, gstEditor.hWnd, nullptr,
                                           gstEditor.hInstance, nullptr);
    AppUiFontApply(gstEditor.hTooltipWnd, FALSE);

    stToolInfo.cbSize = sizeof(TTTOOLINFO);
    stToolInfo.uFlags = TTF_SUBCLASS;
    stToolInfo.hwnd = gstEditor.hListWnd;
    stToolInfo.uId = IDLV_PALETTE_EDIT_GROUPS;
    GetClientRect(gstEditor.hListWnd, &(stToolInfo.rect));
    stToolInfo.lpszText = LPSTR_TEXTCALLBACK;
    SendMessage(gstEditor.hTooltipWnd, TTM_ADDTOOL, 0, (LPARAM)&stToolInfo);
    SendMessage(gstEditor.hTooltipWnd, TTM_SETMAXTIPWIDTH, 0, 0);

    PaletteEditorLoad();
    GetClientRect(gstEditor.hWnd, &clientRect);
    PaletteEditorResize(gstEditor.hWnd, SIZE_RESTORED, clientRect.right, clientRect.bottom);
    return S_OK;
}
} // namespace

HRESULT PaletteEditorOpen(HWND hParentWnd)
{
    if (!(gstEditor.hWnd))
    {
        HRESULT hRslt = PaletteEditorCreate(hParentWnd);
        if (FAILED(hRslt))
            return hRslt;
    }
    else
    {
        PaletteEditorLoad();
    }

    ShowWindow(gstEditor.hWnd, SW_SHOW);
    SetForegroundWindow(gstEditor.hWnd);
    SetFocus(gstEditor.hEditWnd);
    return S_OK;
}

BOOLEAN PaletteEditorHandleMessage(LPMSG pstMsg)
{
    if (!(pstMsg) || !(gstEditor.hWnd) || gstEditor.hWnd != GetForegroundWindow())
        return FALSE;

    if (TranslateAccelerator(gstEditor.hWnd, AppUiContextGet().hMainAccelTable, pstMsg))
        return TRUE;
    if (IsDialogMessage(gstEditor.hWnd, pstMsg))
        return TRUE;

    return FALSE;
}

VOID PaletteEditorDestroy(VOID)
{
    if (gstEditor.hWnd)
    {
        DestroyWindow(gstEditor.hWnd);
        gstEditor.hWnd = nullptr;
    }
}
