// Sunday.h – 앱 전체 상수·구조체·함수 선언
// Copyright (C) 2011-2014 Orinrin/SikigamiHNQ – GPLv3+
//-------------------------------------------------------------------------------------------------

#pragma once

#define STRICT

#include "resource.h"
//-------------------------------------------------------------------------------------------------

#include "AppCommandRegistry.h"
#include "TextEncoding.h"
//-------------------------------------------------------------------------------------------------

// MLT 페이지 구분 문자
#define MLT_SEPARATERW    TEXT("[SPLIT]")
#define MLT_SEPARATERA    ("[SPLIT]")
inline constexpr int MLT_SPRT_CCH = 7;

// AST 페이지 구분 문자
#define AST_SEPARATERW    TEXT("[AA]")
#define AST_SEPARATERA    ("[AA]")
inline constexpr int AST_SPRT_CCH = 4;

// AA 파레트용
#define TMPLE_BEGINW    TEXT("[ListName=")
#define TMPLE_ENDW        TEXT("[end]")

// 줄바꿈
#define CH_CRLFW    TEXT("\r\n")
#define CH_CRLFA    ("\r\n")
inline constexpr int CH_CRLF_CCH = 2;

// 파일 끝 표기
inline constexpr int  EOF_SIZE  = 5;
CONST  TCHAR    gatEOF[] = TEXT("[EOF]");
inline constexpr int  EOF_WIDTH = 39;

inline constexpr int YY2_CRLF  = 6;    // YY 게시판 개행 바이트
inline constexpr int STRB_CRLF = 4;    // 시타라바 게시판 개행 바이트

inline constexpr int PAGE_BYTE_MAX = 8192;    // 1레스 최대 바이트

#define MODIFY_MSG    TEXT("[수정됨]")
//-------------------------------------------------------------------------------------------------

// 시타라바 색지정 태그
#define COLOUR_TAG_WHITE    TEXT("[spo]")
#define COLOUR_TAG_CLOSPO    TEXT("[/spo]")
#define COLOUR_TAG_CLOCLR    TEXT("[/clr]")

//-------------------------------------------------------------------------------------------------



#define CC_TAB    0x09
#define CC_CR    0x0D
#define CC_LF    0x0A

//-----------------------------------------------------------------------------------------------------------------------------------------

//アンドゥ用COMMANDO
#define DO_INSERT    1 // アンドゥ用COMMANDO　文字入力・ペーストとか
#define DO_DELETE    2 // アンドゥ用COMMANDO　文字削除・切り取りとか

//-----------------------------------------------------------------------------------------------------------------------------------------


#define LINE_HEIGHT    18 // ＡＡの壱行の高さドット

#define RULER_AREA    13 // 編集窓のルーラーエリア高さ

#define LINENUM_WID    37 // 編集窓の行番号表示エリアの幅
#define LINENUM_COLOUR        0xFF8000 // 編集窓の行番号表示エリアの色

#define RUL_LNNUM_COLOURBK    0xC0C0C0
//-------------------------------------------------------------------------------------------------

// 스페이스 폭 (dot)
inline constexpr int SPACE_HAN = 5;    // 반각 스페이스
inline constexpr int SPACE_ZEN = 11;    // 전각 스페이스
//-------------------------------------------------------------------------------------------------

// 기본 색상 상수 (BGR)
inline constexpr COLORREF CLR_BLACK   = 0x000000;
inline constexpr COLORREF CLR_MAROON  = 0x000080;
inline constexpr COLORREF CLR_GREEN   = 0x008000;
inline constexpr COLORREF CLR_OLIVE   = 0x008080;
inline constexpr COLORREF CLR_NAVY    = 0x800000;
inline constexpr COLORREF CLR_PURPLE  = 0x800080;
inline constexpr COLORREF CLR_TEAL    = 0x808000;
inline constexpr COLORREF CLR_GRAY    = 0x808080;
inline constexpr COLORREF CLR_SILVER  = 0xC0C0C0;
inline constexpr COLORREF CLR_RED     = 0x0000FF;
inline constexpr COLORREF CLR_LIME    = 0x00FF00;
inline constexpr COLORREF CLR_YELLOW  = 0x00FFFF;
inline constexpr COLORREF CLR_BLUE    = 0xFF0000;
inline constexpr COLORREF CLR_FUCHSIA = 0xFF00FF;
inline constexpr COLORREF CLR_AQUA    = 0xFFFF00;
inline constexpr COLORREF CLR_WHITE   = 0xFFFFFF;

//-------------------------------------------------------------------------------------------------

