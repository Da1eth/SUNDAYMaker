// 설정 JSON 입출력
#include "Sunday.h"
#include "json.hpp"

using nlohmann::ordered_json;

namespace
{
    string JsonWideToUtf8(wstring_view wsText)
    {
        int cbNeeded;
        string sUtf8;

        if (wsText.empty())
            return {};

        cbNeeded = WideCharToMultiByte(CP_UTF8, 0, wsText.data(), (int)wsText.size(), nullptr, 0, nullptr, nullptr);
        if (0 >= cbNeeded)
            return {};

        sUtf8.resize(cbNeeded);
        if (0 >= WideCharToMultiByte(CP_UTF8, 0, wsText.data(), (int)wsText.size(), sUtf8.data(), cbNeeded, nullptr, nullptr))
            return {};

        return sUtf8;
    }

    BOOLEAN JsonUtf8ToWide(string_view sUtf8, wstring *pText)
    {
        int cchNeeded;

        if (!(pText))
            return FALSE;

        pText->clear();
        if (sUtf8.empty())
            return TRUE;

        cchNeeded = MultiByteToWideChar(CP_UTF8, 0, sUtf8.data(), (int)sUtf8.size(), nullptr, 0);
        if (0 >= cchNeeded)
            return FALSE;

        pText->resize(cchNeeded);
        return 0 < MultiByteToWideChar(CP_UTF8, 0, sUtf8.data(), (int)sUtf8.size(), pText->data(), cchNeeded);
    }

    HRESULT JsonReadDocument(LPCTSTR ptFilePath, ordered_json *pDocument)
    {
        string sUtf8;
        wstring wsJson;

        if (!(pDocument))
            return E_INVALIDARG;
        if (!(AppReadTextFile(ptFilePath, &wsJson)))
            return E_HANDLE;

        sUtf8 = JsonWideToUtf8(wsJson);

        try
        {
            *pDocument = ordered_json::parse(sUtf8);
        }
        catch (const nlohmann::json::exception &)
        {
            return E_FAIL;
        }

        return S_OK;
    }

    HRESULT JsonWriteDocument(LPCTSTR ptFilePath, const ordered_json &stDocument)
    {
        string sUtf8;
        wstring wsJson;

        sUtf8 = stDocument.dump(2);
        sUtf8 += '\n';

        if (!(JsonUtf8ToWide(sUtf8, &wsJson)))
            return E_FAIL;

        return AppWriteUtf8TextFile(ptFilePath, wsJson) ? S_OK : E_FAIL;
    }

    const ordered_json *JsonFindMember(const ordered_json &stObject, const char *pcKey)
    {
        ordered_json::const_iterator itFind;

        if (!(stObject.is_object()))
            return nullptr;

        itFind = stObject.find(pcKey);
        if (stObject.end() == itFind)
            return nullptr;

        return &(*itFind);
    }

    BOOLEAN JsonReadWideStringValue(const ordered_json &stValue, wstring *pText)
    {
        if (!(pText) || !(stValue.is_string()))
            return FALSE;

        return JsonUtf8ToWide(stValue.get_ref<const string &>(), pText);
    }

    BOOLEAN JsonReadOptionalWideStringField(const ordered_json &stObject, const char *pcKey, wstring *pText)
    {
        const ordered_json *pstValue;

        if (!(pText))
            return FALSE;

        pstValue = JsonFindMember(stObject, pcKey);
        if (!(pstValue))
        {
            pText->clear();
            return TRUE;
        }

        return JsonReadWideStringValue(*pstValue, pText);
    }

    BOOLEAN JsonReadOptionalIntField(const ordered_json &stObject, const char *pcKey, INT *piValue)
    {
        const ordered_json *pstValue;

        if (!(piValue))
            return FALSE;

        pstValue = JsonFindMember(stObject, pcKey);
        if (!(pstValue))
        {
            *piValue = 0;
            return TRUE;
        }

        if (!(pstValue->is_number_integer()))
            return FALSE;

        *piValue = pstValue->get<INT>();
        return TRUE;
    }

    BOOLEAN JsonReadOptionalBooleanField(const ordered_json &stObject, const char *pcKey, BOOLEAN *pbValue)
    {
        const ordered_json *pstValue;

        if (!(pbValue))
            return FALSE;

        pstValue = JsonFindMember(stObject, pcKey);
        if (!(pstValue))
            return TRUE;

        if (pstValue->is_boolean())
        {
            *pbValue = pstValue->get<bool>() ? TRUE : FALSE;
            return TRUE;
        }
        if (pstValue->is_number_integer())
        {
            *pbValue = (0 != pstValue->get<INT>()) ? TRUE : FALSE;
            return TRUE;
        }

        return FALSE;
    }

    ordered_json JsonWideStringValue(wstring_view wsText)
    {
        return ordered_json(JsonWideToUtf8(wsText));
    }

