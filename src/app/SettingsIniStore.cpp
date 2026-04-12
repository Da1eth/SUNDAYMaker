#include <unordered_map>

#include "TextEncoding.h"

namespace
{
    struct CONFIG_ITEM
    {
        wstring wsKey;
        wstring wsKeyFold;
        wstring wsValue;
    };

    struct CONFIG_SECTION
    {
        wstring wsName;
        wstring wsNameFold;
        vector<CONFIG_ITEM> vcItems;
        unordered_map<wstring, size_t> mpItemIndex;
    };

    struct CONFIG_CACHE
    {
        wstring wsRequestPath;
        wstring wsLoadedPath;
        FILETIME stWriteTime;
        vector<CONFIG_SECTION> vcSections;
        unordered_map<wstring, size_t> mpSectionIndex;
        BOOL bValid;
    };

    vector<CONFIG_CACHE> gvcConfigCache;

    LPTSTR ConfigDecodeUtf8Alloc(LPCVOID pBuffer, INT cbSize)
    {
        INT cchSize;
        LPTSTR ptBuffer;

        if (!(pBuffer) || 0 >= cbSize)
            return nullptr;

        cchSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)pBuffer,
                                      cbSize, nullptr, 0);
        if (0 >= cchSize)
            return nullptr;

        ptBuffer = (LPTSTR)malloc((cchSize + 1) * sizeof(TCHAR));
        if (!(ptBuffer))
            return nullptr;
        ZeroMemory(ptBuffer, (cchSize + 1) * sizeof(TCHAR));

        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)pBuffer, cbSize,
                            ptBuffer, cchSize);
        return ptBuffer;
    }

    wstring ConfigLowerCopy(wstring_view wsText)
    {
        wstring wsOut;

        wsOut.reserve(wsText.size());
        for (wchar_t ch : wsText)
        {
            wsOut.push_back((wchar_t)towlower(ch));
        }

        return wsOut;
    }

    wstring_view ConfigTrimView(wstring_view wsText)
    {
        size_t start = 0;
        size_t end = wsText.size();

        while (start < end && iswspace(wsText[start]))
            start++;
        while (start < end && iswspace(wsText[end - 1]))
            end--;

        return wsText.substr(start, end - start);
    }

    void ConfigSectionAssignName(CONFIG_SECTION *pstSection, wstring_view wsName)
    {
        pstSection->wsName.assign(wsName);
        pstSection->wsNameFold = ConfigLowerCopy(pstSection->wsName);
    }

    void ConfigItemAssignKey(CONFIG_ITEM *pstItem, wstring_view wsKey)
    {
        pstItem->wsKey.assign(wsKey);
        pstItem->wsKeyFold = ConfigLowerCopy(pstItem->wsKey);
    }

    void ConfigSectionRebuildIndex(CONFIG_SECTION *pstSection)
    {
        pstSection->mpItemIndex.clear();
        pstSection->mpItemIndex.reserve(pstSection->vcItems.size());

        for (size_t i = 0; i < pstSection->vcItems.size(); i++)
        {
            pstSection->mpItemIndex[pstSection->vcItems[i].wsKeyFold] = i;
        }
    }

    void ConfigSectionsRebuildIndex(vector<CONFIG_SECTION> *pSections)
    {
        for (auto &stSection : *pSections)
        {
            ConfigSectionRebuildIndex(&stSection);
        }
    }

    void ConfigCacheRebuildIndex(CONFIG_CACHE *pstCache)
    {
        pstCache->mpSectionIndex.clear();
        pstCache->mpSectionIndex.reserve(pstCache->vcSections.size());

        for (size_t i = 0; i < pstCache->vcSections.size(); i++)
        {
            ConfigSectionRebuildIndex(&(pstCache->vcSections[i]));
            pstCache->mpSectionIndex[pstCache->vcSections[i].wsNameFold] = i;
        }
    }

    CONFIG_SECTION *ConfigSectionFind(vector<CONFIG_SECTION> *pSections,
                                      wstring_view wsName)
    {
        wstring wsNeedle = ConfigLowerCopy(wsName);

        for (auto &stSection : *pSections)
        {
            if (stSection.wsNameFold == wsNeedle)
            {
                return &stSection;
            }
        }

        return nullptr;
    }

    CONFIG_ITEM *ConfigItemFind(CONFIG_SECTION *pstSection, wstring_view wsKey)
    {
        wstring wsNeedle = ConfigLowerCopy(wsKey);
        auto itFound = pstSection->mpItemIndex.find(wsNeedle);

        if (pstSection->mpItemIndex.end() != itFound)
        {
            return &(pstSection->vcItems[itFound->second]);
        }

        for (auto &stItem : pstSection->vcItems)
        {
            if (stItem.wsKeyFold == wsNeedle)
            {
                return &stItem;
            }
        }

        return nullptr;
    }

    const CONFIG_ITEM *ConfigItemFindConst(const CONFIG_SECTION *pstSection,
                                           wstring_view wsKey)
    {
        wstring wsNeedle = ConfigLowerCopy(wsKey);
        auto itFound = pstSection->mpItemIndex.find(wsNeedle);

        if (pstSection->mpItemIndex.end() != itFound)
        {
            return &(pstSection->vcItems[itFound->second]);
        }

        return nullptr;
    }

    const CONFIG_SECTION *ConfigCacheSectionFindConst(const CONFIG_CACHE *pstCache,
                                                      wstring_view wsName)
    {
        wstring wsNeedle = ConfigLowerCopy(wsName);
        auto itFound = pstCache->mpSectionIndex.find(wsNeedle);

        if (pstCache->mpSectionIndex.end() == itFound)
        {
            return nullptr;
        }

        return &(pstCache->vcSections[itFound->second]);
    }

    BOOL Utf8WriteFileFromWide(LPCTSTR ptFilePath, const wstring &wsText)
    {
        HANDLE hFile;
        DWORD wrote;
        INT cbSize;
        LPSTR pcUtf8;

        cbSize = WideCharToMultiByte(CP_UTF8, 0, wsText.c_str(), -1, nullptr, 0,
                                     nullptr, nullptr);
        if (0 >= cbSize)
            return FALSE;

        pcUtf8 = (LPSTR)malloc(cbSize);
        if (!(pcUtf8))
            return FALSE;
        ZeroMemory(pcUtf8, cbSize);
        WideCharToMultiByte(CP_UTF8, 0, wsText.c_str(), -1, pcUtf8, cbSize, nullptr,
                            nullptr);

        hFile = CreateFile(ptFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            FREE(pcUtf8);
            return FALSE;
        }

        WriteFile(hFile, pcUtf8, cbSize - 1, &wrote, nullptr);
        CloseHandle(hFile);
        FREE(pcUtf8);

        return TRUE;
    }

    wstring ConfigNormalizePath(LPCTSTR ptFilePath)
    {
        if (!(ptFilePath))
        {
            return wstring();
        }

        return ConfigLowerCopy(ptFilePath);
    }

    BOOL ConfigFileStat(LPCTSTR ptFilePath, wstring *pResolvedPath,
                        FILETIME *pstWriteTime)
    {
        WIN32_FILE_ATTRIBUTE_DATA stData;

        if (!(ptFilePath))
            return FALSE;

        if (GetFileAttributesEx(ptFilePath, GetFileExInfoStandard, &stData))
        {
            if (pResolvedPath)
                *pResolvedPath = ptFilePath;
            if (pstWriteTime)
                *pstWriteTime = stData.ftLastWriteTime;
            return TRUE;
        }

        return FALSE;
    }

    CONFIG_CACHE *ConfigCacheFind(LPCTSTR ptFilePath)
    {
        wstring wsPath = ConfigNormalizePath(ptFilePath);

        for (auto &stCache : gvcConfigCache)
        {
            if (stCache.wsRequestPath == wsPath)
            {
                return &stCache;
            }
        }

        return nullptr;
    }

    void ConfigCacheStore(LPCTSTR ptFilePath, const wstring &wsLoadedPath,
                          const FILETIME *pstWriteTime,
                          const vector<CONFIG_SECTION> &vcSections)
    {
        CONFIG_CACHE *pstCache = ConfigCacheFind(ptFilePath);

        if (!(pstCache))
        {
            CONFIG_CACHE stCache = {};

            stCache.wsRequestPath = ConfigNormalizePath(ptFilePath);
            gvcConfigCache.push_back(stCache);
            pstCache = &(gvcConfigCache.back());
        }

        pstCache->wsLoadedPath = ConfigLowerCopy(wsLoadedPath);
        pstCache->vcSections = vcSections;
        ConfigCacheRebuildIndex(pstCache);
        pstCache->bValid = TRUE;
        if (pstWriteTime)
            pstCache->stWriteTime = *pstWriteTime;
        else
            ZeroMemory(&(pstCache->stWriteTime), sizeof(FILETIME));
    }

    BOOL ConfigParseText(LPCTSTR ptText, vector<CONFIG_SECTION> *pSections)
    {
        wstring wsLine;
        wstring_view wsView;
        const wchar_t *ptCaret;
        CONFIG_SECTION *pstCurrent = nullptr;

        pSections->clear();
        if (!(ptText))
            return FALSE;

        ptCaret = ptText;
        while (*ptCaret)
        {
            const wchar_t *ptLine = ptCaret;
            while (*ptCaret && *ptCaret != TEXT('\r') && *ptCaret != TEXT('\n'))
                ptCaret++;

            wsLine.assign(ptLine, ptCaret - ptLine);
            wsView = ConfigTrimView(wsLine);
            if (!(wsView.empty()) && wsView[0] != TEXT(';') && wsView[0] != TEXT('#'))
            {
                if (2 <= wsView.size() && TEXT('[') == wsView.front() &&
                    TEXT(']') == wsView.back())
                {
                    CONFIG_SECTION stSection;

                    ConfigSectionAssignName(
                        &stSection, ConfigTrimView(wsView.substr(1, wsView.size() - 2)));
                    pSections->push_back(stSection);
                    pstCurrent = &(pSections->back());
                }
                else
                {
                    size_t pos = wsView.find(TEXT('='));
                    if (wstring_view::npos != pos)
                    {
                        CONFIG_ITEM stItem;

                        if (!(pstCurrent))
                        {
                            CONFIG_SECTION stSection;
                            ConfigSectionAssignName(&stSection, TEXT("General"));
                            pSections->push_back(stSection);
                            pstCurrent = &(pSections->back());
                        }

                        ConfigItemAssignKey(&stItem, ConfigTrimView(wsView.substr(0, pos)));
                        stItem.wsValue.assign(ConfigTrimView(wsView.substr(pos + 1)));
                        pstCurrent->vcItems.push_back(stItem);
                    }
                }
            }

            if (TEXT('\r') == *ptCaret)
                ptCaret++;
            if (TEXT('\n') == *ptCaret)
                ptCaret++;
        }

        ConfigSectionsRebuildIndex(pSections);
        return TRUE;
    }

    const CONFIG_CACHE *ConfigLoadCached(LPCTSTR ptFilePath)
    {
        HANDLE hFile;
        DWORD readed;
        INT iByteSize;
        LPVOID pBuffer;
        LPTSTR ptString;
        wstring wsLoadPath;
        FILETIME stWriteTime;
        CONFIG_CACHE *pstCache;
        vector<CONFIG_SECTION> vcSections;

        if (!(ConfigFileStat(ptFilePath, &wsLoadPath, &stWriteTime)))
        {
            return nullptr;
        }

        pstCache = ConfigCacheFind(ptFilePath);
        if (pstCache && pstCache->bValid)
        {
            if (pstCache->wsLoadedPath == ConfigLowerCopy(wsLoadPath) &&
                0 == CompareFileTime(&(pstCache->stWriteTime), &stWriteTime))
            {
                return pstCache;
            }
        }

        hFile = CreateFile(wsLoadPath.c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
        if (INVALID_HANDLE_VALUE == hFile)
            return nullptr;

        iByteSize = GetFileSize(hFile, nullptr);
        pBuffer = malloc(iByteSize + 4);
        if (!(pBuffer))
        {
            CloseHandle(hFile);
            return nullptr;
        }
        ZeroMemory(pBuffer, iByteSize + 4);

        ReadFile(hFile, pBuffer, iByteSize, &readed, nullptr);
        CloseHandle(hFile);

        ptString = ConfigDecodeUtf8Alloc(pBuffer, iByteSize);
        if (!(ptString))
        {
            ptString = TextDecodeAutoAlloc(pBuffer, iByteSize, nullptr);
        }
        FREE(pBuffer);
        if (!(ptString))
            return nullptr;

        ConfigParseText(ptString, &vcSections);
        FREE(ptString);
        ConfigCacheStore(ptFilePath, wsLoadPath, &stWriteTime, vcSections);

        return ConfigCacheFind(ptFilePath);
    }

    BOOL ConfigLoad(LPCTSTR ptFilePath, vector<CONFIG_SECTION> *pSections)
    {
        const CONFIG_CACHE *pstCache;

        pSections->clear();
        pstCache = ConfigLoadCached(ptFilePath);
        if (!(pstCache))
            return FALSE;

        *pSections = pstCache->vcSections;
        return TRUE;
    }

    BOOL ConfigSave(LPCTSTR ptFilePath, const vector<CONFIG_SECTION> &vcSections)
    {
        wstring wsText;
        FILETIME stWriteTime;

        for (const auto &stSection : vcSections)
        {
            wsText += TEXT("[");
            wsText += stSection.wsName;
            wsText += TEXT("]\r\n");

            for (const auto &stItem : stSection.vcItems)
            {
                wsText += stItem.wsKey;
                wsText += TEXT("=");
                wsText += stItem.wsValue;
                wsText += TEXT("\r\n");
            }

            wsText += TEXT("\r\n");
        }

        if (!(Utf8WriteFileFromWide(ptFilePath, wsText)))
            return FALSE;
        if (ConfigFileStat(ptFilePath, nullptr, &stWriteTime))
        {
            ConfigCacheStore(ptFilePath, ptFilePath, &stWriteTime, vcSections);
        }
        else
        {
            ConfigCacheStore(ptFilePath, ptFilePath, nullptr, vcSections);
        }

        return TRUE;
    }
} // namespace