// 문자 스타일 플래그
inline constexpr UINT CT_NORMAL    = 0x0000;    // 보통 문자
inline constexpr UINT CT_WARNING   = 0x0001;    // 경고(연속 반각 공백 등)
inline constexpr UINT CT_SPACE     = 0x0002;    // 공백
inline constexpr UINT CT_SELECT    = 0x0004;    // 선택 상태
inline constexpr UINT CT_NOSJISREF = 0x0008;    // CP932 밖 문자라 숫자 참조가 필요한 문자
inline constexpr UINT CT_LYR_TRNC  = 0x0010;    // 레이어 투과 범위
inline constexpr UINT CT_FINDED    = 0x0020;    // 검색 히트
inline constexpr UINT CT_NOSJIS    = 0x0040;    // CP932 불가 문자

inline constexpr UINT CT_SELRTN    = 0x0100;    // 행말 개행도 선택
inline constexpr UINT CT_LASTSP    = 0x0200;    // 행말 공백
inline constexpr UINT CT_RETURN    = 0x0400;    // 개행 필요
inline constexpr UINT CT_EOF       = 0x0800;    // 끝
inline constexpr UINT CT_FINDRTN   = 0x1000;    // 행말 개행 검색 히트
//-------------------------------------------------------------------------------------------------



inline constexpr int OPENHIST_MAX = 12;

// 파일 열기 기록
struct OPENHIST
{
    TCHAR    atFile[MAX_PATH]{};    // 파일 경로
    DWORD    dMenuNumber{};        // 메뉴 번호
};
using LPOPENHIST = OPENHIST*;
using OPHIS_ITR  = list<OPENHIST>::iterator;
//----------------

// 1문자 정보
struct LETTER
{
    TCHAR    cchMozi{};        // 문자
    INT        rdWidth{};        // 문자 폭(dot)
    UINT    mzStyle{};        // 문자 타입
    CHAR    acNoSjisText[10]{};    // CP932 밖 문자는 숫자 참조로 담는 호환 텍스트 조각
    INT_PTR    mzByte{};        // 호환 저장용 텍스트 바이트 수
};
using LPLETTER = LETTER*;
using LETR_ITR = vector<LETTER>::iterator;
//-----------------------------

inline constexpr int PARTS_CCH = 130;

// 프레임 파츠 데이터
struct FRAMEITEM
{
    TCHAR    atParts[PARTS_CCH]{};    // 파츠 문자열 (9자까지)
    INT        dDot{};                    // 가로 폭(dot)
    INT        iLine{};                // 사용 행 수
    INT        iNowLn{};                // 배치 시 사용 행 번호
};
using LPFRAMEITEM = FRAMEITEM*;
//----------------

// 프레임 정보
struct FRAMEINFO
{
    TCHAR    atFrameName[MAX_STRING]{};

    FRAMEITEM    stDaybreak{};    // 좌측    │
    FRAMEITEM    stMorning{};    // 좌상    ┌
    FRAMEITEM    stNoon{};        // 상단    ─
    FRAMEITEM    stAfternoon{};    // 우상    ┐
    FRAMEITEM    stNightfall{};    // 우측    │
    FRAMEITEM    stTwilight{};    // 우하    ┘
    FRAMEITEM    stMidnight{};    // 하단    ─
    FRAMEITEM    stDawn{};        // 좌하    └

    INT        dLeftOffset{};    // 좌측 여백
    INT        dRightOffset{};    // 우측 여백
    UINT    dRestPadd{};        // 나머지 채우기
};
using LPFRAMEINFO = FRAMEINFO*;
//-----------------------------

// 트레이스 모드 파라미터
struct TRACEPARAM
{
    POINT    stOffsetPt{};        // 위치 오프셋
    INT        dContrast{};        // 콘트라스트
    INT        dGamma{};            // 감마
    INT        dGrayMoph{};        // 담색
    INT        dZooming{};            // 확대축소
    INT        dTurning{};            // 회전
    UINT    bUpset{};
    UINT    bMirror{};
    COLORREF    dMoziColour{};    // 문자색
};
using LPTRACEPARAM = TRACEPARAM*;
//----------------


// 조작 로그
struct OPERATELOG
{
    UINT    dCommando{};    // 조작 타입
    UINT    ixSequence{};    // 조작 번호
    UINT    ixGroup{};        // 조작 그룹 (1인덱스)
    INT        rdXdot{};        // 조작 위치(dot)
    INT        rdYline{};        // 조작 행
    LPTSTR    ptText{};        // 조작된 문자열
    UINT    cchSize{};        // 문자열 길이
};
using LPOPERATELOG = OPERATELOG*;
using OPSQ_ITR    = vector<OPERATELOG>::iterator;
//-----------------------------

