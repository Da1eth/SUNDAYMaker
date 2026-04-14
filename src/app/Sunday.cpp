// Copyright (C) 2011-2014 Orinrin/SikigamiHNQ – GPLv3+

#include "Sunday.h"
#include "Palette.h"
#include "AppModuleInternal.h"
#include "MenuMnemonic.h"
#include "UiText.h"

HINSTANCE ghAppInst;
TCHAR gatAppTitle[MAX_STRING];
TCHAR gatMainWindowClass[MAX_STRING];

HMENU ghMainMenu;
HACCEL ghMainAccelTable;
HWND ghFileTabWnd; // 다중 파일 탭
WNDPROC gpfOriginFileTabProc;

HWND ghMainWnd;
HWND ghMainStatusBarWnd;
HBRUSH ghMainStatusRedBrush;

EXTERNED HWND ghPgVwWnd;         // ページリスト
EXTERNED HWND ghPalInsertWnd;    // 壱行テンプレ
EXTERNED HWND ghPalBucketWnd;    // ブラシテンプレ
EXTERNED BOOLEAN gbDockTmplView; // くっついてるテンプレは見えているか

EXTERNED HWND ghMainSplitWnd; // メインのスプリットバーハンドル
EXTERNED LONG grdSplitPos;    // スプリットバーの、左側の、画面右からのオフセット

EXTERNED UINT gbUniPad;      // パディングにユニコードをつかって、ドットを見せないようにする
EXTERNED UINT gbUniRadixHex; // ユニコード数値参照が１６進数であるか

UINT gdBackupInterval;     // バックアップ間隔
EXTERNED UINT gbAutoBUmsg; // 自動バックアップメッセージ出すか？
EXTERNED UINT gbCrLfCode;  // 改行コード：０したらば・非０ＹＹ

EXTERNED UINT gbSaveMsgOn; // 保存メッセージ出すか？

EXTERNED UINT gdPageByteMax; // 壱レスの最大バイト数

TCHAR gatExePath[MAX_PATH]; // 実行ファイルの位置
TCHAR gatIniPath[MAX_PATH]; // ＩＮＩファイルの位置

EXTERNED INT gbTmpltDock; // テンプレのドッキング

list<OPENHIST> gltOpenHist; // ファイル開いた履歴
HMENU ghHistyMenu;          // 履歴表示する部分

#ifdef FIND_STRINGS
extern HWND ghFindDlg; //    検索ダイヤログのハンドル
#endif

extern HWND ghViewWnd; //    ビュー

extern UINT gdGridXpos;   //    グリッド線のＸ間隔
extern UINT gdGridYpos;   //    グリッド線のＹ間隔
extern UINT gdRightRuler; //    右線の位置ドット
extern UINT gdUnderRuler; //    下線の位置行数

#ifdef SPMOZI_ENCODE
extern UINT gbSpMoziEnc; // 機種依存文字を数値参照コピーする
#endif
//-------------------------------------------------------------------------------------------------

extern list<ONEFILE> gltMultiFiles;
extern FILES_ITR gitFileIt;
//-------------------------------------------------------------------------------------------------

#ifdef ACCELERATOR_EDIT

// アクセラテーブルをダイナミックに確保
LPACCEL AccelKeyTableGetAlloc(LPINT piEntry)
{
    INT iItems;
    LPACCEL pstAccel = nullptr;

    if (!(piEntry))
        return nullptr;
    *piEntry = 0;

    //    まず個数確保
    iItems = CopyAcceleratorTable(ghMainAccelTable, nullptr, 0);
    if (0 >= iItems)
        return nullptr;

    pstAccel = (LPACCEL)malloc(iItems * sizeof(ACCEL));
    if (!(pstAccel))
        return nullptr;

    iItems = CopyAcceleratorTable(ghMainAccelTable, pstAccel, iItems);

    *piEntry = iItems;

    return pstAccel;
}
//-------------------------------------------------------------------------------------------------

// 構造体テーブルを受け取って、それをアクセラハンドルにする・nullptrならResourceから
HACCEL AccelKeyTableCreate(LPACCEL pstAccel, INT iEntry)
{
    HACCEL hAccel;

    if (pstAccel)
    {
        DestroyAcceleratorTable(ghMainAccelTable); //    前のヤツぶっ壊して

        hAccel = CreateAcceleratorTable(pstAccel, iEntry);
    }
    else
    {
        hAccel = LoadAccelerators(ghAppInst, MAKEINTRESOURCE(IDC_SUNDAY));
    }

    ghMainAccelTable = hAccel; //    大域変数にいれる・本当はこういうやり方は良くないと思われ

    AccelKeyMenuRewrite(ghMainWnd, pstAccel, iEntry);

    return hAccel;
}
//-------------------------------------------------------------------------------------------------
#endif