DWORD OrrSettingsGetPrivateProfileString(LPCTSTR ptAppName, LPCTSTR ptKeyName,
                                         LPCTSTR ptDefault, LPTSTR ptReturnedString,
                                         DWORD cchSize, LPCTSTR ptFilePath)
{
    const CONFIG_CACHE *pstCache;
    const CONFIG_SECTION *pstSection;
    const CONFIG_ITEM *pstItem;
    wstring wsDefault = ptDefault ? ptDefault : TEXT("");
    LPCTSTR ptSource = wsDefault.c_str();

    if (!(ptReturnedString) || 0 == cchSize)
        return 0;
    ptReturnedString[0] = 0;

    if (!(ptAppName) || !(ptKeyName))
    {
        StringCchCopy(ptReturnedString, cchSize, ptSource);
        return lstrlen(ptReturnedString);
    }

    pstCache = ConfigLoadCached(ptFilePath);
    pstSection =
        pstCache ? ConfigCacheSectionFindConst(pstCache, ptAppName) : nullptr;
    if (pstSection)
    {
        pstItem = ConfigItemFindConst(pstSection, ptKeyName);
        if (pstItem)
            ptSource = pstItem->wsValue.c_str();
    }

    StringCchCopy(ptReturnedString, cchSize, ptSource);
    return lstrlen(ptReturnedString);
}

