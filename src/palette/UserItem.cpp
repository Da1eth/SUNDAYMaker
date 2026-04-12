#include "Sunday.h"
#include "UiText.h"

extern INT gdDocLine; // キャレットのＹ行数・ドキュメント位置

namespace
{
struct UserItemEntry
{
    TCHAR atItemName[MAX_STRING]{};
    vector<ONELINE> vcUnits;
};

TCHAR gatUserItemPath[MAX_PATH]{};
vector<UserItemEntry> gstUserItems;
HMENU ghMainUserItemMenu = nullptr;

void UserItemClearAll()
{
    gstUserItems.clear();
}

void UserItemPathInitialise()
{
    if (gatUserItemPath[0] != TEXT('\0'))
    {
        return;
    }

    StringCchCopy(gatUserItemPath, MAX_PATH, ExePathGet());
    PathAppend(gatUserItemPath, TEMPLATE_DIR);
    PathAppend(gatUserItemPath, USER_ITEM_FILE);
}

HRESULT UserItemAppendMenu(HWND hWnd)
{
    HMENU hMenu;
    HMENU hInsertMenu;

    if (!(hWnd))
        return E_INVALIDARG;

    hMenu = GetMenu(hWnd);
    if (!(hMenu))
        return E_FAIL;

    hInsertMenu = GetSubMenu(hMenu, 2);
    if (!(hInsertMenu))
        return E_FAIL;

    if (ghMainUserItemMenu)
    {
        DestroyMenu(ghMainUserItemMenu);
        ghMainUserItemMenu = nullptr;
    }

    if (FAILED(MenuPickerCreatePagedMenu(&ghMainUserItemMenu, MENU_PICKER_USERITEM,
                                         MENU_PICKER_MENU_GROUP_MAX)))
        return E_FAIL;

    ModifyMenu(hInsertMenu, 6,
               MF_BYPOSITION | MF_POPUP,
               (UINT_PTR)ghMainUserItemMenu,
               UiTextGetMenuText(IDM_MN_USER_REFS));

    DrawMenuBar(hWnd);

    return S_OK;
}

HRESULT UserItemSetLines(vector<ONELINE> &vcUnits, LPCTSTR ptText, UINT cchSize)
{
    ONELINE stEmptyLine;
    ZeroONELINE(&stEmptyLine);

    vcUnits.push_back(stEmptyLine);

    INT yLine = 0;
    for (UINT i = 0; i < cchSize; i++)
    {
        if (CC_CR == ptText[i] && CC_LF == ptText[i + 1])
        {
            vcUnits.push_back(stEmptyLine);
            i++;
            yLine++;
            continue;
        }

        if (CC_TAB == ptText[i])
        {
            continue;
        }

        LETTER stLetter{};
        DocLetterDataCheck(&stLetter, ptText[i]);

        auto &stLine = vcUnits.at(yLine);
        stLine.vcLine.push_back(stLetter);
        stLine.iDotCnt += stLetter.rdWidth;
        stLine.iByteSz += stLetter.mzByte;
    }

    return S_OK;
}

UINT CALLBACK UserItemLoadCallback(LPTSTR ptName, LPCTSTR ptCont, INT cchSize)
{
    gstUserItems.emplace_back();
    auto &stEntry = gstUserItems.back();

    if (ptName)
    {
        StringCchCopy(stEntry.atItemName, MAX_STRING, ptName);
    }

    if (0 < cchSize)
    {
        UserItemSetLines(stEntry.vcUnits, ptCont, static_cast<UINT>(cchSize));
    }

    return 1;
}

LPTSTR UserItemTextLineAlloc(UINT idNum, INT uLine)
{
    if (gstUserItems.size() <= static_cast<size_t>(idNum))
    {
        return nullptr;
    }

    const auto &vcUnits = gstUserItems[idNum].vcUnits;
    if (static_cast<INT_PTR>(vcUnits.size()) <= uLine)
    {
        return nullptr;
    }

    const auto &vcLetters = vcUnits.at(uLine).vcLine;
    const INT_PTR cchText = static_cast<INT_PTR>(vcLetters.size()) + 1;

    LPTSTR ptText = static_cast<LPTSTR>(malloc(cchText * sizeof(TCHAR)));
    if (!ptText)
    {
        return nullptr;
    }

    ZeroMemory(ptText, cchText * sizeof(TCHAR));

    INT_PTR i = 0;
    for (const auto &stLetter : vcLetters)
    {
        ptText[i++] = stLetter.cchMozi;
    }

    ptText[i] = TEXT('\0');

    return ptText;
}
} // namespace
//-------------------------------------------------------------------------------------------------

