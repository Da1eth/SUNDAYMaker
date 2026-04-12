#pragma once

#include "Sunday.h"

// Doc 이 App 셸 구현 세부를 직접 알지 않도록 UI 동기화 호출을 모은다.
inline HRESULT DocAppPageListSelectionChange(INT dPageNum, INT iPrevPage)
{
    return PageListViewChange(dPageNum, iPrevPage);
}

inline VOID DocAppPageListClear()
{
    PageListClear();
}

inline VOID DocAppPageListBuild(LPVOID pContext)
{
    PageListBuild(pContext);
}

inline VOID DocAppPageListInsert(INT iPage)
{
    PageListInsert(iPage);
}

inline VOID DocAppPageListDelete(INT iPage)
{
    PageListDelete(iPage);
}

inline VOID DocAppPageListRewrite(INT iPage)
{
    PageListViewRewrite(iPage);
}

inline VOID DocAppPageListNameSet(INT iPage, LPTSTR ptName)
{
    PageListNameSet(iPage, ptName);
}

inline VOID DocAppPageListInfoSet(INT iPage, UINT dBytes, UINT_PTR dLines)
{
    PageListInfoSet(iPage, dBytes, dLines);
}

inline BOOLEAN DocAppPageListHasNamedPages(FILES_ITR itFile)
{
    return PageListIsNamed(itFile);
}

inline VOID DocAppMultiFileTabFirst(LPTSTR ptFile)
{
    MultiFileTabFirst(ptFile);
}

inline VOID DocAppMultiFileTabAppend(LPARAM dNumber, LPTSTR ptFile)
{
    MultiFileTabAppend(dNumber, ptFile);
}

inline HRESULT DocAppMultiFileTabSelect(LPARAM dNumber)
{
    return MultiFileTabSelect(dNumber);
}

inline VOID DocAppMultiFileTabRename(LPARAM dNumber, LPTSTR ptFile)
{
    MultiFileTabRename(dNumber, ptFile);
}

inline VOID DocAppTitleChange(LPTSTR ptTitle)
{
    AppTitleChange(ptTitle);
}

inline VOID DocAppBackgroundPageLoadRestart(HWND hWindow)
{
    AppBackgroundPageLoadRestart(hWindow);
}

inline VOID DocAppOpenHistoryLogging(HWND hWnd, LPTSTR ptFile)
{
    OpenHistoryLogging(hWnd, ptFile);
}

inline VOID DocAppMainStatusSetByteCount(UINT dBytes)
{
    MainSttBarSetByteCount(dBytes);
}

inline HRESULT DocAppLoadFlipEntries(LPCTSTR ptFileName, UINT dMode, vector<JSON_FLIP_ENTRY> *pEntries)
{
    return AppLoadFlipEntries(ptFileName, dMode, pEntries);
}

inline VOID DocUiFocusWindow(HWND hWnd)
{
    SetFocus(hWnd);
}

inline LRESULT DocUiSendWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return SendMessage(hWnd, message, wParam, lParam);
}