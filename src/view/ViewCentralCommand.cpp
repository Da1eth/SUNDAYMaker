#include "AppUiContextInternal.h"
#include "DocContextInternal.h"
#include "EditorController.h"
#include "ViewCentralInternal.h"
#include "Palette.h"

struct VIEW_COMMAND_REQUEST
{
    HWND hMainWindow{};
    INT id{};
    HWND hControl{};
    UINT codeNotify{};
};

using VIEW_COMMAND_DISPATCH = BOOLEAN (*)(const VIEW_COMMAND_REQUEST &);

static VOID OperationExtractionModeClear(VOID);
static BOOLEAN OperationHandleDynamicCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleWindowCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleDialogCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleFileCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleEditCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleInsertCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleLayoutCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleViewCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandlePageCommands(const VIEW_COMMAND_REQUEST &);
static BOOLEAN OperationHandleMiscCommands(const VIEW_COMMAND_REQUEST &);

static const VIEW_COMMAND_DISPATCH gapfViewCommandDispatchers[] = {
    OperationHandleWindowCommands,
    OperationHandleDialogCommands,
    OperationHandleFileCommands,
    OperationHandleEditCommands,
    OperationHandleInsertCommands,
    OperationHandleLayoutCommands,
    OperationHandleViewCommands,
    OperationHandlePageCommands,
    OperationHandleMiscCommands,
};

HRESULT OperationOnStatusBar(VOID)
{
    const VIEWCOMMANDSTATE stCommandState = ViewCommandStateGet();
    const VIEWDISPLAYSTATE stDisplayState = ViewDisplayStateGet();
    CONST TCHAR* catTexts[] = {TEXT("[사각형 선택]"), TEXT("[채우기]"), TEXT("[다중 선택]"), TEXT("[공백 표시]")};
    TCHAR atString[SUB_STRING];

    ZeroMemory(atString, sizeof(atString));

    if (stCommandState.dSquareSelect)
    {
        StringCchCat(atString, SUB_STRING, catTexts[0]);
    }
    if (stCommandState.dBucketMode)
    {
        StringCchCat(atString, SUB_STRING, catTexts[1]);
    }
    if (stCommandState.bExtract)
    {
        StringCchCat(atString, SUB_STRING, catTexts[2]);
    }
    if (stDisplayState.dSpaceView)
    {
        StringCchCat(atString, SUB_STRING, catTexts[3]);
    }

    MainStatusBarSetText(SB_OP_STYLE, atString);

    return S_OK;
}

static VOID OperationExtractionModeClear(VOID)
{
    ViewExtractionModeSet(FALSE);
    ViewSelPageAll(-1);
    ViewRedrawSetLine(-1);
    MenuItemCheckOnOff(IDM_EXTRACTION_MODE, 0);
    OperationOnStatusBar();
}

static BOOLEAN OperationHandleDynamicCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    if (MENU_DYNAMIC_FRAME_FIRST <= (UINT)stCommand.id && (UINT)stCommand.id <= MENU_DYNAMIC_FRAME_LAST)
    {
        ViewFrameInsert(stCommand.id - MENU_DYNAMIC_FRAME_FIRST);
        return TRUE;
    }

    if (MENU_DYNAMIC_USER_FIRST <= (UINT)stCommand.id && (UINT)stCommand.id <= MENU_DYNAMIC_USER_LAST)
    {
        UserItemInsert(stCommand.hMainWindow, (stCommand.id - MENU_DYNAMIC_USER_FIRST));
        return TRUE;
    }

    if (IDM_USERINS_ITEM_FIRST <= stCommand.id && stCommand.id <= IDM_USERINS_ITEM_LAST)
    {
        UserItemInsert(stCommand.hMainWindow, (stCommand.id - IDM_USERINS_ITEM_FIRST));
        return TRUE;
    }

    if (IDM_OPEN_HIS_FIRST <= stCommand.id && stCommand.id <= IDM_OPEN_HIS_LAST)
    {
        OpenHistoryLoad(stCommand.hMainWindow, stCommand.id);
        return TRUE;
    }

    if (IDM_OPEN_HIS_CLEAR == stCommand.id)
    {
        OpenHistoryLogging(stCommand.hMainWindow, nullptr);
        return TRUE;
    }

    return FALSE;
}