    HRESULT JsonLoadFlipEntryArray(const ordered_json &stRoot, const char *pcKey, vector<JSON_FLIP_ENTRY> *pEntries)
    {
        const ordered_json *pstArray;

        if (!(pEntries))
            return E_INVALIDARG;

        pEntries->clear();
        pstArray = JsonFindMember(stRoot, pcKey);
        if (!(pstArray) || !(pstArray->is_array()))
            return E_FAIL;

        for (const ordered_json &stItem : *pstArray)
        {
            JSON_FLIP_ENTRY stEntry;

            if (!(stItem.is_object()))
                return E_FAIL;
            if (!(JsonReadOptionalWideStringField(stItem, "source", &(stEntry.wsSource))))
                return E_FAIL;
            if (!(JsonReadOptionalWideStringField(stItem, "target", &(stEntry.wsTarget))))
                return E_FAIL;
            pEntries->push_back(stEntry);
        }

        return S_OK;
    }

    HRESULT JsonLoadTemplateGroupArray(const ordered_json &stRoot, const char *pcKey, vector<JSON_TEMPLATE_GROUP> *pGroups)
    {
        const ordered_json *pstArray;

        if (!(pGroups))
            return E_INVALIDARG;

        pGroups->clear();
        pstArray = JsonFindMember(stRoot, pcKey);
        if (!(pstArray) || !(pstArray->is_array()))
            return E_FAIL;

        for (const ordered_json &stItem : *pstArray)
        {
            JSON_TEMPLATE_GROUP stGroup;
            const ordered_json *pstItems;

            if (!(stItem.is_object()))
                return E_FAIL;
            if (!(JsonReadOptionalWideStringField(stItem, "name", &(stGroup.wsName))))
                return E_FAIL;

            pstItems = JsonFindMember(stItem, "items");
            if (pstItems)
            {
                if (!(pstItems->is_array()))
                    return E_FAIL;
                for (const ordered_json &stValue : *pstItems)
                {
                    wstring wsItem;

                    if (!(JsonReadWideStringValue(stValue, &wsItem)))
                        return E_FAIL;
                    stGroup.vcItems.push_back(wsItem);
                }
            }

            if (stGroup.wsName.empty())
                stGroup.wsName = TEXT("Nameless");

            pGroups->push_back(stGroup);
        }

        return S_OK;
    }

    HRESULT JsonLoadFrameRecordArray(const ordered_json &stRoot, vector<JSON_FRAME_RECORD> *pRecords)
    {
        const ordered_json *pstArray;

        if (!(pRecords))
            return E_INVALIDARG;

        pRecords->clear();
        pstArray = JsonFindMember(stRoot, "frames");
        if (!(pstArray) || !(pstArray->is_array()))
            return E_FAIL;

        for (const ordered_json &stItem : *pstArray)
        {
            JSON_FRAME_RECORD stRecord;
            const ordered_json *pstParts;
            const ordered_json *pstOffset;

            if (!(stItem.is_object()))
                return E_FAIL;

            stRecord.bRestPadding = TRUE;
            if (!(JsonReadOptionalWideStringField(stItem, "name", &(stRecord.wsName))))
                return E_FAIL;

            pstParts = JsonFindMember(stItem, "parts");
            if (pstParts)
            {
                if (!(pstParts->is_object()))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "daybreak", &(stRecord.wsDaybreak))))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "morning", &(stRecord.wsMorning))))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "noon", &(stRecord.wsNoon))))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "afternoon", &(stRecord.wsAfternoon))))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "nightfall", &(stRecord.wsNightfall))))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "twilight", &(stRecord.wsTwilight))))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "midnight", &(stRecord.wsMidnight))))
                    return E_FAIL;
                if (!(JsonReadOptionalWideStringField(*pstParts, "dawn", &(stRecord.wsDawn))))
                    return E_FAIL;
            }

            pstOffset = JsonFindMember(stItem, "offset");
            if (pstOffset)
            {
                if (!(pstOffset->is_object()))
                    return E_FAIL;
                if (!(JsonReadOptionalIntField(*pstOffset, "left", &(stRecord.dLeftOffset))))
                    return E_FAIL;
                if (!(JsonReadOptionalIntField(*pstOffset, "right", &(stRecord.dRightOffset))))
                    return E_FAIL;
            }

            if (!(JsonReadOptionalBooleanField(stItem, "restPadding", &(stRecord.bRestPadding))))
                return E_FAIL;

            pRecords->push_back(stRecord);
        }

        return S_OK;
    }
} // namespace

HRESULT AppLoadFlipEntries(LPCTSTR ptFilePath, BOOLEAN bMirror, vector<JSON_FLIP_ENTRY> *pEntries)
{
    ordered_json stRoot;
    HRESULT hRslt;

    hRslt = JsonReadDocument(ptFilePath, &stRoot);
    if (FAILED(hRslt))
        return hRslt;

    return JsonLoadFlipEntryArray(stRoot, bMirror ? "horizontal" : "vertical", pEntries);
}