// 언두 버퍼
struct UNDOBUFF
{
    UINT_PTR    dNowSqn{};    // 현재 조작 위치
    UINT        dTopSqn{};    // 최신 조작 번호 (1인덱스)
    UINT        dGrpSqn{};    // 조작 그룹 (1인덱스)
    vector<OPERATELOG>    vcOpeSqn;
};
using LPUNDOBUFF = UNDOBUFF*;
//-----------------------------

// 1행 관리
struct ONELINE
{
    INT        iDotCnt{};        // 도트 수
    INT        iByteSz{};        // 바이트 수
    UINT    dStyle{};        // 행 특수 상태
    BOOLEAN    bBadSpace{};    // 경고 공백 여부
    vector<LETTER>    vcLine;    // 행 내용 (개행 미포함)
    INT        dFrtSpDot{};    // 선두 공백(dot) – 레이어용
    INT        dFrtSpMozi{};    // 선두 공백(문자 수)
};
using LPONELINE = ONELINE*;
using LINE_ITR  = list<ONELINE>::iterator;
//-----------------------------

// SPLIT 페이지 1건
struct ONEPAGE
{
    TCHAR    atPageName[SUB_STRING]{};
    INT        dByteSz{};            // 바이트 수
    INT        dSelLineTop{};        // 선택 범위 상단 행
    INT        dSelLineBottom{};    // 선택 범위 하단 행
    BOOLEAN    bVisited{};        // 사용자가 직접 연 적 있는지
    UNDOBUFF    stUndoLog{};        // 언두 로그
    list<ONELINE>    ltPage;        // 행 전체
    LPTSTR    ptRawData{};        // 원시 데이터
    INT        iLineCnt{};            // 행 수
    INT        iMoziCnt{};            // 문자 수
};
using LPONEPAGE = ONEPAGE*;
using PAGE_ITR  = vector<ONEPAGE>::iterator;
//-----------------------------

struct DOCLINEMETRICS
{
    INT    dDotCnt{};
    INT    iLetterCnt{};
    INT    dByteCnt{};
};

struct DOCSELRANGE
{
    INT    dTop{-1};
    INT    dBottom{-1};
};

// 1파일 보유
struct ONEFILE
{
    TCHAR    atFileName[MAX_PATH]{};    // 파일명
    UINT    dModify{};                // 변경 여부
    LPARAM    dUnique{};                // 통번 (1인덱스)
    TCHAR    atDummyName[MAX_PATH]{};    // 파일명 없을 때 가명
    INT        dNowPage{};                // 현재 페이지
    POINT    stCaret{};                // 캐릿 위치 (dot, 행)
    vector<ONEPAGE>    vcCont;        // 페이지 보유
};
using LPONEFILE = ONEFILE*;
using FILES_ITR = list<ONEFILE>::iterator;
//-----------------------------

//    複数ファイル扱うなら、さらにコレを包含すればいい？

// 1행 템플릿 (카테고리명 포함)
struct AATEMPLATE
{
    TCHAR    atCtgryName[SUB_STRING]{};    // 세트명
    vector<wstring>    vcItems;            // 템플릿 문자열
};
using LPAATEMPLATE = AATEMPLATE*;
using TEMPL_ITR    = vector<AATEMPLATE>::iterator;
//-----------------------------

// 리바 위치 확정용
struct REBARLAYOUTINFO
{
    UINT    wID{};
    UINT    cx{};
    UINT    fStyle{};
};
using LPREBARLAYOUTINFO = REBARLAYOUTINFO*;
//-----------------------------


// 페이지 정보 확보용
struct PAGEINFOS
{
    UINT    dMasqus{};                    // 필요한 정보 플래그
    INT_PTR    iLines{};                    // 행 수
    INT_PTR    iBytes{};                    // 바이트 수
    INT_PTR    iMozis{};                    // 문자 수
    TCHAR    atPageName[SUB_STRING]{};
};
using LPPAGEINFOS = PAGEINFOS*;

inline constexpr UINT PI_LINES   = 0x01;
inline constexpr UINT PI_BYTES   = 0x02;
inline constexpr UINT PI_MOZIS   = 0x04;
inline constexpr UINT PI_NAME    = 0x08;
inline constexpr UINT PI_RECALC  = 0x80000000;
//-----------------------------

//-------------------------------------------------------------------------------------------------

// 페이지 로드 콜백
using PAGELOAD = UINT (CALLBACK*)(LPTSTR, LPCTSTR, INT);




#ifdef USE_HOVERTIP

//!    HoverTip用の表示内容確保・内容は増やすかも
//typedef struct tagHOVERTIPINFO
//{
//    LPTSTR    ptInfo;    //    文字列内容を示すポインタ
//
//} HOVERTIPINFO, *LPHOVERTIPINFO;

// HoverTip 콜백
using HOVERTIPDISP = LPTSTR (CALLBACK*)( LPVOID );

