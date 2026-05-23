#pragma once

#include "Sunday.h"

typedef struct tagCOLOUROBJECT
{
    COLORREF dTextColour;
    HBRUSH hBackBrush;
    HPEN hGridPen;
    HPEN hCrLfPen;
    HBRUSH hUniBackBrs;

} COLOUROBJECT, *LPCOLOUROBJECT;

typedef struct tagVIEWCARETSTATE
{
    INT &dXdot;
    INT &dLine;
    INT &dMozi;

} VIEWCARETSTATE;

typedef struct tagVIEWORIGINSTATE
{
    INT &dHideXdot;
    INT &dTopLine;

} VIEWORIGINSTATE;

typedef struct tagVIEWDISPLAYSTATE
{
    HFONT &hAaFont;
    UINT &dSpaceView;
    BOOLEAN &bGridView;
    UINT &dGridXpos;
    UINT &dGridYpos;
    BOOLEAN &bRightRulerView;
    UINT &dRightRuler;
    BOOLEAN &bUnderRulerView;
    UINT &dUnderRuler;

} VIEWDISPLAYSTATE;

enum ViewColourIndex
{
    CLRT_BASICPEN = 0,
    CLRT_BASICBK,
    CLRT_SELECTBK,
    CLRT_SPACEWARN,
    CLRT_SPACELINE,
    CLRT_CARETFD,
    CLRT_CARETBK,
    CLRT_LASTSPWARN,
    CLRT_CRLF_MARK,
    CLRT_EOF_MARK,
    CLRT_RULER,
    CLRT_RULERBK,
    CLRT_NOSJISBK,
    CLRT_CARET_POS,
    CLRT_GRID_LINE,
    CLRT_FINDBACK,
    CLRT_COUNT,
};

enum ViewPenIndex
{
    PENT_CRLF_MARK = 0,
    PENT_RULER,
    PENT_SPACEWARN,
    PENT_SPACELINE,
    PENT_CARET_POS,
    PENT_GRID_LINE,
    PENT_COUNT,
};

enum ViewBrushIndex
{
    BRHT_BASICBK = 0,
    BRHT_RULERBK,
    BRHT_SELECTBK,
    BRHT_LASTSPWARN,
    BRHT_NOSJISBK,
    BRHT_FINDBACK,
    BRHT_COUNT,
};

extern HINSTANCE ghInst;
extern HWND ghPrntWnd;
extern HWND ghViewWnd;

extern INT gdXmemory;
extern INT gdDocXdot;
extern INT gdDocLine;
extern INT gdDocMozi;

extern INT gdHideXdot;
extern INT gdViewTopLine;
extern SIZE gstViewArea;
extern INT gdDispingLine;

extern HFONT ghAaFont;
extern HFONT ghRulerFont;
extern HFONT ghNumFont4L;
extern HFONT ghNumFont5L;
extern HFONT ghNumFont6L;

extern INT gdAutoDiffBase;
extern UINT gdSpaceView;

extern BOOLEAN gbGridView;
extern UINT gdGridXpos;
extern UINT gdGridYpos;

extern BOOLEAN gbRitRlrView;
extern UINT gdRightRuler;

extern BOOLEAN gbUndRlrView;
extern UINT gdUnderRuler;

extern UINT gdWheelLine;

extern BOOLEAN gbExtract;

extern BOOLEAN gbShiftOn;
extern BOOLEAN gbCtrlOn;
extern BOOLEAN gbAltOn;

extern POINT gstCursor;

extern UINT gbSqSelect;
extern UINT gbPalBucketMode;

struct VIEWCOMMANDSTATE
{
    UINT dSquareSelect{};
    UINT dBucketMode{};
    BOOLEAN bExtract{};
};

extern COLORREF gaColourTable[CLRT_COUNT];
extern HPEN gahPen[PENT_COUNT];
extern HBRUSH gahBrush[BRHT_COUNT];

inline VIEWCARETSTATE ViewCurrentCaret()
{
    return {gdDocXdot, gdDocLine, gdDocMozi};
}

inline VIEWORIGINSTATE ViewCurrentOrigin()
{
    return {gdHideXdot, gdViewTopLine};
}

inline VIEWDISPLAYSTATE ViewDisplayStateGet()
{
    return {
        ghAaFont,
        gdSpaceView,
        gbGridView,
        gdGridXpos,
        gdGridYpos,
        gbRitRlrView,
        gdRightRuler,
        gbUndRlrView,
        gdUnderRuler,
    };
}

inline HFONT ViewAaFontGet()
{
    return ViewDisplayStateGet().hAaFont;
}

inline VIEWCOMMANDSTATE ViewCommandStateGet()
{
    VIEWCOMMANDSTATE stState{};

    stState.dSquareSelect = gbSqSelect;
    stState.dBucketMode = gbPalBucketMode;
    stState.bExtract = gbExtract;

    return stState;
}

inline UINT ViewSquareSelectModeGet()
{
    return ViewCommandStateGet().dSquareSelect;
}

inline BOOLEAN ViewExtractionModeGet()
{
    return ViewCommandStateGet().bExtract;
}

inline VOID ViewExtractionModeSet(BOOLEAN bExtractMode)
{
    gbExtract = bExtractMode;
}

HRESULT ViewScrollBarAdjust(LPVOID);
HRESULT ViewRedrawDo(HWND, HDC);
HRESULT ViewColourEditDlg(HWND);

// 편집 명령은 EditorController.h 에서 선언한다.
#include "EditorController.h"
