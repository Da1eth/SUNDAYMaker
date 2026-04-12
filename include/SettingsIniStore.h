#pragma once

#include <windows.h>
#include <tchar.h>

#define SETTINGS_INI_FILE                      TEXT("Setting.ini")
#define SETTINGS_FRAME_JSON_FILE               TEXT("Frame.json")
#define SETTINGS_CONTEXT_MENU_INI_FILE         TEXT("AccelKey.ini")

#define LEGACY_SETTINGS_INI_FILE               TEXT("Utuho.ini")
#define LEGACY_SETTINGS_FRAME_JSON_FILE        TEXT("Satori.ini")
#define LEGACY_SETTINGS_CONTEXT_MENU_INI_FILE  TEXT("Koisi.ini")

DWORD OrrSettingsGetPrivateProfileString(LPCTSTR, LPCTSTR, LPCTSTR, LPTSTR, DWORD, LPCTSTR);
UINT OrrSettingsGetPrivateProfileInt(LPCTSTR, LPCTSTR, INT, LPCTSTR);
BOOL OrrSettingsWritePrivateProfileString(LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR);
BOOL OrrSettingsWritePrivateProfileSection(LPCTSTR, LPCTSTR, LPCTSTR);
DWORD OrrSettingsGetPrivateProfileSection(LPCTSTR, LPTSTR, DWORD, LPCTSTR);

#ifdef GetPrivateProfileString
#undef GetPrivateProfileString
#endif
#ifdef GetPrivateProfileInt
#undef GetPrivateProfileInt
#endif
#ifdef WritePrivateProfileString
#undef WritePrivateProfileString
#endif
#ifdef WritePrivateProfileSection
#undef WritePrivateProfileSection
#endif
#ifdef GetPrivateProfileSection
#undef GetPrivateProfileSection
#endif

#define GetPrivateProfileString OrrSettingsGetPrivateProfileString
#define GetPrivateProfileInt OrrSettingsGetPrivateProfileInt
#define WritePrivateProfileString OrrSettingsWritePrivateProfileString
#define WritePrivateProfileSection OrrSettingsWritePrivateProfileSection
#define GetPrivateProfileSection OrrSettingsGetPrivateProfileSection