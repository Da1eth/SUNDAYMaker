// pch.h – 공통 전처리 헤더
#pragma once

#define STRICT

#pragma warning( disable : 4100 )
#pragma warning( disable : 4201 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4312 )

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif

#ifndef WINVER
#define WINVER 0x0A00
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT
#endif

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <windowsX.h>
#pragma comment(lib, "shell32.lib")

#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")

#include <Commctrl.h>
#pragma comment(lib, "ComCtl32.lib")

#include <imm.h>
#pragma comment(lib, "imm32.lib")

#include "Sqlite3/sqlite3.h"
#pragma comment(lib, "sqlite3.lib")

#include <iterator>
#include <malloc.h>

#include <cassert>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <malloc.h>
#include <cstring>
#include <cwctype>
#include <tchar.h>
#include <ctime>

#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

#ifdef _WIN64
inline HRESULT StringCchLengthW(STRSAFE_PCNZWCH psz, size_t cchMax, UINT *pcch)
{
    size_t n = 0;
    HRESULT hr = ::StringCchLengthW(psz, cchMax, &n);
    *pcch = static_cast<UINT>(n);
    return hr;
}
inline HRESULT StringCchLengthA(STRSAFE_PCNZCH psz, size_t cchMax, UINT *pcch)
{
    size_t n = 0;
    HRESULT hr = ::StringCchLengthA(psz, cchMax, &n);
    *pcch = static_cast<UINT>(n);
    return hr;
}
#endif

#pragma warning( disable : 4995 )
#include <shlwapi.h>
#pragma warning( default : 4995 )
#pragma comment(lib, "shlwapi.lib")

#pragma warning( disable : 4995 )
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <string_view>
#include <array>
#include <optional>
#include <span>
#pragma warning( default : 4995 )

using namespace std;

#include "SettingsIniStore.h"

static constexpr GUID gcstGUID = { 0x66D3E881, 0x972B, 0x458B, { 0x93, 0x5E, 0x9E, 0x78, 0xB9, 0x26, 0xB4, 0x15 } };

#define ACCELERATOR_EDIT
#define USE_HOVERTIP
#define EDGE_BLANK_STYLE
#define MULTIACT_RELAY
#define DOT_SPLIT_MODE
#define SPLIT_BAR_POS_FIX
#define SPMOZI_ENCODE
#define FIND_STRINGS
#define DO_TRY_CATCH

#define EXTERNED

#define TRACE(x,...)

#define SQL_DEBUG(db)

#ifdef DO_TRY_CATCH
#define ETC_MSG(str,ret)    ExceptionMessage( str, __FUNCTION__, __LINE__, ret )
LRESULT ExceptionMessage( LPCSTR, LPCSTR, UINT, LPARAM );
#endif

template<typename T>
inline void SafeFree( T*& pp ) { if( pp ){ free( pp ); pp = nullptr; } }
#define FREE(pp)    SafeFree(pp)

#define StatusBar_SetText(hwndSB,ipart,ptext)    (BOOLEAN)SNDMSG((hwndSB),SB_SETTEXT,ipart,(LPARAM)(LPCTSTR)(ptext))

inline constexpr int MIN_STRING = 20;
inline constexpr int SUB_STRING = 64;
inline constexpr int MAX_STRING = 130;
inline constexpr int BIG_STRING = 520;

inline constexpr int W_WIDTH  = 480;
inline constexpr int W_HEIGHT = 400;

inline constexpr int PLIST_DOCK = 190;
inline constexpr int TMPL_DOCK  = 150;

#define WMP_BRUSH_TOGGLE    (WM_APP+2)
#define WMP_PREVIEW_CLOSE   (WM_APP+3)
#define WMP_TRAYICON        (WM_APP+4)

inline constexpr COLORREF BASIC_COLOUR = RGB(0xF0,0xF0,0xF0);

#define USER_ITEM_FILE    TEXT("UserItem.ast")
inline constexpr int USER_ITEM_MAX = 16;

