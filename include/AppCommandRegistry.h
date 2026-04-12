#pragma once

struct APP_COMMAND_ITEM
{
    UINT dCommandId{};
    LPCTSTR ptFallbackLabel{};
};

UINT AppCommandCount(VOID);
const APP_COMMAND_ITEM *AppCommandAt(UINT dIndex);
const APP_COMMAND_ITEM *AppCommandFind(UINT dCommandId);
LPCTSTR AppCommandLabelGet(UINT dCommandId);
HRESULT AppCommandNameCopy(UINT dCommandId, LPTSTR ptText, size_t cchText);
HRESULT AppCommandDisplayNameCopy(UINT dCommandId, LPTSTR ptText, size_t cchText);

BOOLEAN AppCommandAllowedInAccel(UINT dCommandId);
BOOLEAN AppCommandHasChildMenu(UINT dCommandId);
BOOLEAN AppCommandGetSubmenuCommands(UINT dCommandId, const UINT **ppCommands,
                                     UINT *pdCount);

INT AppCommandFrameIndex(UINT dCommandId);
INT AppCommandUserItemIndex(UINT dCommandId);