HRESULT    HoverTipInitialise( HINSTANCE, HWND );
HRESULT    HoverTipFontRefresh( VOID );
HRESULT    HoverTipResist( HWND  );
LRESULT    HoverTipOnMouseHover( HWND, WPARAM, LPARAM, HOVERTIPDISP );
LRESULT    HoverTipOnMouseLeave( HWND );


#endif
//-------------------------------------------------------------------------------------------------


//    このコード モジュールに含まれる関数の宣言
INT_PTR        CALLBACK About( HWND, UINT, WPARAM, LPARAM );

VOID        WndTagSet( HWND, LONG_PTR );
LONG_PTR    WndTagGet( HWND );

#ifdef SPMOZI_ENCODE
UINT        IsSpMozi( TCHAR );
#endif

HRESULT        InitWindowPos( UINT, UINT, LPRECT );
INT            InitParamValue( UINT, UINT, INT );
HRESULT        InitParamString( UINT, UINT, LPTSTR );

HRESULT        InitProfHistory( UINT, UINT, LPTSTR );

INT            ViewStringWidthGet( LPCTSTR );
INT            ViewLetterWidthGet( TCHAR );
// 2024kai
INT GetHdcC();
INT ReleaseHdcC();
INT ViewLetterWidthGetC(TCHAR);

BOOLEAN        DocBuildNoSjisText( TCHAR, LPSTR );
INT_PTR        DocLetterByteCheck( LPLETTER );
INT_PTR        DocLetterDataCheck( LPLETTER, TCHAR );

BOOLEAN        FileExtensionCheck( LPTSTR, LPTSTR );

HRESULT        ViewingFontGet( LPLOGFONT );
HFONT        AppUiFontGet( VOID );
HFONT        AppUiDropDownFontGet( VOID );
HRESULT        AppUiFontApply( HWND, BOOL );

LPTSTR        FindStringProc( LPTSTR, LPTSTR, LPINT );

UINT        UnicodeUseToggle( LPVOID  );

LPSTR        LegacyEncodedTextEncodeAlloc( LPCTSTR  );

ATOM        InitWndwClass( HINSTANCE  );
BOOL        InitInstance( HINSTANCE , INT, LPTSTR );
LRESULT        CALLBACK WndProc( HWND , UINT, WPARAM, LPARAM );

HRESULT        MainStatusBarSetText( INT, LPCTSTR );
HRESULT        MainSttBarSetByteCount( UINT  );

HRESULT        WindowPositionReset( HWND );

HRESULT        MenuItemCheckOnOff( UINT, UINT );
HRESULT        NotifyBalloonExist( LPCTSTR, LPCTSTR, DWORD );

HRESULT        WindowFocusChange( INT, INT );

HRESULT        OptionDialogueOpen( VOID  );

COLORREF    InitColourValue( UINT, UINT, COLORREF );
INT            InitTraceValue( UINT, LPTRACEPARAM );
//HRESULT    InitLastOpen( UINT, LPTSTR );            //    
INT            InitWindowTopMost( UINT, UINT, INT );
HRESULT        InitToolBarLayout( UINT, INT, LPREBARLAYOUTINFO );

#ifdef ACCELERATOR_EDIT
LPACCEL        AccelKeyTableGetAlloc( LPINT  );
LPACCEL        AccelKeyTableLoadAlloc( LPINT );
HRESULT        AccelKeyDlgOpen( HWND );
HACCEL        AccelKeyHandleGet( HINSTANCE  );

HACCEL        AccelKeyTableCreate( LPACCEL, INT );
HRESULT        AccelKeyMenuRewrite( HWND, LPACCEL, CONST INT );
#endif

HRESULT        OpenHistoryInitialise( HWND );
HRESULT        OpenHistoryLogging( HWND , LPTSTR );
HRESULT        OpenHistoryLoad( HWND, INT );

VOID        ToolBarCreate( HWND, HINSTANCE );
HRESULT        ToolBarInfoChange( LPACCEL, INT );
VOID        ToolBarDestroy( VOID  );
HRESULT        ToolBarSizeGet( LPRECT );
HRESULT        ToolBarCheckOnOff( UINT, UINT );
HRESULT        ToolBarOnSize( HWND, UINT, INT, INT );
LRESULT        ToolBarOnNotify( HWND, INT, LPNMHDR );
LRESULT        ToolBarOnContextMenu( HWND , HWND, LONG, LONG );
VOID        ToolBarPseudoDropDown( HWND , INT );
UINT        ToolBarBandInfoGet( LPVOID );
HRESULT        ToolBarBandReset( HWND );

UINT        AppClientAreaCalc( LPRECT );

HRESULT        AppTitleChange( LPTSTR );
HRESULT        AppTitleTrace( UINT );
HRESULT        AppBackgroundPageLoadRestart( HWND );
HRESULT        AppBackgroundPageLoadStop( HWND );
HRESULT        AppMigrateLegacySettings( LPCTSTR );

