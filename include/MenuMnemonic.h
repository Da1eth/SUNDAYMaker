#ifndef SUNDAY_MENU_MNEMONIC_H
#define SUNDAY_MENU_MNEMONIC_H

#include <windows.h>
#include <tchar.h>
#include <cstddef>

enum MENU_MNEMONIC_SCOPE
{
    MENU_MNEMONIC_MAIN,
    MENU_MNEMONIC_EDITOR_CONTEXT,
    MENU_MNEMONIC_PAGE_LIST,
    MENU_MNEMONIC_LAYER_BOX,
    MENU_MNEMONIC_DOCUMENT_TAB,
    MENU_MNEMONIC_OTHER
};

// 메뉴 항목에 니모닉(&X)을 일괄 적용하는 모듈
void MenuMnemonicApply(HMENU hMenu);
void MenuMnemonicApplyScoped(HMENU hMenu, MENU_MNEMONIC_SCOPE eScope);
HRESULT MenuMnemonicDlgOpen(HWND hWnd);
TCHAR MenuMnemonicValueGet(MENU_MNEMONIC_SCOPE eScope, UINT dItemId,
                           TCHAR cDefault);
void MenuMnemonicValueSet(MENU_MNEMONIC_SCOPE eScope, UINT dItemId,
                          TCHAR cMnemonic);
TCHAR MenuMnemonicDefaultGet(MENU_MNEMONIC_SCOPE eScope, UINT dItemId);
void MenuMnemonicTextBuild(LPCTSTR ptLabel, MENU_MNEMONIC_SCOPE eScope,
                           UINT dItemId, TCHAR cDefault, LPTSTR ptText,
                           size_t cchText);

#endif
