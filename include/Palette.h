#pragma once

#include "Sunday.h"
#include "EditorController.h"

struct PALETTE_LIST_HIT
{
	INT iItem{-1};
	INT iSubItem{-1};
	INT iIndex{-1};
};

HRESULT PaletteListGroupsLoad(BOOLEAN bBrush, vector<JSON_TEMPLATE_GROUP> *pGroups);
VOID PaletteListComboReload(HWND hComboWnd, const vector<JSON_TEMPLATE_GROUP> &vcGroups);
VOID PaletteListResizeColumns(HWND hLvWnd, UINT dColumnCount);
HRESULT PaletteListPopulate(HWND hLvWnd, const vector<wstring> &vcItems, UINT dColumnCount);
UINT PaletteListGridFluctuate(HWND hLvWnd, INT dFluct);
VOID PaletteListClearSelection(HWND hLvWnd);
BOOLEAN PaletteListHitTest(HWND hLvWnd, UINT dColumnCount, POINT stPoint, PALETTE_LIST_HIT *pstHit);
BOOLEAN PaletteListCursorHitTest(HWND hLvWnd, UINT dColumnCount, PALETTE_LIST_HIT *pstHit);
LPCTSTR PaletteListGroupItemGet(const vector<JSON_TEMPLATE_GROUP> &vcGroups, UINT dGroupIndex, INT iItemIndex);
BOOLEAN PaletteListBuildTooltipText(HWND hLvWnd, const vector<JSON_TEMPLATE_GROUP> &vcGroups, UINT dGroupIndex, UINT dColumnCount, LPTSTR ptBuffer, UINT cchBuffer);

HRESULT PaletteBucketModeToggle(VOID);
HWND PaletteCommonInitialise(HINSTANCE, HWND, LPRECT);
VOID PaletteCommonResize(HWND, LPRECT);
HRESULT PaletteCommonPositionReset(HWND);
VOID DockingTabSizeGet(LPRECT);
HRESULT DockingTabContextMenu(HWND, HWND, LONG, LONG);
HWND DockingTabGet(VOID);
HRESULT DockingTmplViewToggle(UINT);
HWND PaletteBucketInitialise(HINSTANCE, HWND, LPRECT, HWND);
LPTSTR BrushStringMake(INT, LPTSTR);
VOID PaletteBucketResize(HWND, LPRECT);
HRESULT PaletteBucketPositionReset(HWND);