HRESULT AppWriteFlipEntries(LPCTSTR ptFilePath, const vector<JSON_FLIP_ENTRY> &vcMirror, const vector<JSON_FLIP_ENTRY> &vcUpset)
{
    ordered_json stRoot;

    stRoot["horizontal"] = ordered_json::array();
    for (const JSON_FLIP_ENTRY &stEntry : vcMirror)
    {
        ordered_json stItem;

        stItem["source"] = JsonWideStringValue(stEntry.wsSource);
        stItem["target"] = JsonWideStringValue(stEntry.wsTarget);
        stRoot["horizontal"].push_back(stItem);
    }

    stRoot["vertical"] = ordered_json::array();
    for (const JSON_FLIP_ENTRY &stEntry : vcUpset)
    {
        ordered_json stItem;

        stItem["source"] = JsonWideStringValue(stEntry.wsSource);
        stItem["target"] = JsonWideStringValue(stEntry.wsTarget);
        stRoot["vertical"].push_back(stItem);
    }

    return JsonWriteDocument(ptFilePath, stRoot);
}

HRESULT AppLoadPaletteGroups(LPCTSTR ptFilePath, BOOLEAN bBrush, vector<JSON_TEMPLATE_GROUP> *pGroups)
{
    ordered_json stRoot;
    HRESULT hRslt;

    hRslt = JsonReadDocument(ptFilePath, &stRoot);
    if (FAILED(hRslt))
        return hRslt;

    return JsonLoadTemplateGroupArray(stRoot, bBrush ? "brushTemplates" : "lineTemplates", pGroups);
}

HRESULT AppWritePaletteGroups(LPCTSTR ptFilePath, const vector<JSON_TEMPLATE_GROUP> &vcLine, const vector<JSON_TEMPLATE_GROUP> &vcBrush)
{
    ordered_json stRoot;

    stRoot["lineTemplates"] = ordered_json::array();
    for (const JSON_TEMPLATE_GROUP &stGroup : vcLine)
    {
        ordered_json stItem;

        stItem["name"] = JsonWideStringValue(stGroup.wsName);
        stItem["items"] = ordered_json::array();
        for (const wstring &wsItem : stGroup.vcItems)
        {
            stItem["items"].push_back(JsonWideStringValue(wsItem));
        }

        stRoot["lineTemplates"].push_back(stItem);
    }

    stRoot["brushTemplates"] = ordered_json::array();
    for (const JSON_TEMPLATE_GROUP &stGroup : vcBrush)
    {
        ordered_json stItem;

        stItem["name"] = JsonWideStringValue(stGroup.wsName);
        stItem["items"] = ordered_json::array();
        for (const wstring &wsItem : stGroup.vcItems)
        {
            stItem["items"].push_back(JsonWideStringValue(wsItem));
        }

        stRoot["brushTemplates"].push_back(stItem);
    }

    return JsonWriteDocument(ptFilePath, stRoot);
}

HRESULT AppLoadFrameRecords(LPCTSTR ptFilePath, vector<JSON_FRAME_RECORD> *pRecords)
{
    ordered_json stRoot;
    HRESULT hRslt;

    hRslt = JsonReadDocument(ptFilePath, &stRoot);
    if (FAILED(hRslt))
        return hRslt;

    return JsonLoadFrameRecordArray(stRoot, pRecords);
}

HRESULT AppWriteFrameRecords(LPCTSTR ptFilePath, const vector<JSON_FRAME_RECORD> &vcRecords)
{
    ordered_json stRoot;

    stRoot["frames"] = ordered_json::array();
    for (const JSON_FRAME_RECORD &stRecord : vcRecords)
    {
        ordered_json stItem;
        ordered_json stParts;
        ordered_json stOffset;

        stItem["name"] = JsonWideStringValue(stRecord.wsName);

        stParts["daybreak"] = JsonWideStringValue(stRecord.wsDaybreak);
        stParts["morning"] = JsonWideStringValue(stRecord.wsMorning);
        stParts["noon"] = JsonWideStringValue(stRecord.wsNoon);
        stParts["afternoon"] = JsonWideStringValue(stRecord.wsAfternoon);
        stParts["nightfall"] = JsonWideStringValue(stRecord.wsNightfall);
        stParts["twilight"] = JsonWideStringValue(stRecord.wsTwilight);
        stParts["midnight"] = JsonWideStringValue(stRecord.wsMidnight);
        stParts["dawn"] = JsonWideStringValue(stRecord.wsDawn);
        stItem["parts"] = stParts;

        stOffset["left"] = stRecord.dLeftOffset;
        stOffset["right"] = stRecord.dRightOffset;
        stItem["offset"] = stOffset;

        stItem["restPadding"] = (FALSE != stRecord.bRestPadding);
        stRoot["frames"].push_back(stItem);
    }

    return JsonWriteDocument(ptFilePath, stRoot);
}
