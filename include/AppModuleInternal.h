#pragma once

#include "Sunday.h"
#include "AppLayoutInternal.h"
#include "AppUiContextInternal.h"

extern HINSTANCE   ghAppInst;
extern TCHAR       gatAppTitle[MAX_STRING];
extern TCHAR       gatMainWindowClass[MAX_STRING];
extern HWND        ghFileTabWnd;
extern WNDPROC     gpfOriginFileTabProc;
extern HWND        ghMainWnd;
extern HWND        ghViewWnd;
extern HWND        ghPgVwWnd;
extern HWND        ghPalInsertWnd;
extern HWND        ghPalBucketWnd;
extern HWND        ghMainSplitWnd;
extern HMENU       ghMainMenu;
extern HACCEL      ghMainAccelTable;
extern HWND        ghMainStatusBarWnd;
extern HBRUSH      ghMainStatusRedBrush;
extern UINT        gdBackupInterval;
extern UINT        gbUniPad;
extern UINT        gbNoSjisSkipHangul;
extern UINT        gbAutoBUmsg;
extern UINT        gbCrLfCode;
extern UINT        gbSaveMsgOn;
extern TCHAR       gatExePath[MAX_PATH];
extern TCHAR       gatIniPath[MAX_PATH];
extern HFONT       ghNameFont;
extern HMENU       ghHistyMenu;
extern INT         gbTmpltDock;
extern BOOLEAN     gbDockTmplView;
extern UINT        gdPageByteMax;
extern LONG        grdSplitPos;
extern UINT        gdGridXpos;
extern UINT        gdGridYpos;
extern UINT        gdRightRuler;
extern UINT        gdUnderRuler;
extern HWND        ghFindDlg;
extern HWND        ghColourTagDlg;
extern HWND        ghGradientTagDlg;
extern UINT        gbSpMoziEnc;
extern list<OPENHIST> gltOpenHist;

HRESULT AppUiResourcesInitialise( HINSTANCE );
VOID    AppUiResourcesFinalise( VOID );
HRESULT AppUiFontsInitialise( VOID );
VOID    AppUiFontsFinalise( VOID );
HRESULT AppTrayIconInitialise( HWND );
VOID    AppTrayIconFinalise( VOID );
BOOLEAN AppTrayIconHandleWindowMessage( HWND, UINT, WPARAM, LPARAM, LRESULT * );
VOID    AppBackgroundPageLoadTick( HWND );

HRESULT AppHandleDocumentOpenPath( HWND, LPTSTR );
HRESULT AppHandleDocumentSelect( LPARAM );
BOOLEAN AppHandleMainCommand( HWND, INT );

BOOLEAN Cls_OnCreate( HWND, LPCREATESTRUCT );
VOID    Cls_OnActivate( HWND, UINT, HWND, BOOL );
VOID    Cls_OnPaint( HWND );
VOID    Cls_OnCommand( HWND, INT, HWND, UINT );
VOID    Cls_OnSize( HWND, UINT, INT, INT );
VOID    Cls_OnMove( HWND, INT, INT );
VOID    Cls_OnDestroy( HWND );
LRESULT Cls_OnNotify( HWND, INT, LPNMHDR );
VOID    Cls_OnTimer( HWND, UINT );
VOID    Cls_OnDropFiles( HWND, HDROP );
VOID    Cls_OnContextMenu( HWND, HWND, UINT, UINT );
VOID    Cls_OnHotKey( HWND, INT, UINT, UINT );
VOID    Cls_OnDrawItem( HWND, CONST DRAWITEMSTRUCT * );
BOOL    Cls_OnWindowPosChanging( HWND, LPWINDOWPOS );
void    Cls_OnCopyData( HWND, HWND, PCOPYDATASTRUCT );

BOOLEAN AppModelessDialogHandleMessage( LPMSG );
VOID    AppModelessDialogsDestroy( VOID );
BOOLEAN AppModelessEditCommandForward( HWND, UINT );

HRESULT AppFileTabHandleSelectionChange( VOID );
BOOLEAN AppFileTabHandleContextMenu( HWND, HWND, const POINT * );
LPCTSTR AppFileTabTooltipTextGet( const POINT *, LPTSTR, UINT );

LRESULT CALLBACK gpfFileTabProc( HWND, UINT, WPARAM, LPARAM );
VOID    Ftb_OnMButtonUp( HWND, INT, INT, UINT );