struct JSON_FLIP_ENTRY
{
    wstring    wsSource;
    wstring    wsTarget;
};

struct JSON_TEMPLATE_GROUP
{
    wstring    wsName;
    vector<wstring>    vcItems;
};

struct JSON_FRAME_RECORD
{
    wstring    wsName;
    wstring    wsDaybreak;
    wstring    wsMorning;
    wstring    wsNoon;
    wstring    wsAfternoon;
    wstring    wsNightfall;
    wstring    wsTwilight;
    wstring    wsMidnight;
    wstring    wsDawn;
    INT        dLeftOffset{};
    INT        dRightOffset{};
    BOOLEAN    bRestPadding{};
};

BOOLEAN        AppReadTextFile( LPCTSTR, wstring * );
BOOLEAN        AppWriteUtf8TextFile( LPCTSTR, const wstring & );
HRESULT        AppLoadFlipEntries( LPCTSTR, BOOLEAN, vector<JSON_FLIP_ENTRY> * );
HRESULT        AppWriteFlipEntries( LPCTSTR, const vector<JSON_FLIP_ENTRY> &, const vector<JSON_FLIP_ENTRY> & );
HRESULT        AppLoadPaletteGroups( LPCTSTR, BOOLEAN, vector<JSON_TEMPLATE_GROUP> * );
HRESULT        AppWritePaletteGroups( LPCTSTR, const vector<JSON_TEMPLATE_GROUP> &, const vector<JSON_TEMPLATE_GROUP> & );
HRESULT        AppLoadFrameRecords( LPCTSTR, vector<JSON_FRAME_RECORD> * );
HRESULT        AppWriteFrameRecords( LPCTSTR, const vector<JSON_FRAME_RECORD> & );

LPTSTR        ExePathGet( VOID  );

HRESULT        FrameInitialise( LPTSTR, HINSTANCE ); // 프레임 설정 JSON 경로 확보 후 초기화
HRESULT        FrameNameModifyPopUp( HMENU, UINT );
INT_PTR        FrameEditDialogue( HINSTANCE, HWND, UINT );
HRESULT        FrameNameLoad( UINT, LPTSTR, UINT_PTR ); // 指定された枠の名前を返す
UINT        FrameCountGet( VOID );

HWND        FrameInsBoxCreate( HINSTANCE, HWND );
HRESULT        FrameMoveFromView( HWND, UINT );

HRESULT        CntxEditInitialise( LPTSTR, HINSTANCE );
HRESULT        CntxEditDlgOpen( HWND );
HMENU        CntxMenuGet( VOID );
HINSTANCE    CntxInstanceGet( VOID );
LPCTSTR        CntxIniPathGet( VOID );
UINT        CntxItemCountGet( VOID );
UINT        CntxItemCommandGet( UINT );
LPCTSTR        CntxItemLabelGet( UINT );
HRESULT        CntxCommandNameCopy( UINT, LPTSTR, size_t );

HRESULT        AccelKeyTextBuild( LPTSTR, UINT_PTR, DWORD, LPACCEL, INT );

HRESULT        MultiFileTabFirst( LPTSTR );
HRESULT        MultiFileTabAppend( LPARAM, LPTSTR );
HRESULT        MultiFileTabSelect( LPARAM );
HRESULT        MultiFileTabSelectDocument( LPARAM );
HRESULT        MultiFileTabSlide( INT );
HRESULT        MultiFileTabRename( LPARAM, LPTSTR );
HRESULT        MultiFileTabClose( INT );
INT            MultiFileTabSearch( LPARAM );
INT            InitMultiFileTabOpen( UINT, INT, LPTSTR );

VOID        OperationOnCommand( HWND, INT, HWND, UINT );

VOID        AaFontCreate( UINT );

HWND        ViewInitialise( HINSTANCE, HWND, LPRECT, LPTSTR );
HRESULT        ViewSizeMove( HWND, LPRECT );
HRESULT        ViewFocusSet( VOID );

BOOL        ViewShowCaret( VOID );
VOID        ViewHideCaret( VOID );
INT            ViewCaretPosGet( PINT, PINT );

HRESULT        ViewFrameInsert( INT  );

HRESULT        ViewNowPosStatus( VOID );

HRESULT        ViewRedrawSetLine( INT );
HRESULT        ViewRedrawSetRect( LPRECT );
HRESULT        ViewRedrawSetVartRuler( INT );
HRESULT        ViewRulerRedraw( INT, INT );
HRESULT        ViewEditReset( VOID );

COLORREF    ViewMoziColourGet( LPCOLORREF );
COLORREF    ViewBackColourGet( LPVOID );