UINT UserItemCountGet(VOID)
{
    return static_cast<UINT>(gstUserItems.size());
}
//-------------------------------------------------------------------------------------------------

INT UserItemInitialise(HWND hWnd, UINT bFirst)
{
    if (bFirst)
    {
        ZeroMemory(gatUserItemPath, sizeof(gatUserItemPath));
    }

    UserItemPathInitialise();
    UserItemClearAll();

    HANDLE hFile = CreateFile(gatUserItemPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        UserItemAppendMenu(hWnd);
        return 0;
    }

    const INT iByteSize = GetFileSize(hFile, nullptr);
    LPVOID pBuffer = malloc(iByteSize + 2);
    if (!pBuffer)
    {
        CloseHandle(hFile);
        return 0;
    }

    ZeroMemory(pBuffer, iByteSize + 2);

    DWORD dRead = 0;
    SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
    ReadFile(hFile, pBuffer, iByteSize, &dRead, nullptr);
    CloseHandle(hFile);

    LPTSTR ptString = TextDecodeAutoAlloc(pBuffer, iByteSize, nullptr);
    FREE(pBuffer);
    if (!ptString)
    {
        UserItemAppendMenu(hWnd);
        return 0;
    }

    UINT cchSize = 0;
    StringCchLength(ptString, STRSAFE_MAX_CCH, &cchSize);

    if (0 == StrCmpN(AST_SEPARATERW, ptString, AST_SPRT_CCH))
    {
        DocStringSplitAST(ptString, cchSize, UserItemLoadCallback);
    }

    FREE(ptString);

    UserItemAppendMenu(hWnd);

    return 1;
}
//-------------------------------------------------------------------------------------------------

HRESULT UserItemNameGet(UINT dNumber, LPTSTR ptNamed, UINT_PTR cchSize)
{
    if (gstUserItems.size() <= static_cast<size_t>(dNumber))
    {
        return E_OUTOFMEMORY;
    }

    StringCchCopy(ptNamed, cchSize, gstUserItems[dNumber].atItemName);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

HRESULT UserItemInsert(HWND hWnd, UINT idNum)
{
    UNREFERENCED_PARAMETER(hWnd);

    if (gstUserItems.size() <= static_cast<size_t>(idNum))
    {
        return E_OUTOFMEMORY;
    }

    INT yLine = gdDocLine;
    const INT_PTR dNeedLine = static_cast<INT_PTR>(gstUserItems[idNum].vcUnits.size());

    INT iLines = DocPageParamGet(nullptr, nullptr);
    BOOLEAN bFirst = TRUE;

    if (iLines < (dNeedLine + yLine))
    {
        const INT iExtraLineCount = static_cast<INT>((dNeedLine + yLine) - iLines);
        DocAdditionalLine(iExtraLineCount, &bFirst);
        iLines = DocPageParamGet(nullptr, nullptr);
    }

    for (INT i = 0; i < dNeedLine; i++, yLine++)
    {
        LPTSTR ptText = UserItemTextLineAlloc(idNum, i);
        if (!ptText)
        {
            continue;
        }

        INT dmyDot = 0;
        DocInsertString(&dmyDot, &yLine, nullptr, ptText, 0, bFirst);
        bFirst = FALSE;

        FREE(ptText);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------