#define AA_PALETTE_FILE  TEXT("AAPalette.json")
#define FLIP_FILE        TEXT("Flip.json")

#define LEGACY_AA_BRUSH_FILE    TEXT("aabrush.txt")
#define LEGACY_AA_LIST_FILE     TEXT("aalist.txt")
#define LEGACY_AA_MIRROR_FILE   TEXT("hantenX.txt")
#define LEGACY_AA_UPSET_FILE    TEXT("hantenY.txt")

#define NAMELESS_DUMMY   TEXT("NoName0.txt")
#define NAME_DUMMY_NAME  TEXT("NoName")
#define NAME_DUMMY_EXT   TEXT("txt")

#define TEMPLATE_DIR     TEXT("Templates")
#define BACKUP_DIR       TEXT("BackUp")
#define PROFILE_DIR      TEXT("Profile")

#define DROP_OBJ_NAME    TEXT("[*DROP_OBJECT*]")

inline constexpr int FRAME_MAX = 20;

inline constexpr UINT INIT_LOAD = 1;
inline constexpr UINT INIT_SAVE = 0;

inline constexpr UINT WDP_MVIEW   = 1;
inline constexpr UINT WDP_PLIST   = 2;
inline constexpr UINT WDP_LNTMPL  = 3;
inline constexpr UINT WDP_BRTMPL  = 4;
inline constexpr UINT WDP_PREVIEW = 6;

inline constexpr int FONTSZ_NORMAL = 16;
inline constexpr int FONTSZ_REDUCE = 12;

inline constexpr UINT VL_CLASHCOVER   = 0;
inline constexpr UINT VL_GROUP_UNDO   = 1;
inline constexpr UINT VL_USE_UNICODE  = 2;
inline constexpr UINT VL_LAYER_TRANS  = 3;
inline constexpr UINT VL_LINETMP_CLM  = 10;
inline constexpr UINT VL_BRUSHTMP_CLM = 11;
inline constexpr UINT VL_UNIRADIX_HEX = 12;
inline constexpr UINT VL_BACKUP_INTVL = 13;
inline constexpr UINT VL_BACKUP_MSGON = 14;
inline constexpr UINT VL_GRID_X_POS   = 15;
inline constexpr UINT VL_GRID_Y_POS   = 16;
inline constexpr UINT VL_R_RULER_POS  = 18;
inline constexpr UINT VL_CRLF_CODE    = 19;
inline constexpr UINT VL_SPACE_VIEW   = 20;
inline constexpr UINT VL_GRID_VIEW    = 21;
inline constexpr UINT VL_R_RULER_VIEW = 22;
inline constexpr UINT VL_PAGETIP_VIEW = 23;
inline constexpr UINT VL_PDELETE_NM   = 26;
inline constexpr UINT VL_USE_BALLOON  = 28;
inline constexpr UINT VL_CLIPFILECNT  = 29;
inline constexpr UINT VL_PLS_LN_DOCK  = 30;
inline constexpr UINT VS_PROFILE_NAME = 32;
inline constexpr UINT VL_SWAP_COPY    = 34;
inline constexpr UINT VL_MAIN_SPLIT   = 35;
inline constexpr UINT VL_MAXIMISED    = 36;
inline constexpr UINT VL_FIRST_READED = 38;
inline constexpr UINT VL_LAST_OPEN    = 39;
inline constexpr UINT VL_WORKLOG      = 43;
inline constexpr UINT VL_COLLECT_AON  = 46;
inline constexpr UINT VL_COLHOT_MODY  = 47;
inline constexpr UINT VL_COLHOT_VKEY  = 48;
inline constexpr UINT VL_PGL_RETFCS   = 50;
inline constexpr UINT VL_U_RULER_POS  = 51;
inline constexpr UINT VL_U_RULER_VIEW = 52;
inline constexpr UINT VS_RGUIDE_MOZI  = 56;
inline constexpr UINT VL_MULTI_ACT_E  = 59;
inline constexpr UINT VL_SAVE_MSGON   = 60;
inline constexpr UINT VL_SPMOZI_ENC   = 63;