HRESULT        ViewCaretCreate( HWND, COLORREF, COLORREF );
HRESULT        ViewCaretDelete( VOID );
BOOLEAN        ViewDrawCaret( INT, INT , BOOLEAN ); // 本当はドローじゃなくてポジションチェンジだけ
BOOLEAN        ViewPosResetCaret( INT, INT );
HRESULT        ViewCaretReColour( COLORREF );

HRESULT        ViewPositionTransform( PINT, PINT, BOOLEAN );
BOOLEAN        ViewIsPosOnFrame( INT, INT );
INT            ViewAreaSizeGet( PINT );

HRESULT        ViewSelPositionSet( LPVOID );
HRESULT        ViewSelMoveCheck( UINT );
UINT        ViewSelRangeCheck( UINT );
UINT        ViewSelBackCheck( INT );
INT            ViewSelPageAll( INT );
UINT        ViewSqSelModeToggle( UINT, LPVOID );
HRESULT        ViewSelAreaSelect( LPVOID );

// ViewInsertUniSpace, ViewInsertTmpleString, ViewBrushStyleSetting,
// ViewLayoutCommandForward 는 EditorController.h 로 이동함.
INT            ViewInsertColourTag( UINT );
HRESULT        DocInsertSpoTag( UINT );
HRESULT        ColourTagDialogueOpen( HINSTANCE, HWND );

VOID        Evw_OnMouseMove( HWND, INT, INT, UINT );
VOID        Evw_OnLButtonDown( HWND, BOOL, INT, INT, UINT );
VOID        Evw_OnLButtonUp( HWND, INT, INT, UINT );
VOID        Evw_OnRButtonDown( HWND, BOOL, INT, INT, UINT );

VOID        Evw_OnKey( HWND, UINT, BOOL, INT, UINT );
VOID        Evw_OnChar( HWND, TCHAR, INT );
VOID        Evw_OnMouseWheel( HWND, INT, INT, INT, UINT );

VOID        Evw_OnImeComposition( HWND, WPARAM, LPARAM );

BOOLEAN        IsSelecting( PUINT );

HRESULT        OperationOnStatusBar( VOID );

HWND        PageListInitialise( HINSTANCE, HWND, LPRECT );
VOID        PageListResize( HWND , LPRECT );
HRESULT        PageListClear( VOID );
HRESULT        PageToolBarInfoChange( LPACCEL, INT );
HRESULT        PageListInsert( INT );
HRESULT        PageListDelete( INT );
HRESULT        PageListViewChange( INT , INT );
HRESULT        PageListInfoSet( INT, INT, INT );
HRESULT        PageListNameSet( INT , LPTSTR );
INT            PageListIsNamed( FILES_ITR );
HRESULT        PageListPositionReset( HWND );

HRESULT        PageListViewRewrite( INT  );
HRESULT        PageListBuild( LPVOID );


HRESULT        TemplateItemLoad( LPCTSTR, PAGELOAD );

INT            UserItemInitialise( HWND, UINT );
HRESULT        UserItemInsert( HWND, UINT );
HRESULT        UserItemNameGet( UINT, LPTSTR, UINT_PTR );
UINT        UserItemCountGet( VOID );

inline constexpr UINT MENU_PICKER_FRAME = 1;
inline constexpr UINT MENU_PICKER_USERITEM = 2;
inline constexpr UINT MENU_PICKER_VISIBLE_MAX = 10;
inline constexpr UINT MENU_PICKER_MENU_GROUP_MAX = 20;
inline constexpr UINT MENU_DYNAMIC_FRAME_FIRST = 51000;
inline constexpr UINT MENU_DYNAMIC_FRAME_LAST = 54999;
inline constexpr UINT MENU_DYNAMIC_USER_FIRST = 55000;
inline constexpr UINT MENU_DYNAMIC_USER_LAST = 58999;
HRESULT        MenuPickerInitialise( HINSTANCE );
HRESULT        MenuPickerShow( HWND, UINT, INT, INT );
HRESULT        MenuPickerCreatePagedMenu( HMENU*, UINT, UINT );

HRESULT        FrameNameModifyMenu( HWND );

VOID        PreviewInitialise( HINSTANCE, HWND );
HRESULT        PreviewVisibalise( INT, BOOLEAN );

INT            TraceInitialise( HWND, UINT );
HRESULT        TraceDialogueOpen( HINSTANCE, HWND );
HRESULT        TraceImgViewTglExt( VOID );
UINT        TraceImageAppear( HDC, INT, INT );
UINT        TraceMoziColourGet( LPCOLORREF );

HRESULT        ImageFileSaveDC( HDC, LPTSTR, INT );


