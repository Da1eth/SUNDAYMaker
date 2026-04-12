#pragma once

struct INSERT_UI_GEOMETRY
{
    POINT stViewOrigin{};
    POINT stWindowOffset{};
    POINT stContentOffset{};
};

VOID InsertUiUpdateViewOrigin(HWND hViewWnd, LPPOINT pstViewOrigin);
VOID InsertUiCaptureContentOffset(HWND hWnd, INT dTopInset, LPPOINT pstContentOffset);
VOID InsertUiPlaceRelativeToView(HWND hViewWnd, HWND hWnd, INSERT_UI_GEOMETRY *pstGeometry, INT xOffsetFromView, INT yOffsetFromView);
VOID InsertUiPlaceContentAtViewPoint(HWND hViewWnd, HWND hWnd, INSERT_UI_GEOMETRY *pstGeometry, INT dTopInset, INT xContentFromView, INT yContentFromView);
BOOL InsertUiSnapWindowYToViewLine(HWND hViewWnd, INSERT_UI_GEOMETRY *pstGeometry, LPWINDOWPOS pstWpos);
VOID InsertUiUpdateOffsetFromWindowPos(HWND hViewWnd, INSERT_UI_GEOMETRY *pstGeometry, const LPWINDOWPOS pstWpos);
VOID InsertUiFormatStatusText(const RECT *pstPos, const INSERT_UI_GEOMETRY *pstGeometry, INT dHideXdot, INT dTopLine, LPCTSTR ptLabel, LPTSTR ptBuffer, UINT_PTR cchBuffer);
HRESULT InsertUiMoveFromView(HWND hViewWnd, HWND hFloatingWnd, UINT state, INSERT_UI_GEOMETRY *pstGeometry);
INT InsertUiToolbarInitialise(HWND hToolbarWnd, HIMAGELIST hImageList, TBBUTTON *pstButtons, UINT dButtonCount,
                              const UINT *pdStringButtonIndices, const LPCTSTR *pptButtonTexts, UINT dStringCount,
                              BOOL bSetButtonSize);