// 레거시 설정 마이그레이션
#include "Sunday.h"

namespace
{
    template<typename TRecord>
    using LEGACY_VECTOR_PARSER = BOOLEAN (*)(const wstring &, vector<TRecord> *);

    using PATH_BUFFER = array<TCHAR, MAX_PATH>;

    struct LEGACY_WINDOW_SECTION
    {
        LPCTSTR ptLegacySection;
        LPCTSTR ptTargetSection;
    };

    struct LEGACY_FILE_PAIR
    {
        LPCTSTR ptLegacyPath;
        LPCTSTR ptTargetPath;
    };

    struct AUTO_FILE_HANDLE
    {
        HANDLE hValue{INVALID_HANDLE_VALUE};

        AUTO_FILE_HANDLE() = default;
        explicit AUTO_FILE_HANDLE(HANDLE hHandle) : hValue(hHandle) {}

        AUTO_FILE_HANDLE(const AUTO_FILE_HANDLE &) = delete;
        AUTO_FILE_HANDLE &operator=(const AUTO_FILE_HANDLE &) = delete;

        AUTO_FILE_HANDLE(AUTO_FILE_HANDLE &&stOther) noexcept : hValue(stOther.hValue)
        {
            stOther.hValue = INVALID_HANDLE_VALUE;
        }