VOID        LayerBoxInitialise( HINSTANCE, LPRECT );
HRESULT        LayerBoxAlphaSet( UINT );
HRESULT        LayerMoveFromView( HWND, UINT );
HWND        LayerBoxVisibalise( HINSTANCE, LPCTSTR, UINT );
INT            LayerHeadSpaceCheck( vector<LETTER> *, PINT );
HRESULT        LayerTransparentToggle( HWND, UINT );
HRESULT        LayerContentsImportable( HWND, UINT, LPINT, LPINT, UINT );
HRESULT        LayerBoxPositionChange( HWND , LONG, LONG );
HRESULT        LayerStringReplace( HWND , LPTSTR );



HRESULT        DocInitialise( UINT );

BOOLEAN        DocRangeIsError( FILES_ITR , INT, INT );

UINT_PTR    DocNowFilePageCount( VOID );
UINT_PTR    DocNowFilePageLineCount( VOID );

UINT        DocRawDataParamGet( LPCTSTR, PINT, PINT );

VOID        DocCaretPosMemory( UINT , LPPOINT );

HRESULT        DocOpenFromNull( HWND );

DOCLINEMETRICS DocLineMetricsCalculate( const ONELINE & );
INT            DocLetterPosAdjustInLine( const ONELINE &, PINT, INT );
DOCSELRANGE    DocSelectionRangeNormalize( INT, INT );
DOCSELRANGE    DocSelectionRangeCalculate( const ONEPAGE & );

UINT        DocPageParamGet( PINT, PINT );
UINT        DocPageByteCount( FILES_ITR , INT, PINT, PINT );
HRESULT        DocPageInfoRenew( INT, UINT );
INT            DocPageMaxDotGet( INT, INT );
HRESULT        DocPageNameSet( LPTSTR );

INT            DocPageCreate( INT );
HRESULT        DocPageDelete( INT, INT );
HRESULT        DocPageChange( INT );

UINT        DocDelayPageLoad( FILES_ITR , INT ); // ディレイ頁のロード

HRESULT        DocModifyContent( UINT );

LPARAM        DocMultiFileCreate( LPTSTR );
HRESULT        DocActivateEmptyCreate( LPTSTR );

INT            DocLineParamGet( INT , PINT, PINT );

UINT        DocBadSpaceCheck( INT );
BOOLEAN        DocBadSpaceIsExist( INT );

INT            DocInputLetter( INT, INT, TCHAR );
INT            DocInputBkSpace( PINT, PINT );
INT            DocInputDelete( INT , INT );
INT            DocInputFromClipboard( PINT, PINT, PINT, UINT );

INT            DocAdditionalLine( INT, PBOOLEAN );

INT            DocStringAdd( PINT, PINT, LPCTSTR, INT );
HRESULT        DocCrLfAdd( INT, INT, BOOLEAN );
INT            DocSquareAdd( PINT, PINT, LPCTSTR, INT, LPPOINT * );
INT            DocStringErase( INT, INT, LPTSTR, INT );

INT            DocInsertLetter( PINT, INT, TCHAR );
INT            DocInsertString( PINT, PINT, PINT, LPCTSTR, UINT, BOOLEAN );

INT            DocIterateDelete( LETR_ITR, INT );
HRESULT        DocLineCombine( INT );

HRESULT        DocLineErase( INT, PBOOLEAN );


HRESULT        DocFrameInsert( INT, INT );
HRESULT        DocScreenFill( LPTSTR );

INT            DocExClipSelect( UINT );
HRESULT        DocPageAllCopy( UINT );

INT            DocLetterShiftPos( INT, INT, INT, PINT, PBOOLEAN );
INT            DocLetterPosGetAdjust( PINT, INT, INT );

HRESULT        DocReturnSelStateToggle( INT, INT );
INT            DocRangeSelStateToggle( INT, INT, INT, INT );
UINT        DocLetterSelStateGet( INT, INT );
INT            DocPageSelStateToggle( INT );
HRESULT        DocSelRangeSet( INT, INT );
HRESULT        DocSelRangeGet( PINT, PINT );
HRESULT        DocSelRangeReset( PINT, PINT );
VOID        DocSelByteSet( INT );
//BOOLEAN        DocIsSelecting( VOID );

LPTSTR        DocClipboardDataGet( PUINT );
HRESULT        DocClipboardDataSet( LPVOID, INT, UINT );
BOOLEAN        DocBuildCopyEntityText( TCHAR, LPTSTR, UINT_PTR );

INT            DocLineDataGetAlloc( INT, INT, LPLETTER *, PINT, PUINT );

HRESULT        DocThreadDropCopy( VOID );

INT            DocPageTextGetAlloc( FILES_ITR, INT, UINT, LPVOID *, BOOLEAN );
INT            DocPageGetAlloc( UINT, LPVOID * );