// タイトルバーを変更する
HRESULT AppTitleChange(LPTSTR ptText)
{
    LPTSTR ptName;
    LPTSTR ptCurrentName;
    TCHAR atBuff[MAX_PATH];
    TCHAR atDummyName[MAX_PATH];

    if (ptText)
    {
        if (0 != ptText[0])
        {
            ptName = PathFindFileName(ptText);
        }
        else
        {
            ptCurrentName = nullptr;
            if (ghFileTabWnd)
            {
                const INT iCurrentTab = TabCtrl_GetCurSel(ghFileTabWnd);
                if (0 <= iCurrentTab)
                {
                    ptCurrentName = DocMultiFileNameGet(iCurrentTab);
                }
            }

            if (ptCurrentName && 0 != ptCurrentName[0])
            {
                ptName = PathFindFileName(ptCurrentName);
            }
            else
            {
                StringCchPrintf(atDummyName, MAX_PATH, TEXT("%s1.%s"),
                                NAME_DUMMY_NAME, NAME_DUMMY_EXT);
                ptName = atDummyName;
            }
        }
        StringCchPrintf(atBuff, MAX_PATH, TEXT("%s - %s"), gatAppTitle, ptName);
    }
    else
    {
        StringCchCopy(atBuff, MAX_PATH, gatAppTitle);
    }

    SetWindowText(ghMainWnd, atBuff);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// タイトルバーにトレスモード表記を漬けたり消したり
HRESULT AppTitleTrace(UINT bMode)
{
    static TCHAR atOrig[MAX_PATH];
    TCHAR atBuff[MAX_PATH];

    if (bMode)
    {
        GetWindowText(ghMainWnd, atOrig, MAX_PATH);
        StringCchPrintf(atBuff, MAX_PATH, TEXT("%s [트레이싱 모드]"), atOrig);
        SetWindowText(ghMainWnd, atBuff);
    }
    else
    {
        if (0 != atOrig[0])
        {
            SetWindowText(ghMainWnd, atOrig);
        }
        else
        {
            SetWindowText(ghMainWnd, gatAppTitle);
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 상태 표시줄에 문자열 추가
HRESULT MainStatusBarSetText(INT room, LPCTSTR ptText)
{
    StatusBar_SetText(ghMainStatusBarWnd, room, ptText);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ステータスバーにバイト数をオーナードローで乳力
HRESULT MainSttBarSetByteCount(UINT dByte)
{
    SendMessage(ghMainStatusBarWnd, SB_SETTEXT, (WPARAM)(SB_BYTECNT | SBT_OWNERDRAW), (LPARAM)dByte);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// メニュー項目をチェックしたり外したり
HRESULT MenuItemCheckOnOff(UINT itemID, UINT bCheck)
{
    CheckMenuItem(ghMainMenu, itemID, bCheck ? MF_CHECKED : MF_UNCHECKED);

    ToolBarCheckOnOff(itemID, bCheck);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 外部でブラシ機能ON/OFFした場合
HRESULT PaletteBucketModeToggle(VOID)
{
    LRESULT rlst;
    HMENU hSubMenu;

    rlst = SendMessage(ghPalBucketWnd, WMP_BRUSH_TOGGLE, 0, 0);
    hSubMenu = GetSubMenu(ghMainMenu, 1);
    CheckMenuItem(hSubMenu, IDM_BRUSH_STYLE, rlst ? MF_CHECKED : MF_UNCHECKED);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// メインウインドウのクライヤント領域を求める
UINT AppClientAreaCalc(LPRECT pstRect)
{
    RECT rect, sbRect, tbRect;
    RECT ftRect;

    if (!(pstRect))
        return 0;

    if (!(ghMainWnd))
    {
        SetRect(pstRect, 0, 0, 0, 0);
        return 0;
    }

    GetClientRect(ghMainWnd, &rect); //

    ToolBarSizeGet(&tbRect); //    ツールバーのサイズとって
    rect.top += tbRect.bottom;
    rect.bottom -= tbRect.bottom;

    GetClientRect(ghMainStatusBarWnd, &sbRect); //    ステータスバーのサイズ確認
    rect.bottom -= sbRect.bottom;

    GetWindowRect(ghFileTabWnd, &ftRect); //    タブバーのサイズ確認
    ftRect.bottom -= ftRect.top;
    SetWindowPos(ghFileTabWnd, HWND_TOP, 0, tbRect.bottom, rect.right, ftRect.bottom, SWP_NOZORDER);
    rect.top += ftRect.bottom;
    rect.bottom -= ftRect.bottom; //    タブバーの分縮める

    SetRect(pstRect, rect.left, rect.top, rect.right, rect.bottom);

    return 1;
}
//-------------------------------------------------------------------------------------------------

// トレスの各モードの数値
INT InitTraceValue(UINT dMode, LPTRACEPARAM pstInfo)
{
    TCHAR atBuff[MIN_STRING];
    INT iBuff = 0;

    if (dMode) //    ロード
    {
        iBuff = GetPrivateProfileInt(TEXT("Trace"), TEXT("Turning"), -1, gatIniPath);
        if (0 > iBuff)
            return 0;

        pstInfo->dTurning = iBuff;
        pstInfo->dZooming = GetPrivateProfileInt(TEXT("Trace"), TEXT("Zooming"), 0, gatIniPath);
        pstInfo->dGrayMoph = GetPrivateProfileInt(TEXT("Trace"), TEXT("GrayMoph"), 0, gatIniPath);
        pstInfo->dGamma = GetPrivateProfileInt(TEXT("Trace"), TEXT("Gamma"), 0, gatIniPath);
        pstInfo->dContrast = GetPrivateProfileInt(TEXT("Trace"), TEXT("Contrast"), 0, gatIniPath);
        pstInfo->stOffsetPt.y = GetPrivateProfileInt(TEXT("Trace"), TEXT("OffsetY"), 0, gatIniPath);
        pstInfo->stOffsetPt.x = GetPrivateProfileInt(TEXT("Trace"), TEXT("OffsetX"), 0, gatIniPath);
        pstInfo->bUpset = GetPrivateProfileInt(TEXT("Trace"), TEXT("Upset"), 0, gatIniPath);
        pstInfo->bMirror = GetPrivateProfileInt(TEXT("Trace"), TEXT("Mirror"), 0, gatIniPath);
        pstInfo->dMoziColour = GetPrivateProfileInt(TEXT("Trace"), TEXT("MoziColour"), 0, gatIniPath);
    }
    else //    セーブ
    {
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->stOffsetPt.x);
        WritePrivateProfileString(TEXT("Trace"), TEXT("OffsetX"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->stOffsetPt.y);
        WritePrivateProfileString(TEXT("Trace"), TEXT("OffsetY"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dContrast);
        WritePrivateProfileString(TEXT("Trace"), TEXT("Contrast"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dGamma);
        WritePrivateProfileString(TEXT("Trace"), TEXT("Gamma"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dGrayMoph);
        WritePrivateProfileString(TEXT("Trace"), TEXT("GrayMoph"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dZooming);
        WritePrivateProfileString(TEXT("Trace"), TEXT("Zooming"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstInfo->dTurning);
        WritePrivateProfileString(TEXT("Trace"), TEXT("Turning"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), pstInfo->bUpset);
        WritePrivateProfileString(TEXT("Trace"), TEXT("Upset"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), pstInfo->bMirror);
        WritePrivateProfileString(TEXT("Trace"), TEXT("Mirror"), atBuff, gatIniPath);

        StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), pstInfo->dMoziColour);
        WritePrivateProfileString(TEXT("Trace"), TEXT("MoziColour"), atBuff, gatIniPath);
    }

    return 1;
}
//-------------------------------------------------------------------------------------------------

// 色設定のセーブロード
COLORREF InitColourValue(UINT dMode, UINT dStyle, COLORREF nColour)
{
    TCHAR atKeyName[MIN_STRING], atBuff[MIN_STRING];
    LPTSTR ptEnd;

    switch (dStyle)
    {
    case CLRV_BASICPEN:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("BasicPen"));
        break; //    文字色
    case CLRV_BASICBK:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("BasicBack"));
        break; //    背景色
    case CLRV_GRIDLINE:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("GridLine"));
        break; //    グリッド
    case CLRV_CRLFMARK:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("CrLfMark"));
        break; //    改行マーク
    case CLRV_NOSJISBK:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("NoSjisBack"));
        break; //    유니코드 문자
    default:
        return nColour;
    }

    StringCchPrintf(atBuff, MIN_STRING, TEXT("%08X"), nColour);

    if (dMode) //    ロード
    {
        GetPrivateProfileString(TEXT("Colour"), atKeyName, atBuff, atBuff, MIN_STRING, gatIniPath);
        nColour = (COLORREF)_tcstoul(atBuff, &ptEnd, 16);
    }
    else //    セーブ
    {
        WritePrivateProfileString(TEXT("Colour"), atKeyName, atBuff, gatIniPath);
    }

    return nColour;
}
//-------------------------------------------------------------------------------------------------

// リバー配置のセーブロード
HRESULT InitToolBarLayout(UINT dMode, INT items, LPREBARLAYOUTINFO pstInfo)
{
    TCHAR atKeyName[MIN_STRING], atBuff[MIN_STRING];
    UINT dValue;
    INT i;

    if (dMode) //    ロード
    {
        //    存在確認
        dValue = GetPrivateProfileInt(TEXT("ReBar"), TEXT("IDX0_ID"), 0, gatIniPath);
        if (0 == dValue)
        {
            return E_NOTIMPL;
        } //    ＩＤなので０にはならない

        for (i = 0; items > i; i++) //    インデックス順
        {
            StringCchPrintf(atKeyName, MIN_STRING, TEXT("IDX%d_ID"), i);
            pstInfo[i].wID = GetPrivateProfileInt(TEXT("ReBar"), atKeyName, pstInfo[i].wID, gatIniPath);

            StringCchPrintf(atKeyName, MIN_STRING, TEXT("IDX%d_CX"), i);
            pstInfo[i].cx = GetPrivateProfileInt(TEXT("ReBar"), atKeyName, pstInfo[i].cx, gatIniPath);

            StringCchPrintf(atKeyName, MIN_STRING, TEXT("IDX%d_STYLE"), i);
            pstInfo[i].fStyle = GetPrivateProfileInt(TEXT("ReBar"), atKeyName, pstInfo[i].fStyle, gatIniPath);
        }
    }
    else //    セーブ
    {
        WritePrivateProfileSection(TEXT("ReBar"), nullptr, gatIniPath); //    一旦全削除

        for (i = 0; items > i; i++) //    インデックス順
        {
            StringCchPrintf(atKeyName, MIN_STRING, TEXT("IDX%d_ID"), i);
            StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), pstInfo[i].wID);
            WritePrivateProfileString(TEXT("ReBar"), atKeyName, atBuff, gatIniPath);

            StringCchPrintf(atKeyName, MIN_STRING, TEXT("IDX%d_CX"), i);
            StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), pstInfo[i].cx);
            WritePrivateProfileString(TEXT("ReBar"), atKeyName, atBuff, gatIniPath);

            StringCchPrintf(atKeyName, MIN_STRING, TEXT("IDX%d_STYLE"), i);
            StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), pstInfo[i].fStyle);
            WritePrivateProfileString(TEXT("ReBar"), atKeyName, atBuff, gatIniPath);
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// パラメータ値のセーブロード
INT InitParamValue(UINT dMode, UINT dStyle, INT nValue)
{
    TCHAR atKeyName[MIN_STRING], atLegacyKeyName[MIN_STRING], atBuff[MIN_STRING];
    INT iBuff = 0;
    atLegacyKeyName[0] = 0;

    switch (dStyle) //    INIT_LOAD
    {
    case VL_CLASHCOVER:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("ClashCover"));
        break;
    case VL_USE_UNICODE:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("UseUnicode"));
        break;
    case VL_LAYER_TRANS:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("LayerTrans"));
        break;
    case VL_LINETMP_CLM:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("LineTmplClm"));
        break;
    case VL_BRUSHTMP_CLM:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("BrushTmplClm"));
        break;
    case VL_UNIRADIX_HEX:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("UniRadixHex"));
        break;
    case VL_BACKUP_INTVL:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("BackUpIntvl"));
        break;
    case VL_BACKUP_MSGON:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("BackUpMsgOn"));
        break;
    case VL_CRLF_CODE:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("CrLfCode"));
        break;
    case VL_SPACE_VIEW:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("SpaceView"));
        break;
    case VL_GRID_X_POS:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("GridXpos"));
        break;
    case VL_GRID_Y_POS:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("GridYpos"));
        break;
    case VL_GRID_VIEW:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("GridView"));
        break;
    case VL_R_RULER_POS:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("RightRuler"));
        break;
    case VL_R_RULER_VIEW:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("RitRulerView"));
        break;
    case VL_PAGETIP_VIEW:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("PageToolTip"));
        break;
    case VL_PDELETE_NM:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("PageDeleteNM"));
        break;
    case VL_PLS_LN_DOCK:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("PLstLineDock"));
        break;
    case VL_SWAP_COPY:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("CopyModeSwap"));
        break;
    case VL_MAIN_SPLIT:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("MainSplit"));
        break;
    case VL_MAXIMISED:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("Maximised"));
        break;
    case VL_FIRST_READED:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("FirstStep"));
        break;
    case VL_LAST_OPEN:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("LastOpenStyle"));
        break;
    case VL_PGL_RETFCS:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("PageRetFocus"));
        break;
    case VL_U_RULER_POS:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("UnderRuler"));
        break;
    case VL_U_RULER_VIEW:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("UndRulerView"));
        break;
    case VL_MULTI_ACT_E:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("MultiAct"));
        break;
    case VL_SAVE_MSGON:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("SaveMsgOn"));
        break;
    case VL_SPMOZI_ENC:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("SpMoziEnc"));
        break; //    SPMOZI_ENCODE
    default:
        return nValue;
    }

    StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), nValue);

    if (dMode) //    ロード
    {
        atBuff[0] = 0;
        GetPrivateProfileString(TEXT("General"), atKeyName, TEXT(""), atBuff, MIN_STRING, gatIniPath);
        if (0 == atBuff[0] && 0 != atLegacyKeyName[0])
        {
            GetPrivateProfileString(TEXT("General"), atLegacyKeyName, TEXT(""), atBuff, MIN_STRING, gatIniPath);
        }
        if (0 == atBuff[0])
        {
            StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), nValue);
        }
        iBuff = StrToInt(atBuff);
    }
    else //    セーブ
    {
        WritePrivateProfileString(TEXT("General"), atKeyName, atBuff, gatIniPath);
        if (0 != atLegacyKeyName[0])
        {
            WritePrivateProfileString(TEXT("General"), atLegacyKeyName, nullptr, gatIniPath);
        }
    }

    return iBuff;
}
//-------------------------------------------------------------------------------------------------