inline constexpr UINT CLRV_BASICPEN  = 101;
inline constexpr UINT CLRV_BASICBK   = 102;
inline constexpr UINT CLRV_GRIDLINE  = 103;
inline constexpr UINT CLRV_CRLFMARK  = 104;
inline constexpr UINT CLRV_NOSJISBK = 105;

inline constexpr INT SB_MODIFY   = 0;
inline constexpr INT SB_OP_STYLE = 1;
inline constexpr INT SB_MOUSEPOS = 2;
inline constexpr INT SB_CURSOR   = 3;
inline constexpr INT SB_LAYER    = 4;
inline constexpr INT SB_BYTECNT  = 5;
inline constexpr INT SB_SELBYTE  = 6;

inline constexpr INT WND_MAIN  = 1;
inline constexpr INT WND_PAGE  = 2;
inline constexpr INT WND_LINE  = 3;
inline constexpr INT WND_BRUSH = 4;
inline constexpr INT WND_TAIL  = 4;

inline constexpr INT M_DESTROY   = 0;
inline constexpr INT M_CREATE    = 1;
inline constexpr INT M_EXISTENCE = 2;

inline constexpr INT LASTOPEN_DO  = 0;
inline constexpr INT LASTOPEN_NON = 1;
inline constexpr INT LASTOPEN_ASK = 2;

inline constexpr UINT D_SJIS   = 0x00;
inline constexpr UINT D_UNI    = 0x01;
inline constexpr UINT D_SQUARE = 0x02;
inline constexpr UINT D_ENTITY = 0x04;
inline constexpr UINT D_INVISI = 0x10;
inline constexpr UINT D_RENAME = 0x80;

inline constexpr INT ISAVE_PNG = 0x1;
inline constexpr INT ISAVE_BMP = 0x2;

#define CLIP_FORMAT    TEXT("SUNDAY_EDITOR_STYLE")
#define CLIP_SQUARE    TEXT("MSDEVColumnSelect")

inline constexpr UINT VK_0 = 0x30;
inline constexpr UINT VK_1 = 0x31;
inline constexpr UINT VK_2 = 0x32;
inline constexpr UINT VK_3 = 0x33;
inline constexpr UINT VK_4 = 0x34;
inline constexpr UINT VK_5 = 0x35;
inline constexpr UINT VK_6 = 0x36;
inline constexpr UINT VK_7 = 0x37;
inline constexpr UINT VK_8 = 0x38;
inline constexpr UINT VK_9 = 0x39;
inline constexpr UINT VK_A = 0x41;
inline constexpr UINT VK_B = 0x42;
inline constexpr UINT VK_C = 0x43;
inline constexpr UINT VK_D = 0x44;
inline constexpr UINT VK_E = 0x45;
inline constexpr UINT VK_F = 0x46;
inline constexpr UINT VK_G = 0x47;
inline constexpr UINT VK_H = 0x48;
inline constexpr UINT VK_I = 0x49;
inline constexpr UINT VK_J = 0x4A;
inline constexpr UINT VK_K = 0x4B;
inline constexpr UINT VK_L = 0x4C;
inline constexpr UINT VK_M = 0x4D;
inline constexpr UINT VK_N = 0x4E;
inline constexpr UINT VK_O = 0x4F;
inline constexpr UINT VK_P = 0x50;
inline constexpr UINT VK_Q = 0x51;
inline constexpr UINT VK_R = 0x52;
inline constexpr UINT VK_S = 0x53;
inline constexpr UINT VK_T = 0x54;
inline constexpr UINT VK_U = 0x55;
inline constexpr UINT VK_V = 0x56;
inline constexpr UINT VK_W = 0x57;
inline constexpr UINT VK_X = 0x58;
inline constexpr UINT VK_Y = 0x59;
inline constexpr UINT VK_Z = 0x5A;