INT            DocLineTextGetAlloc( FILES_ITR, INT, UINT, UINT, LPVOID * );

INT            DocSelectedDelete( PINT, PINT, UINT, BOOLEAN );
INT            DocSelectedBrushFilling( LPTSTR, PINT, PINT );
INT            DocSelectTextGetAlloc( UINT, LPVOID *, LPPOINT * );

HRESULT        DocExtractExecute( HINSTANCE  );

LPARAM        DocOpendFileCheck( LPTSTR );
HRESULT        DocFileSave( HWND, UINT );
HRESULT        DocFileOpen( HWND );
HRESULT        DocDoOpenFile( HWND, LPTSTR );
HRESULT        DocImageSave( HWND, UINT, HFONT );

UINT        DocStringSplitMLT( LPTSTR, INT, PAGELOAD );
UINT        DocStringSplitAST( LPTSTR, INT, PAGELOAD );

UINT        DocImportSplitASD( LPSTR, INT, PAGELOAD );

INT            DocLineStateCheckWithDot( INT, INT, PINT, PINT, PINT, PINT, PBOOLEAN );
HRESULT        DocRightGuideline( LPVOID );
INT            DocSpaceShiftProc( UINT, PINT, INT );
LPTSTR        DocPaddingSpaceMake( INT  );
LPTSTR        DocPaddingSpaceUni( INT, PINT, PINT, PINT );
LPTSTR        DocPaddingSpaceWithGap( INT, PINT, PINT );
LPTSTR        DocPaddingSpaceWithPeriod( INT, PINT, PINT, PINT, BOOLEAN );
HRESULT        DocLastSpaceErase( PINT , INT );
HRESULT        DocTopLetterInsert( TCHAR, PINT, INT );
HRESULT        DocLastLetterErase( PINT, INT );
HRESULT        DocTopSpaceErase( PINT, INT );

HRESULT        DocPositionShift( UINT, PINT, INT );
#ifdef DOT_SPLIT_MODE
HRESULT        DocCentreWidthShift( UINT vk, PINT, INT );
#endif
HRESULT        DocHeadHalfSpaceExchange( HWND );

LPTSTR        DocLastSpDel( vector<LETTER> * );

INT            DocDiffAdjBaseSet( INT );
INT            DocDiffAdjExec( PINT, INT );

VOID        ZeroONELINE( LPONELINE );
INT            DocStringInfoCount( LPCTSTR, UINT_PTR, PINT, PINT );

BOOLEAN        NowPageInfoGet( UINT, LPPAGEINFOS );

BOOLEAN        PageIsDelayed( FILES_ITR, UINT );

UINT        DocRangeDeleteByMozi( INT, INT, INT, INT, PBOOLEAN );

INT            DocUndoExecute( PINT, PINT );
INT            DocRedoExecute( PINT, PINT );

LPARAM        DocFileInflate( LPTSTR );
INT            DocFileCloseCheck( HWND );
HRESULT        DocClipLetter( TCHAR  );
VOID        DocBackupDirectoryInit( LPTSTR );
HRESULT        DocFileBackup( HWND );

HRESULT        DocMultiFileCloseAll( VOID );
LPARAM        DocMultiFileClose( HWND, LPARAM );
HRESULT        DocMultiFileSelect( LPARAM );
HRESULT        DocMultiFileModify( UINT  );
HRESULT        DocMultiFileStore( LPTSTR );
INT            DocMultiFileFetch( INT, LPTSTR, LPTSTR );
LPTSTR        DocMultiFileNameGet( INT  );

HRESULT        DocInverseInit( UINT  );
HRESULT        DocInverseTransform( UINT, UINT, PINT, INT );

HRESULT        SqnInitialise( LPUNDOBUFF );
HRESULT        SqnFreeAll( LPUNDOBUFF );
HRESULT        SqnSetting( VOID  );
UINT        SqnAppendLetter( LPUNDOBUFF, UINT, TCHAR, INT, INT, UINT );
UINT        SqnAppendString( LPUNDOBUFF, UINT, LPCTSTR, INT, INT, UINT );
UINT        SqnAppendSquare( LPUNDOBUFF, UINT, LPCTSTR, LPPOINT, INT, UINT );

HRESULT        UnicodeRadixExchange( VOID );

#ifdef FIND_STRINGS
HRESULT        FindDialogueOpen( HINSTANCE, HWND );
HRESULT        FindDirectly( HINSTANCE, HWND, INT );
//INT            FindStringJump( UINT, PINT, PINT, PINT );
#endif

LPCTSTR        NextLineW( LPCTSTR );
LPTSTR        NextLineW( LPTSTR );

LPSTR        NextLineA( LPSTR  );


INT        TextViewSizeGet( LPCTSTR, PINT );