// 文字列の設定内容をセーブロード
HRESULT InitParamString(UINT dMode, UINT dStyle, LPTSTR ptStr)
{
    TCHAR atKeyName[MIN_STRING], atDefault[MAX_PATH];

    if (!(ptStr))
        return E_INVALIDARG;

    switch (dStyle)
    {
    case VS_PROFILE_NAME:
        StringCchCopy(atKeyName, SUB_STRING, TEXT("ProfilePath"));
        break;
    default:
        return E_INVALIDARG;
    }

    if (dMode) //    ロード
    {
        StringCchCopy(atDefault, MAX_PATH, ptStr);
        GetPrivateProfileString(TEXT("General"), atKeyName, atDefault, ptStr, MAX_PATH, gatIniPath);
    }
    else
    {
        WritePrivateProfileString(TEXT("General"), atKeyName, ptStr, gatIniPath);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 最前面状態のセーブロード
INT InitWindowTopMost(UINT dMode, UINT dStyle, INT nValue)
{
    TCHAR atAppName[MIN_STRING], atBuff[MIN_STRING];
    INT iBuff = 0;

    switch (dStyle)
    {
    case WDP_PLIST:
        StringCchCopy(atAppName, SUB_STRING, TEXT("PageList"));
        break;
    case WDP_LNTMPL:
        StringCchCopy(atAppName, SUB_STRING, TEXT("PalInsert"));
        break;
    case WDP_BRTMPL:
        StringCchCopy(atAppName, SUB_STRING, TEXT("PalBucket"));
        break;
    default:
        return 0;
    }

    StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), nValue);

    if (dMode) //    ロード
    {
        GetPrivateProfileString(atAppName, TEXT("TopMost"), atBuff, atBuff, MIN_STRING, gatIniPath);
        iBuff = StrToInt(atBuff);
    }
    else //    セーブ
    {
        WritePrivateProfileString(atAppName, TEXT("TopMost"), atBuff, gatIniPath);
    }

    return iBuff;
}
//-------------------------------------------------------------------------------------------------