static BOOLEAN OperationHandleWindowCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    const AppUiContext stUiContext = AppUiContextGet();

    switch (stCommand.id)
    {
    case IDM_ON_PREVIEW:
        PreviewVisibalise(DocCurrentPageIndex(), TRUE);
        return TRUE;

    case IDM_PAGELIST_VIEW:
        ShowWindow(stUiContext.hPageListWindow, SW_SHOW);
        SetForegroundWindow(stUiContext.hPageListWindow);
        return TRUE;

    case IDM_INSERT_PALETTE:
        ShowWindow(stUiContext.hInsertPaletteWindow, SW_SHOW);
        SetForegroundWindow(stUiContext.hInsertPaletteWindow);
        return TRUE;

    case IDM_BRUSH_PALETTE:
        ShowWindow(stUiContext.hBucketPaletteWindow, SW_SHOW);
        SetForegroundWindow(stUiContext.hBucketPaletteWindow);
        return TRUE;

    case IDM_WINDOW_CHANGE:
        WindowFocusChange(WND_MAIN, 1);
        return TRUE;

    case IDM_WINDOW_CHG_RVRS:
        WindowFocusChange(WND_MAIN, -1);
        return TRUE;

    case IDM_TRC_VIEWTOGGLE:
        TraceImgViewTglExt();
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOLEAN OperationHandleDialogCommands(HWND hWnd, INT id, HWND hWndCtl)
{
    const AppUiContext stUiContext = AppUiContextGet();

    switch (id)
    {
    case IDM_INSFRAME_EDIT:
        FrameEditDialogue(stUiContext.hInstance, hWnd, 0);
        return TRUE;

    case IDM_PALETTE_EDIT_OPEN:
        PaletteEditorOpen(hWnd);
        return TRUE;

    case IDM_GENERAL_OPTION:
        OptionDialogueOpen();
        return TRUE;

    case IDM_TRACE_MODE_ON:
        TraceDialogueOpen(stUiContext.hInstance, hWnd);
        return TRUE;

    case IDM_COLOUR_EDIT_OPEN:
        ViewColourEditDlg(hWnd);
        return TRUE;

    case IDM_IN_UNI_SPACE:
    case IDM_USERINS_NA:
        ToolBarPseudoDropDown(hWnd, id);
        return TRUE;

    case IDM_INSTAG_COLOUR:
        if (hWndCtl)
        {
            ToolBarPseudoDropDown(hWnd, id);
            return TRUE;
        }
        return FALSE;

    case IDM_MN_INSFRAME_SEL:
    {
        POINT stPoint;
        GetCursorPos(&stPoint);
        MenuPickerShow(hWnd, MENU_PICKER_FRAME, stPoint.x, stPoint.y);
        return TRUE;
    }

    case IDM_MN_USER_REFS:
    {
        POINT stPoint;
        GetCursorPos(&stPoint);
        MenuPickerShow(hWnd, MENU_PICKER_USERITEM, stPoint.x, stPoint.y);
        return TRUE;
    }

    case IDM_FRMINSBOX_OPEN:
        FrameInsBoxCreate(stUiContext.hInstance, hWnd);
        return TRUE;

    case IDM_MENUEDIT_DLG_OPEN:
        CntxEditDlgOpen(hWnd);
        return TRUE;

#ifdef ACCELERATOR_EDIT
    case IDM_ACCELKEY_EDIT_DLG_OPEN:
        AccelKeyDlgOpen(hWnd);
        return TRUE;
#endif

#ifdef FIND_STRINGS
    case IDM_FIND_DLG_OPEN:
        FindDialogueOpen(stUiContext.hInstance, hWnd);
        return TRUE;
    case IDM_FIND_JUMP_NEXT:
        FindDirectly(stUiContext.hInstance, hWnd, IDM_FIND_JUMP_NEXT);
        return TRUE;

    case IDM_FIND_TARGET_SET:
        FindDirectly(stUiContext.hInstance, hWnd, IDM_FIND_TARGET_SET);
        return TRUE;
#endif

    default:
        return FALSE;
    }
}

static BOOLEAN OperationHandleDialogCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    return OperationHandleDialogCommands(stCommand.hMainWindow, stCommand.id, stCommand.hControl);
}

static BOOLEAN OperationHandleFileCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    UNREFERENCED_PARAMETER(stCommand);

    return FALSE;
}

