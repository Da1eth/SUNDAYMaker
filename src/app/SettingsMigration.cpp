// 레거시 설정 마이그레이션
#include "Sunday.h"

namespace
{
    template<typename TRecord>
    using LEGACY_VECTOR_PARSER = BOOLEAN (*)(const wstring &, vector<TRecord> *);

    VOID MigrateSettingIniKeyNames(LPCTSTR ptFilePath)
    {
        TCHAR atValue[MIN_STRING];
        TCHAR atExisting[MIN_STRING];

        if (!(ptFilePath) || !(PathFileExists(ptFilePath)))
            return;

        GetPrivateProfileString(TEXT("Colour"), TEXT("UseEntity"), TEXT(""), atValue, MIN_STRING, ptFilePath);
        if (0 == atValue[0])
            return;

        GetPrivateProfileString(TEXT("Colour"), TEXT("NoSjisBack"), TEXT(""), atExisting, MIN_STRING, ptFilePath);
        if (0 == atExisting[0])
        {
            WritePrivateProfileString(TEXT("Colour"), TEXT("NoSjisBack"), atValue, ptFilePath);
        }

        WritePrivateProfileString(TEXT("Colour"), TEXT("UseEntity"), nullptr, ptFilePath);
    }

    VOID MigrateIniValueIfMissing(LPCTSTR ptSourcePath, LPCTSTR ptSourceSection, LPCTSTR ptSourceKey, LPCTSTR ptTargetPath, LPCTSTR ptTargetSection, LPCTSTR ptTargetKey)
    {
        TCHAR atValue[MIN_STRING];
        TCHAR atExisting[MIN_STRING];

        if (!(ptSourcePath) || !(ptSourceSection) || !(ptSourceKey) || !(ptTargetPath) || !(ptTargetSection) || !(ptTargetKey))
            return;
        if (!(PathFileExists(ptSourcePath)))
            return;

        GetPrivateProfileString(ptTargetSection, ptTargetKey, TEXT(""), atExisting, MIN_STRING, ptTargetPath);
        if (0 != atExisting[0])
            return;

        GetPrivateProfileString(ptSourceSection, ptSourceKey, TEXT(""), atValue, MIN_STRING, ptSourcePath);
        if (0 == atValue[0])
            return;

        WritePrivateProfileString(ptTargetSection, ptTargetKey, atValue, ptTargetPath);
    }

    VOID MigratePaletteWindowSettingsFromLegacyIni(LPCTSTR ptLegacyPath, LPCTSTR ptTargetPath)
    {
        struct LEGACY_WINDOW_SECTION
        {
            LPCTSTR ptLegacySection;
            LPCTSTR ptTargetSection;
        };

        static const LEGACY_WINDOW_SECTION gastSections[] = {
            {TEXT("LineTmple"), TEXT("PalInsert")},
            {TEXT("BrushTmple"), TEXT("PalBucket")}};
        static const LPCTSTR gaptKeys[] = {
            TEXT("LEFT"),
            TEXT("TOP"),
            TEXT("RIGHT"),
            TEXT("BOTTOM"),
            TEXT("TopMost")};
        UINT i, j;

        if (!(ptLegacyPath) || !(ptTargetPath) || !(PathFileExists(ptLegacyPath)))
            return;

        for (i = 0; ARRAYSIZE(gastSections) > i; i++)
        {
            for (j = 0; ARRAYSIZE(gaptKeys) > j; j++)
            {
                MigrateIniValueIfMissing(ptLegacyPath, gastSections[i].ptLegacySection, gaptKeys[j], ptTargetPath, gastSections[i].ptTargetSection, gaptKeys[j]);
            }
        }
    }