// ウインドウ位置のセーブロード
HRESULT InitWindowPos(UINT dMode, UINT dStyle, LPRECT pstRect)
{
    TCHAR atAppName[MIN_STRING], atBuff[MIN_STRING];

    if (!pstRect)
        return E_INVALIDARG;

    switch (dStyle)
    {
    case WDP_MVIEW:
        StringCchCopy(atAppName, SUB_STRING, TEXT("MainView"));
        break;
    case WDP_PLIST:
        StringCchCopy(atAppName, SUB_STRING, TEXT("PageList"));
        break;
    case WDP_LNTMPL:
        StringCchCopy(atAppName, SUB_STRING, TEXT("PalInsert"));
        break;
    case WDP_BRTMPL:
        StringCchCopy(atAppName, SUB_STRING, TEXT("PalBucket"));
        break;
    case WDP_PREVIEW:
        StringCchCopy(atAppName, SUB_STRING, TEXT("Preview"));
        break;
    default:
        return E_INVALIDARG;
    }

    if (dMode) //    ロード
    {
        GetPrivateProfileString(atAppName, TEXT("LEFT"), TEXT("0"), atBuff, MIN_STRING, gatIniPath);
        pstRect->left = StrToInt(atBuff);
        GetPrivateProfileString(atAppName, TEXT("TOP"), TEXT("0"), atBuff, MIN_STRING, gatIniPath);
        pstRect->top = StrToInt(atBuff);
        GetPrivateProfileString(atAppName, TEXT("RIGHT"), TEXT("0"), atBuff, MIN_STRING, gatIniPath);
        pstRect->right = StrToInt(atBuff);
        GetPrivateProfileString(atAppName, TEXT("BOTTOM"), TEXT("0"), atBuff, MIN_STRING, gatIniPath);
        pstRect->bottom = StrToInt(atBuff);
    }
    else //    セーブ
    {
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstRect->left);
        WritePrivateProfileString(atAppName, TEXT("LEFT"), atBuff, gatIniPath);
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstRect->top);
        WritePrivateProfileString(atAppName, TEXT("TOP"), atBuff, gatIniPath);
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstRect->right);
        WritePrivateProfileString(atAppName, TEXT("RIGHT"), atBuff, gatIniPath);
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%d"), pstRect->bottom);
        WritePrivateProfileString(atAppName, TEXT("BOTTOM"), atBuff, gatIniPath);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 枠情報のセーブロード