static BOOLEAN OperationHandleEditCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    const AppUiContext stUiContext = AppUiContextGet();
    const UINT dSquareSelect = ViewSquareSelectModeGet();

    switch (stCommand.id)
    {
    case IDM_UNDO:
        ViewEditUndoForward();
        return TRUE;

    case IDM_REDO:
        ViewEditRedoForward();
        return TRUE;

    case IDM_CUT:
        ViewEditCutSelection(dSquareSelect);
        return TRUE;

    case IDM_COPY:
        if (ViewExtractionModeGet())
        {
            ViewEditExecuteExtraction(nullptr);
            OperationExtractionModeClear();
        }
        else
        {
            ViewEditCopySelection(dSquareSelect);
        }
        return TRUE;

    case IDM_SJISCOPY_ALL:
        ViewEditCopyPageAll();
        return TRUE;

    case IDM_PASTE:
        ViewEditPasteFromClipboard(0);
        return TRUE;

    case IDM_SQUARE_PASTE:
        ViewEditPasteFromClipboard(1);
        return TRUE;

    case IDM_DELETE:
        ViewEditDeleteForward();
        return TRUE;

    case IDM_ALLSEL:
        ViewSelPageAll(1);
        return TRUE;

    case IDM_SQSELECT:
        ViewSqSelModeToggle(1, nullptr);
        return TRUE;

    case IDM_FILL_SPACE:
        ViewEditFillSelection(nullptr);
        return TRUE;

    case IDM_EXTRACTION_MODE:
        if (ViewExtractionModeGet())
        {
            ViewExtractionModeSet(FALSE);
            ViewSelPageAll(-1);
            ViewRedrawSetLine(-1);
        }
        else
        {
            ViewExtractionModeSet(TRUE);
        }
        MenuItemCheckOnOff(IDM_EXTRACTION_MODE, ViewExtractionModeGet());
        OperationOnStatusBar();
        return TRUE;

    case IDM_LAYERBOX:
        if (ViewExtractionModeGet())
        {
            ViewEditExecuteExtraction(stUiContext.hInstance);
            OperationExtractionModeClear();
        }
        else
        {
            LayerBoxVisibalise(stUiContext.hInstance, nullptr, 0x00);
        }
        return TRUE;

    case IDM_UNICODE_TOGGLE:
        UnicodeUseToggle(nullptr);
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOLEAN OperationHandleInsertCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    UNREFERENCED_PARAMETER(stCommand);
    const UINT dSquareSelect = ViewSquareSelectModeGet();

    switch (stCommand.id)
    {
    case IDM_BRUSH_STYLE:
        PaletteBucketModeToggle();
        return TRUE;

    case IDM_INSFRAME_ALPHA:
    case IDM_INSFRAME_BRAVO:
    case IDM_INSFRAME_CHARLIE:
    case IDM_INSFRAME_DELTA:
    case IDM_INSFRAME_ECHO:
    case IDM_INSFRAME_FOXTROT:
    case IDM_INSFRAME_GOLF:
    case IDM_INSFRAME_HOTEL:
    case IDM_INSFRAME_INDIA:
    case IDM_INSFRAME_JULIETTE:
    case IDM_INSFRAME_KILO:
    case IDM_INSFRAME_LIMA:
    case IDM_INSFRAME_MIKE:
    case IDM_INSFRAME_NOVEMBER:
    case IDM_INSFRAME_OSCAR:
    case IDM_INSFRAME_PAPA:
    case IDM_INSFRAME_QUEBEC:
    case IDM_INSFRAME_ROMEO:
    case IDM_INSFRAME_SIERRA:
    case IDM_INSFRAME_TANGO:
        ViewFrameInsert(stCommand.id - IDM_INSFRAME_ALPHA);
        return TRUE;

    case IDM_IN_01SPACE:
    case IDM_IN_02SPACE:
    case IDM_IN_03SPACE:
    case IDM_IN_04SPACE:
    case IDM_IN_05SPACE:
    case IDM_IN_08SPACE:
    case IDM_IN_10SPACE:
    case IDM_IN_16SPACE:
        ViewInsertUniSpace(stCommand.id);
        return TRUE;

    case IDM_INSTAG_SPO:
        ViewInsertSpoTag();
        return TRUE;

    case IDM_INSTAG_COLOUR:
        ViewInsertColourTag(stCommand.id);
        return TRUE;

    case IDM_INSTAG_GRADIENT:
        ViewInsertColourTag(stCommand.id);
        return TRUE;

    case IDM_LINE_BRUSH_TMPL_VIEW:
        DockingTmplViewToggle(0);
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOLEAN OperationHandleLayoutCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    return ViewLayoutCommandForward(stCommand.id, stCommand.hMainWindow);
}

static BOOLEAN OperationHandleViewCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    const AppUiContext stUiContext = AppUiContextGet();
    auto stDisplayState = ViewDisplayStateGet();

    switch (stCommand.id)
    {
    case IDM_NOW_PAGE_REFRESH:
        ViewRedrawSetLine(-1);
        PreviewVisibalise(DocCurrentPageIndex(), FALSE);
        return TRUE;

    case IDM_SPACE_VIEW_TOGGLE:
        stDisplayState.dSpaceView = !(stDisplayState.dSpaceView);
        InitParamValue(INIT_SAVE, VL_SPACE_VIEW, stDisplayState.dSpaceView);
        MenuItemCheckOnOff(IDM_SPACE_VIEW_TOGGLE, stDisplayState.dSpaceView);
        OperationOnStatusBar();
        ViewRedrawSetLine(-1);
        return TRUE;

    case IDM_GRID_VIEW_TOGGLE:
        stDisplayState.bGridView = !(stDisplayState.bGridView);
        InitParamValue(INIT_SAVE, VL_GRID_VIEW, stDisplayState.bGridView);
        MenuItemCheckOnOff(IDM_GRID_VIEW_TOGGLE, stDisplayState.bGridView);
        ViewRedrawSetLine(-1);
        return TRUE;

    case IDM_RIGHT_RULER_TOGGLE:
        stDisplayState.bRightRulerView = !(stDisplayState.bRightRulerView);
        InitParamValue(INIT_SAVE, VL_R_RULER_VIEW, stDisplayState.bRightRulerView);
        MenuItemCheckOnOff(IDM_RIGHT_RULER_TOGGLE, stDisplayState.bRightRulerView);
        ViewRedrawSetLine(-1);
        return TRUE;

    case IDM_UNDER_RULER_TOGGLE:
        stDisplayState.bUnderRulerView = !(stDisplayState.bUnderRulerView);
        InitParamValue(INIT_SAVE, VL_U_RULER_VIEW, stDisplayState.bUnderRulerView);
        MenuItemCheckOnOff(IDM_UNDER_RULER_TOGGLE, stDisplayState.bUnderRulerView);
        ViewRedrawSetLine(-1);
        return TRUE;

    case IDM_REBER_DORESET:
        ToolBarBandReset(stCommand.hMainWindow);
        return TRUE;

    case IDM_TMPLT_GROUP_PREV:
    case IDM_TMPLT_GROUP_NEXT:
    case IDM_TMPL_GRID_INCREASE:
    case IDM_TMPL_GRID_DECREASE:
        if (stUiContext.bTemplateDocked)
        {
            if (IsWindowVisible(stUiContext.hInsertPaletteWindow))
            {
                FORWARD_WM_COMMAND(stUiContext.hInsertPaletteWindow, stCommand.id, stCommand.hControl, stCommand.codeNotify, SendMessage);
            }
            else if (IsWindowVisible(stUiContext.hBucketPaletteWindow))
            {
                FORWARD_WM_COMMAND(stUiContext.hBucketPaletteWindow, stCommand.id, stCommand.hControl, stCommand.codeNotify, SendMessage);
            }
        }
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOLEAN OperationHandlePageCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    switch (stCommand.id)
    {
    case IDM_PAGEL_ADD:
    case IDM_PAGEL_INSERT:
    case IDM_PAGEL_DELETE:
    case IDM_PAGEL_DUPLICATE:
    case IDM_PAGEL_UPFLOW:
    case IDM_PAGEL_DOWNSINK:
    case IDM_PAGEL_RENAME:
    case IDM_PAGE_PREV:
    case IDM_PAGE_NEXT:
        FORWARD_WM_COMMAND(AppPageListWindowGet(), stCommand.id, stCommand.hControl, stCommand.codeNotify, SendMessage);
        return TRUE;

    default:
        return FALSE;
    }
}

static BOOLEAN OperationHandleMiscCommands(const VIEW_COMMAND_REQUEST &stCommand)
{
    switch (stCommand.id)
    {
    case IDM_TESTCODE:
        TRACE(TEXT("기능 테스트"));
        return TRUE;

    default:
        return FALSE;
    }
}

VOID OperationOnCommand(HWND hWnd, INT id, HWND hWndCtl, UINT codeNotify)
{
    VIEW_COMMAND_REQUEST stCommand{};

    stCommand.hMainWindow = hWnd;
    stCommand.id = id;
    stCommand.hControl = hWndCtl;
    stCommand.codeNotify = codeNotify;

    if (OperationHandleDynamicCommands(stCommand))
    {
        return;
    }

    for (const auto pfnDispatch : gapfViewCommandDispatchers)
    {
        if (pfnDispatch(stCommand))
            return;
    }

    TRACE(TEXT("未実装"));
}