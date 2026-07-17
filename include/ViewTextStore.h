#pragma once

#include "Sunday.h"

// TSF bridge for the custom editor view.  The implementation owns one
// ITextStoreACP and attaches/detaches its document manager as focus changes.
HRESULT ViewTextStoreInitialise(HWND hWnd);
VOID ViewTextStoreDestroy();
VOID ViewTextStoreSetFocus(BOOLEAN bFocused);
BOOLEAN ViewTextStoreIsComposing();
VOID ViewTextStoreFlushPendingActions();
VOID ViewTextStoreNotifyExternalChange();