        AUTO_FILE_HANDLE &operator=(AUTO_FILE_HANDLE &&stOther) noexcept
        {
            if (this != &stOther)
            {
                Close();
                hValue = stOther.hValue;
                stOther.hValue = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        ~AUTO_FILE_HANDLE()
        {
            Close();
        }

        VOID Close()
        {
            if (INVALID_HANDLE_VALUE != hValue && nullptr != hValue)
            {
                CloseHandle(hValue);
                hValue = INVALID_HANDLE_VALUE;
            }
        }

        BOOLEAN IsValid() const
        {
            return INVALID_HANDLE_VALUE != hValue && nullptr != hValue;
        }

        HANDLE Get() const
        {
            return hValue;
        }
    };

    struct LEGACY_FRAME_TEXT_FIELD
    {
        LPCTSTR ptKey;
        wstring JSON_FRAME_RECORD::*pField;
        BOOLEAN bDecode;
    };

    BOOLEAN PathExists(LPCTSTR ptFilePath)
    {
        return ptFilePath && PathFileExists(ptFilePath);
    }

    PATH_BUFFER BuildPath(LPCTSTR ptBasePath, LPCTSTR ptLeafName)
    {
        PATH_BUFFER atPath{};

        if (!(ptBasePath) || !(ptLeafName))
            return atPath;

        StringCchCopy(atPath.data(), atPath.size(), ptBasePath);
        PathAppend(atPath.data(), ptLeafName);
        return atPath;
    }

    VOID CopyLegacyFileIfMissing(LPCTSTR ptLegacyPath, LPCTSTR ptTargetPath)
    {
        if (!(PathExists(ptLegacyPath)) || PathExists(ptTargetPath))
            return;

        CopyFile(ptLegacyPath, ptTargetPath, TRUE);
    }

    VOID MigrateSettingIniKeyNames(LPCTSTR ptFilePath)
    {
        TCHAR atValue[MIN_STRING];
        TCHAR atExisting[MIN_STRING];

        if (!(PathExists(ptFilePath)))
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
        if (!(PathExists(ptSourcePath)))
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
        static constexpr auto gastSections = to_array<LEGACY_WINDOW_SECTION>({
            {TEXT("LineTmple"), TEXT("PalInsert")},
            {TEXT("BrushTmple"), TEXT("PalBucket")}});
        static constexpr auto gaptKeys = to_array<LPCTSTR>({
            TEXT("LEFT"),
            TEXT("TOP"),
            TEXT("RIGHT"),
            TEXT("BOTTOM"),
            TEXT("TopMost")});

        if (!(ptLegacyPath) || !(ptTargetPath) || !(PathExists(ptLegacyPath)))
            return;

        for (const auto &stSection : gastSections)
        {
            for (LPCTSTR ptKey : gaptKeys)
            {
                MigrateIniValueIfMissing(ptLegacyPath, stSection.ptLegacySection, ptKey, ptTargetPath, stSection.ptTargetSection, ptKey);
            }
        }
    }

    VOID CleanupSettingIni(LPCTSTR ptFilePath)
    {
        static constexpr auto gaptGeneralKeys = to_array<LPCTSTR>({
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
            TEXT("WorkLogDebug")});

        if (!(PathExists(ptFilePath)))
            return;

        for (LPCTSTR ptKey : gaptGeneralKeys)
        {
            WritePrivateProfileString(TEXT("General"), ptKey, nullptr, ptFilePath);
        }

        WritePrivateProfileString(TEXT("Colour"), TEXT("CantSjis"), nullptr, ptFilePath);
        WritePrivateProfileString(TEXT("History"), TEXT("LastOpen"), nullptr, ptFilePath);

        WritePrivateProfileSection(TEXT("ProfHistory"), nullptr, ptFilePath);

        WritePrivateProfileSection(TEXT("History"), nullptr, ptFilePath);
        WritePrivateProfileSection(TEXT("MaaTmple"), nullptr, ptFilePath);
    }

    BOOLEAN DeleteFileIfExists(LPCTSTR ptFilePath)
    {
        if (!(PathExists(ptFilePath)))
            return TRUE;

        return DeleteFile(ptFilePath);
    }

    BOOLEAN DeleteLegacyFileIfTargetExists(LPCTSTR ptLegacyPath, LPCTSTR ptTargetPath)
    {
        if (!(PathExists(ptLegacyPath)))
            return TRUE;
        if (!(PathExists(ptTargetPath)))
            return FALSE;

        return DeleteFile(ptLegacyPath);
    }

    VOID CleanupLegacyMigrationFiles(
        LPCTSTR ptLegacyIniPath,
        LPCTSTR ptLegacySqliteDllPath,
        LPCTSTR ptLegacySatoriPath,
        LPCTSTR ptLegacyKoisiPath,
        LPCTSTR ptLegacyImgctlDllPath,
        LPCTSTR ptLegacyBrushPath,
        LPCTSTR ptLegacyListPath,
        LPCTSTR ptLegacyMirrorPath,
        LPCTSTR ptLegacyUpsetPath,
        LPCTSTR ptLegacyPreviewPath,
        LPCTSTR ptSettingPath,
        LPCTSTR ptAccelKeyPath,
        LPCTSTR ptFramePath,
        LPCTSTR ptFlipPath,
        LPCTSTR ptPalettePath)
    {
        const array<LPCTSTR, 3> gaptStandaloneFiles = {
            ptLegacySqliteDllPath,
            ptLegacyImgctlDllPath,
            ptLegacyPreviewPath};
        const array<LEGACY_FILE_PAIR, 7> gaptMigratedTargets = {
            LEGACY_FILE_PAIR{ptLegacyIniPath, ptSettingPath},
            LEGACY_FILE_PAIR{ptLegacyKoisiPath, ptAccelKeyPath},
            LEGACY_FILE_PAIR{ptLegacySatoriPath, ptFramePath},
            LEGACY_FILE_PAIR{ptLegacyMirrorPath, ptFlipPath},
            LEGACY_FILE_PAIR{ptLegacyUpsetPath, ptFlipPath},
            LEGACY_FILE_PAIR{ptLegacyListPath, ptPalettePath},
            LEGACY_FILE_PAIR{ptLegacyBrushPath, ptPalettePath}};

        for (const auto &stTarget : gaptMigratedTargets)
        {
            DeleteLegacyFileIfTargetExists(stTarget.ptLegacyPath, stTarget.ptTargetPath);
        }

        for (LPCTSTR ptFilePath : gaptStandaloneFiles)
        {
            DeleteFileIfExists(ptFilePath);
        }
    }

    BOOLEAN ReadDecodedTextFile(LPCTSTR ptFilePath, wstring *pText)
    {
        DWORD dRead = 0;
        DWORD dFileSize;
        vector<BYTE> vcBuffer;
        LPTSTR ptDecoded;

        if (!(ptFilePath) || !(pText))
            return FALSE;

        pText->clear();
        ptDecoded = nullptr;

        AUTO_FILE_HANDLE hFile(CreateFile(ptFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!(hFile.IsValid()))
            return FALSE;

        dFileSize = GetFileSize(hFile.Get(), nullptr);
        if (INVALID_FILE_SIZE == dFileSize && NO_ERROR != GetLastError())
            return FALSE;

        vcBuffer.resize(static_cast<size_t>(dFileSize) + 4, 0);

        if (!ReadFile(hFile.Get(), vcBuffer.data(), dFileSize, &dRead, nullptr))
            return FALSE;

        ptDecoded = TextDecodeAutoAlloc(vcBuffer.data(), (INT)dFileSize, nullptr);
        if (!(ptDecoded))
            return FALSE;

        *pText = ptDecoded;
        FREE(ptDecoded);
        return TRUE;
    }

    BOOLEAN WriteUtf8TextFileInternal(LPCTSTR ptFilePath, const wstring &wsText)
    {
        DWORD dWritten = 0;
        const INT cchWide = static_cast<INT>(wsText.size());
        const INT cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wsText.c_str(), cchWide, nullptr, 0, nullptr, nullptr);
        string scUtf8;

        if (!(ptFilePath))
            return FALSE;

        if (0 > cbUtf8 || (0 == cbUtf8 && !(wsText.empty())))
            return FALSE;

        scUtf8.resize(cbUtf8);
        if (!(wsText.empty()))
        {
            WideCharToMultiByte(CP_UTF8, 0, wsText.c_str(), cchWide, scUtf8.data(), cbUtf8, nullptr, nullptr);
        }

        AUTO_FILE_HANDLE hFile(CreateFile(ptFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!(hFile.IsValid()))
            return FALSE;

        if (!(scUtf8.empty()) && !WriteFile(hFile.Get(), scUtf8.data(), static_cast<DWORD>(scUtf8.size()), &dWritten, nullptr))
            return FALSE;

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

    BOOLEAN ApplyLegacyFrameTextField(JSON_FRAME_RECORD *pstRecord, const wstring &wsKey, const wstring &wsValue)
    {
        static constexpr auto gastTextFields = to_array<LEGACY_FRAME_TEXT_FIELD>({
            {TEXT("Name"), &JSON_FRAME_RECORD::wsName, FALSE},
            {TEXT("Daybreak"), &JSON_FRAME_RECORD::wsDaybreak, TRUE},
            {TEXT("Morning"), &JSON_FRAME_RECORD::wsMorning, TRUE},
            {TEXT("Noon"), &JSON_FRAME_RECORD::wsNoon, TRUE},
            {TEXT("Afternoon"), &JSON_FRAME_RECORD::wsAfternoon, TRUE},
            {TEXT("Nightfall"), &JSON_FRAME_RECORD::wsNightfall, TRUE},
            {TEXT("Twilight"), &JSON_FRAME_RECORD::wsTwilight, TRUE},
            {TEXT("Midnight"), &JSON_FRAME_RECORD::wsMidnight, TRUE},
            {TEXT("Dawn"), &JSON_FRAME_RECORD::wsDawn, TRUE}});

        if (!(pstRecord))
            return FALSE;

        for (const auto &stField : gastTextFields)
        {
            if (0 != StrCmpI(wsKey.c_str(), stField.ptKey))
                continue;

            pstRecord->*(stField.pField) = stField.bDecode ? LegacyFrameTextDecode(wsValue) : wsValue;
            return TRUE;
        }

        return FALSE;
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

                if (ApplyLegacyFrameTextField(&(pRecords->at(iFrame)), wsKey, wsValue))
                {
                    continue;
                }

                if (0 == StrCmpI(wsKey.c_str(), TEXT("LEFTOFFSET")))
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

        if (!(ptLegacyPath) || !(ptTargetPath) || PathExists(ptTargetPath))
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

        if (!(ptLegacyMirrorPath) || !(ptLegacyUpsetPath) || !(ptTargetPath) || PathExists(ptTargetPath))
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

        if (!(ptLegacyLinePath) || !(ptLegacyBrushPath) || !(ptTargetPath) || PathExists(ptTargetPath))
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
    if (!(ptExePath))
        return E_INVALIDARG;

    const PATH_BUFFER atLegacyIni = BuildPath(ptExePath, LEGACY_SETTINGS_INI_FILE);
    const PATH_BUFFER atSettingPath = BuildPath(ptExePath, SETTINGS_INI_FILE);
    const PATH_BUFFER atLegacyKoisi = BuildPath(ptExePath, LEGACY_SETTINGS_CONTEXT_MENU_INI_FILE);
    const PATH_BUFFER atAccelKeyPath = BuildPath(ptExePath, SETTINGS_CONTEXT_MENU_INI_FILE);
    const PATH_BUFFER atTemplateDir = BuildPath(ptExePath, TEMPLATE_DIR);
    const PATH_BUFFER atLegacySatori = BuildPath(ptExePath, LEGACY_SETTINGS_FRAME_JSON_FILE);
    const PATH_BUFFER atNewFrame = BuildPath(atTemplateDir.data(), SETTINGS_FRAME_JSON_FILE);
    const PATH_BUFFER atLegacyMirror = BuildPath(atTemplateDir.data(), LEGACY_AA_MIRROR_FILE);
    const PATH_BUFFER atLegacyUpset = BuildPath(atTemplateDir.data(), LEGACY_AA_UPSET_FILE);
    const PATH_BUFFER atNewFlip = BuildPath(atTemplateDir.data(), FLIP_FILE);
    const PATH_BUFFER atLegacyLine = BuildPath(atTemplateDir.data(), LEGACY_AA_LIST_FILE);
    const PATH_BUFFER atLegacyBrush = BuildPath(atTemplateDir.data(), LEGACY_AA_BRUSH_FILE);
    const PATH_BUFFER atNewPalette = BuildPath(atTemplateDir.data(), AA_PALETTE_FILE);
    const PATH_BUFFER atLegacyPreview = BuildPath(atTemplateDir.data(), TEXT("Preview.htm"));
    const PATH_BUFFER atLegacySqliteDll = BuildPath(ptExePath, TEXT("sqlite3.dll"));
    const PATH_BUFFER atLegacyImgctlDll = BuildPath(ptExePath, TEXT("Imgctl.dll"));

    CopyLegacyFileIfMissing(atLegacyIni.data(), atSettingPath.data());
    MigratePaletteWindowSettingsFromLegacyIni(atLegacyIni.data(), atSettingPath.data());

    CopyLegacyFileIfMissing(atLegacyKoisi.data(), atAccelKeyPath.data());

    MigrateLegacyFrameSettingsIfMissing(atLegacySatori.data(), atNewFrame.data());
    MigrateLegacyFlipSettingsIfMissing(atLegacyMirror.data(), atLegacyUpset.data(), atNewFlip.data());
    MigrateLegacyPaletteSettingsIfMissing(atLegacyLine.data(), atLegacyBrush.data(), atNewPalette.data());

    MigrateSettingIniKeyNames(atSettingPath.data());
    CleanupSettingIni(atSettingPath.data());

    CleanupLegacyMigrationFiles(
        atLegacyIni.data(),
        atLegacySqliteDll.data(),
        atLegacySatori.data(),
        atLegacyKoisi.data(),
        atLegacyImgctlDll.data(),
        atLegacyBrush.data(),
        atLegacyLine.data(),
        atLegacyMirror.data(),
        atLegacyUpset.data(),
        atLegacyPreview.data(),
        atSettingPath.data(),
        atAccelKeyPath.data(),
        atNewFrame.data(),
        atNewFlip.data(),
        atNewPalette.data());

    return S_OK;
}