HRESULT WindowFocusChange(INT nowWnd, INT iDir)
{
    INT nextWnd;
    HWND hTargetWnd = nullptr;

    nextWnd = nowWnd + iDir;
    if (0 >= nextWnd)
        nextWnd = WND_BRUSH;
    if (WND_BRUSH < nextWnd)
        nextWnd = WND_MAIN;

    switch (nextWnd)
    {
    default:
    case WND_MAIN:
        SetForegroundWindow(ghMainWnd);
        return S_OK;
    case WND_PAGE:
        hTargetWnd = ghPgVwWnd;
        break;
    case WND_LINE:
        hTargetWnd = ghPalInsertWnd;
        break;
    case WND_BRUSH:
        hTargetWnd = ghPalBucketWnd;
        break;
    }

    if (gbTmpltDock)
    {
        SetForegroundWindow(ghMainWnd);
        SetFocus(hTargetWnd);
    }
    else
    {
        SetForegroundWindow(hTargetWnd);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// プロフ履歴をINIから読んだり書いたり
HRESULT InitProfHistory(UINT dMode, UINT dNumber, LPTSTR ptFile)
{
    TCHAR atKeyName[MIN_STRING], atDefault[MAX_PATH];

    if (dMode) //    ロード
    {
        ZeroMemory(ptFile, sizeof(TCHAR) * MAX_PATH);

        StringCchPrintf(atKeyName, MIN_STRING, TEXT("Hist%X"), dNumber);
        GetPrivateProfileString(TEXT("ProfHistory"), atKeyName, TEXT(""), atDefault, MAX_PATH, gatIniPath);

        if (0 == atDefault[0])
            return E_NOTIMPL; //    記録無し

        StringCchCopy(ptFile, MAX_PATH, atDefault);
    }
    else //    セーブ
    {
        if (ptFile)
        {
            StringCchPrintf(atKeyName, MIN_STRING, TEXT("Hist%X"), dNumber);
            WritePrivateProfileString(TEXT("ProfHistory"), atKeyName, ptFile, gatIniPath);
        }
        else //    一旦全削除
        {
            WritePrivateProfileSection(TEXT("ProfHistory"), nullptr, gatIniPath);
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 各窓の表示位置と大きさをデフォルトに戻す
HRESULT WindowPositionReset(HWND hWnd)
{
    HWND hWorkWnd;
    RECT rect;

    TRACE(TEXT("★창 표시 위치 리셋"));

    //    メイン窓
    hWorkWnd = GetDesktopWindow();
    GetWindowRect(hWorkWnd, &rect);
    rect.left = (rect.right - W_WIDTH) / 3; //    左より
    rect.top = (rect.bottom - W_HEIGHT) / 2;
    rect.right = W_WIDTH;
    rect.bottom = W_HEIGHT;
    //    位置変更
    SetWindowPos(ghMainWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_SHOWWINDOW);

    //    メインスプリットバー
    if (ghMainSplitWnd)
    {
        AppClientAreaCalc(&rect); //    右に併せて移動
        grdSplitPos = PLIST_DOCK;
        AppLayoutSplitBarSync(ghMainSplitWnd, &rect, grdSplitPos);

        ViewSizeMove(hWnd, &rect); //    位置情報リセットした
    }
    else //    フローティングウインドウ
    {
        PageListPositionReset(ghMainWnd);
        PaletteCommonPositionReset(ghMainWnd);
        PaletteBucketPositionReset(ghMainWnd);
    }

    DestroyWindow(ghPgVwWnd);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ファイルから履歴取り込んだり書き込んだり
HRESULT OpenHistoryInitialise(HWND hWnd)
{
    HMENU hSubMenu;
    TCHAR atKeyName[MIN_STRING], atString[MAX_PATH + 10];
    UINT d;
    UINT_PTR dItems;
    OPENHIST stOpenHist;
    OPHIS_ITR itHist;

    if (hWnd)
    {
        gltOpenHist.clear(); //    とりあえず全削除

        for (d = 0; OPENHIST_MAX > d; d++)
        {
            ZeroMemory(&stOpenHist, sizeof(OPENHIST));
            StringCchPrintf(atKeyName, MIN_STRING, TEXT("Hist%X"), d);
            GetPrivateProfileString(TEXT("OpenHistory"), atKeyName, TEXT(""), stOpenHist.atFile, MAX_PATH, gatIniPath);
            if (0 == stOpenHist.atFile[0])
                break; //    記録無くなったらそこで終了だよ

            gltOpenHist.push_back(stOpenHist);
        }

        if (ghHistyMenu)
            DestroyMenu(ghHistyMenu);
        //    メニュー作成
        ghHistyMenu = CreatePopupMenu();
        AppendMenu(ghHistyMenu, MF_SEPARATOR, 0, nullptr); //    セッパレター
        AppendMenu(ghHistyMenu, MF_STRING, IDM_OPEN_HIS_CLEAR, UiTextGetLabel(IDM_OPEN_HIS_CLEAR));
        // 文字列固定はあまりイクナイ

        dItems = gltOpenHist.size();
        if (0 == dItems)
        {
            //    オーポン履歴が無い場合
            InsertMenu(ghHistyMenu, 0, MF_STRING | MF_BYPOSITION | MF_GRAYED, IDM_OPEN_HIS_FIRST, TEXT("(비었음)"));
        }
        else
        {
            //    オーポン履歴を並べる
            for (itHist = gltOpenHist.begin(), d = dItems - 1; gltOpenHist.end() != itHist; itHist++, d--)
            {
                StringCchPrintf(atString, MAX_PATH + 10, TEXT("(&%X) %s"), d, itHist->atFile);
                InsertMenu(ghHistyMenu, 0, MF_STRING | MF_BYPOSITION, (IDM_OPEN_HIS_FIRST + d), atString);
                itHist->dMenuNumber = (IDM_OPEN_HIS_FIRST + d);
            }
        }
        //    メニュー情報の書換
        hSubMenu = GetSubMenu(ghMainMenu, 0);
        ModifyMenu(hSubMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR)ghHistyMenu,
               UiTextGetLabel(IDM_OPEN_HISTORY));
        MenuMnemonicApply(ghMainMenu);
        // 文字列固定はあまりイクナイ

        DrawMenuBar(hWnd); //    要らないかも？
    }
    else //    APPZ終了時
    {
        if (ghHistyMenu)
            DestroyMenu(ghHistyMenu);

        WritePrivateProfileSection(TEXT("OpenHistory"), nullptr, gatIniPath); //    一旦全削除

        //    中身を保存
        for (itHist = gltOpenHist.begin(), d = 0; gltOpenHist.end() != itHist; itHist++, d++)
        {
            StringCchPrintf(atKeyName, MIN_STRING, TEXT("Hist%X"), d);
            WritePrivateProfileString(TEXT("OpenHistory"), atKeyName, itHist->atFile, gatIniPath);
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 開いた履歴を番号指定して読み込む
HRESULT OpenHistoryLoad(HWND hWnd, INT id)
{
    UINT_PTR dNumber, dItems;
    LPARAM lUnique;
    OPHIS_ITR itHist;

    dNumber = id - IDM_OPEN_HIS_FIRST;

    TRACE(TEXT("열어본 파일 기록 -> %d"), dNumber);
    if (OPENHIST_MAX <= dNumber)
    {
        return E_OUTOFMEMORY;
    }

    dItems = gltOpenHist.size();
    dNumber = (dItems - 1) - dNumber;

    itHist = gltOpenHist.begin();
    std::advance(itHist, dNumber); //    個数分進める

    lUnique = DocOpendFileCheck(itHist->atFile);
    if (1 <= lUnique) //    既存のファイルヒット・そっちに移動する
    {
        if (SUCCEEDED(MultiFileTabSelect(dNumber)))
        {
            DocMultiFileSelect(lUnique); //    そのタブのファイルを表示
        }
    }
    else
    {
        DocDoOpenFile(hWnd, itHist->atFile); //    履歴から選択したファイルを開く
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 開いたファイルを記録する
HRESULT OpenHistoryLogging(HWND hWnd, LPTSTR ptFile)
{
    UINT_PTR dItems;
    OPENHIST stOpenHist;
    OPHIS_ITR itHist;

    if (ptFile)
    {
        ZeroMemory(&stOpenHist, sizeof(OPENHIST));

        StringCchCopy(stOpenHist.atFile, MAX_PATH, ptFile);
        // 既存の内容なら最新に入れ替えるので、検索しておく
        for (itHist = gltOpenHist.begin(); gltOpenHist.end() != itHist; itHist++)
        {
            if (!StrCmp(itHist->atFile, stOpenHist.atFile)) //    同じものがあったら削除する
            {
                gltOpenHist.erase(itHist);
                break;
            }
        }

        gltOpenHist.push_back(stOpenHist); //    リスト末尾ほど新しい

        //    もしはみ出すようなら古いのを削除する
        dItems = gltOpenHist.size();
        if (OPENHIST_MAX < dItems)
        {
            gltOpenHist.pop_front();
        }
    }
    else //    文字列指定無い場合は全クリ
    {
        gltOpenHist.clear();
    }

    OpenHistoryInitialise(nullptr); //    古いの破壊して
    OpenHistoryInitialise(hWnd);    //    最新の内容で作り直し

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// ユニコード空白の使用・不使用
UINT UnicodeUseToggle(LPVOID pVoid)
{
    gbUniPad = !(gbUniPad); //    トグル
    InitParamValue(INIT_SAVE, VL_USE_UNICODE, gbUniPad);
    MenuItemCheckOnOff(IDM_UNICODE_TOGGLE, gbUniPad);

    return gbUniPad;
}
//-------------------------------------------------------------------------------------------------

// ドッキングしてる壱行ブラシテンプレを表示/非表示
HRESULT DockingTmplViewToggle(UINT bMode)
{
    HWND hDockWnd;
    RECT rect;
    APP_DOCKED_WINDOW_LAYOUT stDockedLayout;
    INT curSel;

    UNREFERENCED_PARAMETER(bMode);

    hDockWnd = DockingTabGet();
    //    分離状態ならタブは無い
    if (!(hDockWnd))
        return E_ABORT;

    AppClientAreaCalc(&rect);
    curSel = TabCtrl_GetCurSel(hDockWnd);

    if (gbDockTmplView) //    見えてるなら閉じればいい
    {
        gbDockTmplView = FALSE;
        ShowWindow(ghPalInsertWnd, SW_HIDE);
        ShowWindow(ghPalBucketWnd, SW_HIDE);

        stDockedLayout = AppLayoutDockedWindowsCalc(&rect, grdSplitPos, AppLayoutDockTabHeightGet(hDockWnd), gbDockTmplView);
        AppLayoutApplyDockedWindows(stDockedLayout, ghPgVwWnd, hDockWnd, ghPalInsertWnd, ghPalBucketWnd);
    }
    else
    {
        gbDockTmplView = TRUE;

        stDockedLayout = AppLayoutDockedWindowsCalc(&rect, grdSplitPos, AppLayoutDockTabHeightGet(hDockWnd), gbDockTmplView);
        AppLayoutApplyDockedWindows(stDockedLayout, ghPgVwWnd, hDockWnd, ghPalInsertWnd, ghPalBucketWnd);

        //    今の状態に合わせて復帰
        switch (curSel)
        {
        case 0:
            ShowWindow(ghPalInsertWnd, SW_SHOW);
            break;
        case 1:
            ShowWindow(ghPalBucketWnd, SW_SHOW);
            break;
        default:
            break;
        }
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
//    Dirty Deeds Done Dirt Cheap 自分のスタンドに「いとも容易く行われるえげつない行為」なんて名前を付けるのはどうかと思う。