UINT OrrSettingsGetPrivateProfileInt(LPCTSTR ptAppName, LPCTSTR ptKeyName, INT nDefault,
                                     LPCTSTR ptFilePath)
{
    TCHAR atBuff[64];

    StringCchPrintf(atBuff, 64, TEXT("%d"), nDefault);
    OrrSettingsGetPrivateProfileString(ptAppName, ptKeyName, atBuff, atBuff, 64,
                                       ptFilePath);
    return (UINT)StrToInt(atBuff);
}

BOOL OrrSettingsWritePrivateProfileString(LPCTSTR ptAppName, LPCTSTR ptKeyName,
                                          LPCTSTR ptString, LPCTSTR ptFilePath)
{
    vector<CONFIG_SECTION> vcSections;
    CONFIG_SECTION *pstSection;
    CONFIG_ITEM *pstItem;

    ConfigLoad(ptFilePath, &vcSections);
    if (!(ptAppName))
        return FALSE;

    pstSection = ConfigSectionFind(&vcSections, ptAppName);
    if (!(pstSection))
    {
        CONFIG_SECTION stSection;
        ConfigSectionAssignName(&stSection, ptAppName);
        vcSections.push_back(stSection);
        pstSection = &(vcSections.back());
    }

    if (!(ptKeyName))
    {
        const wstring wsNameFold = ConfigLowerCopy(ptAppName);
        vcSections.erase(remove_if(vcSections.begin(), vcSections.end(),
                                   [&wsNameFold](const CONFIG_SECTION &stSection)
                                   {
                                       return stSection.wsNameFold == wsNameFold;
                                   }),
                         vcSections.end());
        return ConfigSave(ptFilePath, vcSections);
    }

    pstItem = ConfigItemFind(pstSection, ptKeyName);
    if (!(ptString))
    {
        const wstring wsKeyFold = ConfigLowerCopy(ptKeyName);
        pstSection->vcItems.erase(
            remove_if(pstSection->vcItems.begin(), pstSection->vcItems.end(),
                      [&wsKeyFold](const CONFIG_ITEM &stItem)
                      {
                          return stItem.wsKeyFold == wsKeyFold;
                      }),
            pstSection->vcItems.end());
        ConfigSectionRebuildIndex(pstSection);
        return ConfigSave(ptFilePath, vcSections);
    }

    if (pstItem)
    {
        pstItem->wsValue = ptString;
    }
    else
    {
        CONFIG_ITEM stItem;
        ConfigItemAssignKey(&stItem, ptKeyName);
        stItem.wsValue = ptString;
        pstSection->vcItems.push_back(stItem);
        ConfigSectionRebuildIndex(pstSection);
    }

    return ConfigSave(ptFilePath, vcSections);
}