    VOID CleanupSettingIni(LPCTSTR ptFilePath)
    {
        static const LPCTSTR gaptGeneralKeys[] =
            {
                TEXT("MaaSplit"),
                TEXT("HintPopup"),
                TEXT("RightSlide"),
                TEXT("PageByteMax"),
                TEXT("GroupUndo"),
                TEXT("FontName"),
                TEXT("ThumbHoriz"),
                TEXT("ThumbVerti"),
                TEXT("CopyModeSwap"),
                TEXT("ProfilePath"),
                TEXT("RightGuideMozi"),
                TEXT("ExtM2HPath"),
                TEXT("WorkLogDebug")};
        UINT i;

        if (!(ptFilePath) || !(PathFileExists(ptFilePath)))
            return;

        for (i = 0; ARRAYSIZE(gaptGeneralKeys) > i; i++)
        {
            WritePrivateProfileString(TEXT("General"), gaptGeneralKeys[i], nullptr, ptFilePath);
        }

        WritePrivateProfileString(TEXT("Colour"), TEXT("CantSjis"), nullptr, ptFilePath);
        WritePrivateProfileString(TEXT("History"), TEXT("LastOpen"), nullptr, ptFilePath);

        WritePrivateProfileSection(TEXT("ProfHistory"), nullptr, ptFilePath);

        WritePrivateProfileSection(TEXT("History"), nullptr, ptFilePath);
        WritePrivateProfileSection(TEXT("MaaTmple"), nullptr, ptFilePath);
    }

