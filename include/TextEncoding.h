#pragma once

#include <windows.h>
#include <tchar.h>

LPTSTR TextDecodeAutoAlloc( LPCVOID, INT, PUINT );
LPTSTR LegacyEncodedTextDecodeAlloc( LPSTR );
LPSTR LegacyEncodedTextEncodeAlloc( LPCTSTR );
