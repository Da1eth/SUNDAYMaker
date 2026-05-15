#pragma once

#include "Sunday.h"

// 페이지 목록 UI 상태를 한 곳에 모은 컨트롤러 상태
struct PAGE_LIST_CONTROLLER_STATE
{
    HINSTANCE hInstance{};
    HWND hPageWindow{};
    HWND hToolWindow{};
    HWND hPageListWindow{};
    BOOLEAN bPageTooltipView{};
    INT ixMouseSelection{-1};
    BOOLEAN bReturnFocus{};
    WNDPROC pfOrigPageViewProc{};
    WNDPROC pfOrigPageToolProc{};
    HIMAGELIST hToolbarImageList{};
    INT ixLastInfoPage{-1};
    INT dLastInfoByte{-1};
    INT dLastInfoLine{-1};
};

inline PAGE_LIST_CONTROLLER_STATE *PageListControllerStateFromWindow(HWND hWnd)
{
    return reinterpret_cast<PAGE_LIST_CONTROLLER_STATE *>(WndTagGet(hWnd));
}