BOOL OrrSettingsWritePrivateProfileSection(LPCTSTR ptAppName, LPCTSTR ptString,
                                           LPCTSTR ptFilePath)
{
    vector<CONFIG_SECTION> vcSections;

    ConfigLoad(ptFilePath, &vcSections);
    if (!(ptAppName))
        return FALSE;

    {
        const wstring wsNameFold = ConfigLowerCopy(ptAppName);
        vcSections.erase(remove_if(vcSections.begin(), vcSections.end(),
                                   [&wsNameFold](const CONFIG_SECTION &stSection)
                                   {
                                       return stSection.wsNameFold == wsNameFold;
                                   }),
                         vcSections.end());
    }

    if (!(ptString))
    {
        return ConfigSave(ptFilePath, vcSections);
    }

    CONFIG_SECTION stSection;
    ConfigSectionAssignName(&stSection, ptAppName);

    while (*ptString)
    {
        LPCTSTR ptEq;
        CONFIG_ITEM stItem;
        wstring wsLine = ptString;

        ptEq = StrChr(ptString, TEXT('='));
        if (ptEq)
        {
            ConfigItemAssignKey(&stItem, wstring_view(ptString, ptEq - ptString));
            stItem.wsValue = ptEq + 1;
            stSection.vcItems.push_back(stItem);
        }

        ptString += wsLine.size() + 1;
    }

    ConfigSectionRebuildIndex(&stSection);
    vcSections.push_back(stSection);
    return ConfigSave(ptFilePath, vcSections);
}

DWORD OrrSettingsGetPrivateProfileSection(LPCTSTR ptAppName, LPTSTR ptReturnedString,
                                          DWORD cchSize, LPCTSTR ptFilePath)
{
    const CONFIG_CACHE *pstCache;
    const CONFIG_SECTION *pstSection;
    DWORD dWritten = 0;

    if (!(ptReturnedString) || 0 == cchSize)
        return 0;
    ZeroMemory(ptReturnedString, cchSize * sizeof(TCHAR));

    if (!(ptAppName))
        return 0;

    pstCache = ConfigLoadCached(ptFilePath);
    pstSection =
        pstCache ? ConfigCacheSectionFindConst(pstCache, ptAppName) : nullptr;
    if (!(pstSection))
        return 0;

    for (const auto &stItem : pstSection->vcItems)
    {
        wstring wsLine = stItem.wsKey + wstring(TEXT("=")) + stItem.wsValue;
        if ((dWritten + wsLine.size() + 2) > cchSize)
            break;

        CopyMemory(ptReturnedString + dWritten, wsLine.c_str(),
                   wsLine.size() * sizeof(TCHAR));
        dWritten += (DWORD)wsLine.size() + 1;
    }

    ptReturnedString[dWritten] = 0;
    return dWritten;
}