    BOOLEAN ReadDecodedTextFile(LPCTSTR ptFilePath, wstring *pText)
    {
        HANDLE hFile;
        DWORD dRead = 0;
        DWORD dFileSize;
        LPVOID pBuffer;
        LPTSTR ptDecoded;

        if (!(ptFilePath) || !(pText))
            return FALSE;

        pText->clear();

        hFile = CreateFile(ptFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (INVALID_HANDLE_VALUE == hFile)
            return FALSE;

        dFileSize = GetFileSize(hFile, nullptr);
        pBuffer = malloc(dFileSize + 4);
        if (!(pBuffer))
        {
            CloseHandle(hFile);
            return FALSE;
        }
        ZeroMemory(pBuffer, dFileSize + 4);

        ReadFile(hFile, pBuffer, dFileSize, &dRead, nullptr);
        CloseHandle(hFile);

        ptDecoded = TextDecodeAutoAlloc(pBuffer, (INT)dFileSize, nullptr);
        FREE(pBuffer);
        if (!(ptDecoded))
            return FALSE;

        *pText = ptDecoded;
        FREE(ptDecoded);
        return TRUE;
    }

    BOOLEAN WriteUtf8TextFileInternal(LPCTSTR ptFilePath, const wstring &wsText)
    {
        HANDLE hFile;
        DWORD dWritten = 0;
        INT cbUtf8;
        LPSTR pcUtf8;

        if (!(ptFilePath))
            return FALSE;

        cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wsText.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (0 >= cbUtf8)
            return FALSE;

        pcUtf8 = (LPSTR)malloc(cbUtf8);
        if (!(pcUtf8))
            return FALSE;
        ZeroMemory(pcUtf8, cbUtf8);

        WideCharToMultiByte(CP_UTF8, 0, wsText.c_str(), -1, pcUtf8, cbUtf8, nullptr, nullptr);

        hFile = CreateFile(ptFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            FREE(pcUtf8);
            return FALSE;
        }

        WriteFile(hFile, pcUtf8, cbUtf8 - 1, &dWritten, nullptr);
        CloseHandle(hFile);
        FREE(pcUtf8);
        return TRUE;
    }

    BOOLEAN TextReadLine(wstring_view wsText, size_t *pixCaret, wstring *pLine)
    {
        size_t ixStart;

        if (!(pixCaret) || !(pLine) || wsText.size() <= *pixCaret)
            return FALSE;

        ixStart = *pixCaret;
        while (wsText.size() > *pixCaret && TEXT('\r') != wsText[*pixCaret] && TEXT('\n') != wsText[*pixCaret])
            (*pixCaret)++;
        pLine->assign(wsText.substr(ixStart, *pixCaret - ixStart));

        if (wsText.size() > *pixCaret && TEXT('\r') == wsText[*pixCaret])
            (*pixCaret)++;
        if (wsText.size() > *pixCaret && TEXT('\n') == wsText[*pixCaret])
            (*pixCaret)++;

        return TRUE;
    }

    wstring LegacyFrameTextDecode(wstring_view wsText)
    {
        wstring wsOut;

        for (size_t i = 0; wsText.size() > i; i++)
        {
            if (TEXT('\\') == wsText[i] && wsText.size() > (i + 1))
            {
                i++;
                if (TEXT('n') == wsText[i])
                {
                    wsOut += TEXT("\r\n");
                }
                else if (TEXT('s') == wsText[i])
                {
                    wsOut.push_back(TEXT(' '));
                }
                else
                {
                    wsOut.push_back(wsText[i]);
                }
                continue;
            }

            wsOut.push_back(wsText[i]);
        }

        return wsOut;
    }

    template<typename TRecord>
    BOOLEAN LoadLegacyVectorFile(LPCTSTR ptFilePath, vector<TRecord> *pRecords, LEGACY_VECTOR_PARSER<TRecord> pfParser)
    {
        wstring wsLegacy;

        if (!(ptFilePath) || !(pRecords) || !(pfParser))
            return FALSE;
        if (!(AppReadTextFile(ptFilePath, &wsLegacy)))
            return FALSE;

        return pfParser(wsLegacy, pRecords);
    }

    BOOLEAN ParseLegacyFlipText(const wstring &wsText, vector<JSON_FLIP_ENTRY> *pEntries)
    {
        size_t ixCaret = 0;
        wstring wsLine;
        BOOLEAN bHeaderSkipped = FALSE;

        if (!(pEntries))
            return FALSE;
        pEntries->clear();

        while (TextReadLine(wsText, &ixCaret, &wsLine))
        {
            size_t ixSplit;
            JSON_FLIP_ENTRY stEntry;

            if (wsLine.empty())
                continue;
            if (!(bHeaderSkipped))
            {
                bHeaderSkipped = TRUE;
                continue;
            }

            ixSplit = wsLine.find(TEXT('　'));
            if (wstring::npos == ixSplit)
                continue;

            stEntry.wsSource.assign(wsLine.substr(0, ixSplit));
            while (wsLine.size() > ixSplit && (TEXT('　') == wsLine[ixSplit] || TEXT(' ') == wsLine[ixSplit] || TEXT('\t') == wsLine[ixSplit]))
                ixSplit++;
            stEntry.wsTarget.assign(wsLine.substr(ixSplit));

            if (!(stEntry.wsSource.empty()) && !(stEntry.wsTarget.empty()))
            {
                pEntries->push_back(stEntry);
            }
        }

        return !(pEntries->empty());
    }

    BOOLEAN ParseLegacyPaletteText(const wstring &wsText, vector<JSON_TEMPLATE_GROUP> *pGroups)
    {
        size_t ixCaret = 0;
        wstring wsLine;
        JSON_TEMPLATE_GROUP *pstGroup = nullptr;

        if (!(pGroups))
            return FALSE;
        pGroups->clear();

        while (TextReadLine(wsText, &ixCaret, &wsLine))
        {
            if (11 <= wsLine.size() && 0 == wsLine.compare(0, 10, TEXT("[ListName=")) && TEXT(']') == wsLine.back())
            {
                JSON_TEMPLATE_GROUP stGroup;

                stGroup.wsName.assign(wsLine.substr(10, wsLine.size() - 11));
                if (stGroup.wsName.empty())
                    stGroup.wsName = TEXT("Nameless");
                pGroups->push_back(stGroup);
                pstGroup = &(pGroups->back());
                continue;
            }

            if (0 == wsLine.compare(TEXT("[end]")))
            {
                pstGroup = nullptr;
                continue;
            }

            if (pstGroup && !(wsLine.empty()))
            {
                pstGroup->vcItems.push_back(wsLine);
            }
        }

        return !(pGroups->empty());
    }

    BOOLEAN ParseLegacyFrameIniText(const wstring &wsText, vector<JSON_FRAME_RECORD> *pRecords)
    {
        size_t ixCaret = 0;
        wstring wsLine;
        INT iFrame = -1;

        if (!(pRecords))
            return FALSE;
        pRecords->clear();

        while (TextReadLine(wsText, &ixCaret, &wsLine))
        {
            if (wsLine.empty() || TEXT(';') == wsLine[0] || TEXT('#') == wsLine[0])
                continue;

            if (TEXT('[') == wsLine.front() && TEXT(']') == wsLine.back())
            {
                wstring wsSection = wsLine.substr(1, wsLine.size() - 2);
                if (5 <= wsSection.size() && 0 == StrCmpNI(wsSection.c_str(), TEXT("Frame"), 5))
                {
                    iFrame = StrToInt(wsSection.c_str() + 5);
                    if (0 <= iFrame)
                    {
                        if (pRecords->size() <= (size_t)iFrame)
                            pRecords->resize(iFrame + 1);
                        pRecords->at(iFrame).bRestPadding = TRUE;
                    }
                }
                else
                {
                    iFrame = -1;
                }
                continue;
            }

            if (0 > iFrame)
                continue;

            {
                size_t ixEq = wsLine.find(TEXT('='));
                wstring wsKey;
                wstring wsValue;

                if (wstring::npos == ixEq)
                    continue;
                wsKey.assign(wsLine.substr(0, ixEq));
                wsValue.assign(wsLine.substr(ixEq + 1));

                if (0 == StrCmpI(wsKey.c_str(), TEXT("Name")))
                {
                    pRecords->at(iFrame).wsName = wsValue;
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Daybreak")))
                {
                    pRecords->at(iFrame).wsDaybreak = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Morning")))
                {
                    pRecords->at(iFrame).wsMorning = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Noon")))
                {
                    pRecords->at(iFrame).wsNoon = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Afternoon")))
                {
                    pRecords->at(iFrame).wsAfternoon = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Nightfall")))
                {
                    pRecords->at(iFrame).wsNightfall = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Twilight")))
                {
                    pRecords->at(iFrame).wsTwilight = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Midnight")))
                {
                    pRecords->at(iFrame).wsMidnight = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("Dawn")))
                {
                    pRecords->at(iFrame).wsDawn = LegacyFrameTextDecode(wsValue);
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("LEFTOFFSET")))
                {
                    pRecords->at(iFrame).dLeftOffset = StrToInt(wsValue.c_str());
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("RIGHTOFFSET")))
                {
                    pRecords->at(iFrame).dRightOffset = StrToInt(wsValue.c_str());
                }
                else if (0 == StrCmpI(wsKey.c_str(), TEXT("RestPadding")))
                {
                    pRecords->at(iFrame).bRestPadding = (0 != StrToInt(wsValue.c_str()));
                }
            }
        }

        return !(pRecords->empty());
    }

    VOID MigrateLegacyFrameSettingsIfMissing(LPCTSTR ptLegacyPath, LPCTSTR ptTargetPath)
    {
        vector<JSON_FRAME_RECORD> vcFrames;

        if (!(ptLegacyPath) || !(ptTargetPath) || PathFileExists(ptTargetPath))
            return;

        if (LoadLegacyVectorFile(ptLegacyPath, &vcFrames, ParseLegacyFrameIniText))
        {
            AppWriteFrameRecords(ptTargetPath, vcFrames);
        }
    }

    VOID MigrateLegacyFlipSettingsIfMissing(LPCTSTR ptLegacyMirrorPath, LPCTSTR ptLegacyUpsetPath, LPCTSTR ptTargetPath)
    {
        vector<JSON_FLIP_ENTRY> vcMirror;
        vector<JSON_FLIP_ENTRY> vcUpset;
        BOOLEAN bLoaded = FALSE;

        if (!(ptLegacyMirrorPath) || !(ptLegacyUpsetPath) || !(ptTargetPath) || PathFileExists(ptTargetPath))
            return;

        if (LoadLegacyVectorFile(ptLegacyMirrorPath, &vcMirror, ParseLegacyFlipText))
            bLoaded = TRUE;
        if (LoadLegacyVectorFile(ptLegacyUpsetPath, &vcUpset, ParseLegacyFlipText))
            bLoaded = TRUE;

        if (bLoaded)
        {
            AppWriteFlipEntries(ptTargetPath, vcMirror, vcUpset);
        }
    }

    VOID MigrateLegacyPaletteSettingsIfMissing(LPCTSTR ptLegacyLinePath, LPCTSTR ptLegacyBrushPath, LPCTSTR ptTargetPath)
    {
        vector<JSON_TEMPLATE_GROUP> vcLine;
        vector<JSON_TEMPLATE_GROUP> vcBrush;
        BOOLEAN bLoaded = FALSE;

        if (!(ptLegacyLinePath) || !(ptLegacyBrushPath) || !(ptTargetPath) || PathFileExists(ptTargetPath))
            return;

        if (LoadLegacyVectorFile(ptLegacyLinePath, &vcLine, ParseLegacyPaletteText))
            bLoaded = TRUE;
        if (LoadLegacyVectorFile(ptLegacyBrushPath, &vcBrush, ParseLegacyPaletteText))
            bLoaded = TRUE;

        if (bLoaded)
        {
            AppWritePaletteGroups(ptTargetPath, vcLine, vcBrush);
        }
    }

} // namespace

BOOLEAN AppReadTextFile(LPCTSTR ptFilePath, wstring *pText)
{
    return ReadDecodedTextFile(ptFilePath, pText);
}

BOOLEAN AppWriteUtf8TextFile(LPCTSTR ptFilePath, const wstring &wsText)
{
    return WriteUtf8TextFileInternal(ptFilePath, wsText);
}

HRESULT AppMigrateLegacySettings(LPCTSTR ptExePath)
{
    TCHAR atLegacyPath[MAX_PATH];
    TCHAR atNewPath[MAX_PATH];
    TCHAR atSettingPath[MAX_PATH];
    TCHAR atTemplateDir[MAX_PATH];
    TCHAR atLegacySatori[MAX_PATH];
    TCHAR atLegacyMirror[MAX_PATH];
    TCHAR atLegacyUpset[MAX_PATH];
    TCHAR atLegacyLine[MAX_PATH];
    TCHAR atLegacyBrush[MAX_PATH];
    TCHAR atNewFrame[MAX_PATH];
    TCHAR atNewFlip[MAX_PATH];
    TCHAR atNewPalette[MAX_PATH];

    if (!(ptExePath))
        return E_INVALIDARG;

    StringCchCopy(atLegacyPath, MAX_PATH, ptExePath);
    PathAppend(atLegacyPath, LEGACY_SETTINGS_INI_FILE);
    StringCchCopy(atNewPath, MAX_PATH, ptExePath);
    PathAppend(atNewPath, SETTINGS_INI_FILE);
    if (!(PathFileExists(atNewPath)) && PathFileExists(atLegacyPath))
    {
        CopyFile(atLegacyPath, atNewPath, TRUE);
    }
    MigratePaletteWindowSettingsFromLegacyIni(atLegacyPath, atNewPath);

    StringCchCopy(atLegacyPath, MAX_PATH, ptExePath);
    PathAppend(atLegacyPath, LEGACY_SETTINGS_CONTEXT_MENU_INI_FILE);
    StringCchCopy(atNewPath, MAX_PATH, ptExePath);
    PathAppend(atNewPath, SETTINGS_CONTEXT_MENU_INI_FILE);
    if (!(PathFileExists(atNewPath)) && PathFileExists(atLegacyPath))
    {
        CopyFile(atLegacyPath, atNewPath, TRUE);
    }

    StringCchCopy(atTemplateDir, MAX_PATH, ptExePath);
    PathAppend(atTemplateDir, TEMPLATE_DIR);

    StringCchCopy(atLegacySatori, MAX_PATH, ptExePath);
    PathAppend(atLegacySatori, LEGACY_SETTINGS_FRAME_JSON_FILE);
    StringCchCopy(atNewFrame, MAX_PATH, atTemplateDir);
    PathAppend(atNewFrame, SETTINGS_FRAME_JSON_FILE);
    MigrateLegacyFrameSettingsIfMissing(atLegacySatori, atNewFrame);

    StringCchCopy(atLegacyMirror, MAX_PATH, atTemplateDir);
    PathAppend(atLegacyMirror, LEGACY_AA_MIRROR_FILE);
    StringCchCopy(atLegacyUpset, MAX_PATH, atTemplateDir);
    PathAppend(atLegacyUpset, LEGACY_AA_UPSET_FILE);
    StringCchCopy(atNewFlip, MAX_PATH, atTemplateDir);
    PathAppend(atNewFlip, FLIP_FILE);
    MigrateLegacyFlipSettingsIfMissing(atLegacyMirror, atLegacyUpset, atNewFlip);

    StringCchCopy(atLegacyLine, MAX_PATH, atTemplateDir);
    PathAppend(atLegacyLine, LEGACY_AA_LIST_FILE);
    StringCchCopy(atLegacyBrush, MAX_PATH, atTemplateDir);
    PathAppend(atLegacyBrush, LEGACY_AA_BRUSH_FILE);
    StringCchCopy(atNewPalette, MAX_PATH, atTemplateDir);
    PathAppend(atNewPalette, AA_PALETTE_FILE);
    MigrateLegacyPaletteSettingsIfMissing(atLegacyLine, atLegacyBrush, atNewPalette);

    StringCchCopy(atSettingPath, MAX_PATH, ptExePath);
    PathAppend(atSettingPath, SETTINGS_INI_FILE);
    MigrateSettingIniKeyNames(atSettingPath);
    CleanupSettingIni(atSettingPath);

    return S_OK;
}
