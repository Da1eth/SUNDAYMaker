#pragma once

#include "Sunday.h"

enum DOC_APP_SHELL_SYNC_FLAGS : UINT
{
    DOC_APP_SYNC_PAGE_LIST_SELECTION = 0x00000001,
    DOC_APP_SYNC_PAGE_LIST_CLEAR = 0x00000002,
    DOC_APP_SYNC_PAGE_LIST_BUILD = 0x00000004,
    DOC_APP_SYNC_PAGE_LIST_INSERT = 0x00000008,
    DOC_APP_SYNC_PAGE_LIST_DELETE = 0x00000010,
    DOC_APP_SYNC_PAGE_LIST_REWRITE = 0x00000020,
    DOC_APP_SYNC_PAGE_LIST_NAME = 0x00000040,
    DOC_APP_SYNC_PAGE_LIST_INFO = 0x00000080,
    DOC_APP_SYNC_FILE_TAB_FIRST = 0x00000100,
    DOC_APP_SYNC_FILE_TAB_APPEND = 0x00000200,
    DOC_APP_SYNC_FILE_TAB_SELECT = 0x00000400,
    DOC_APP_SYNC_FILE_TAB_RENAME = 0x00000800,
    DOC_APP_SYNC_TITLE = 0x00001000,
    DOC_APP_SYNC_BACKGROUND_RESTART = 0x00002000,
    DOC_APP_SYNC_OPEN_HISTORY = 0x00004000,
    DOC_APP_SYNC_STATUS_BYTE_COUNT = 0x00008000,
};

struct DOC_APP_SHELL_SYNC_REQUEST
{
    UINT dFlags{};
    INT iPage{-1};
    INT iPrevPage{-1};
    UINT dBytes{};
    UINT_PTR dLines{};
    LPARAM dFileNumber{};
    LPVOID pContext{};
    HWND hWindow{};
    LPCTSTR ptText{};
};

// Doc 이 App 셸 구현 세부를 직접 알지 않도록 UI 동기화 요청을 한 번에 넘긴다.
HRESULT DocAppShellSync(const DOC_APP_SHELL_SYNC_REQUEST &stRequest);

BOOLEAN DocAppPageListHasNamedPages(FILES_ITR itFile);

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