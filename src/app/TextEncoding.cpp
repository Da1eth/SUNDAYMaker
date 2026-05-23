#include "pch.h"
#include "TextEncoding.h"

namespace
{
    inline BOOL AsciiIsDigit(unsigned char ch)
    {
        return ('0' <= ch) && (ch <= '9');
    }

    inline BOOL AsciiIsHexDigit(unsigned char ch)
    {
        return AsciiIsDigit(ch) || (('A' <= ch) && (ch <= 'F')) || (('a' <= ch) && (ch <= 'f'));
    }

    LPSTR AsciiEntityFind(LPSTR pcText)
    {
        if (!(pcText))
        {
            return nullptr;
        }

        for (; pcText[0] != 0x00; pcText++)
        {
            if (('&' == pcText[0]) && ('#' == pcText[1]))
            {
                return pcText;
            }
        }

        return nullptr;
    }

    typedef struct tagMBCCHECK
    {
        UINT dPairCount;
        UINT dKanaCount;
        UINT dInvalidCount;
    } MBCCHECK;

    BOOL Utf8LeadByteCheck(BYTE dByte, PUINT pdLength)
    {
        if (dByte < 0x80)
        {
            *pdLength = 1;
            return TRUE;
        }
        if (0xC2 <= dByte && dByte <= 0xDF)
        {
            *pdLength = 2;
            return TRUE;
        }
        if (0xE0 <= dByte && dByte <= 0xEF)
        {
            *pdLength = 3;
            return TRUE;
        }
        if (0xF0 <= dByte && dByte <= 0xF4)
        {
            *pdLength = 4;
            return TRUE;
        }

        *pdLength = 0;
        return FALSE;
    }

    BOOL Utf8BufferCheck(const BYTE *pdData, INT cbSize, PUINT pdScore)
    {
        INT i;
        UINT dLength;
        UINT dScore = 0;

        for (i = 0; cbSize > i;)
        {
            if (pdData[i] < 0x80)
            {
                i++;
                continue;
            }

            if (!(Utf8LeadByteCheck(pdData[i], &dLength)))
            {
                return FALSE;
            }
            if ((i + (INT)dLength) > cbSize)
            {
                return FALSE;
            }

            if (2 == dLength)
            {
                if ((pdData[i + 1] & 0xC0) != 0x80)
                    return FALSE;
            }
            else if (3 == dLength)
            {
                if ((pdData[i + 1] & 0xC0) != 0x80)
                    return FALSE;
                if ((pdData[i + 2] & 0xC0) != 0x80)
                    return FALSE;
                if (0xE0 == pdData[i] && pdData[i + 1] < 0xA0)
                    return FALSE;
                if (0xED == pdData[i] && 0xA0 <= pdData[i + 1])
                    return FALSE;
            }
            else if (4 == dLength)
            {
                if ((pdData[i + 1] & 0xC0) != 0x80)
                    return FALSE;
                if ((pdData[i + 2] & 0xC0) != 0x80)
                    return FALSE;
                if ((pdData[i + 3] & 0xC0) != 0x80)
                    return FALSE;
                if (0xF0 == pdData[i] && pdData[i + 1] < 0x90)
                    return FALSE;
                if (0xF4 == pdData[i] && 0x90 <= pdData[i + 1])
                    return FALSE;
            }

            dScore += dLength;
            i += dLength;
        }

        if (pdScore)
            *pdScore = dScore;
        return TRUE;
    }

    VOID Cp932BufferCheck(const BYTE *pdData, INT cbSize, MBCCHECK *pstCheck)
    {
        INT i;
        BYTE dByte, dNext;

        ZeroMemory(pstCheck, sizeof(MBCCHECK));

        for (i = 0; cbSize > i; i++)
        {
            dByte = pdData[i];
            if (dByte < 0x80)
                continue;

            if (0xA1 <= dByte && dByte <= 0xDF)
            {
                pstCheck->dKanaCount++;
                continue;
            }

            if ((0x81 <= dByte && dByte <= 0x9F) || (0xE0 <= dByte && dByte <= 0xFC))
            {
                if ((i + 1) >= cbSize)
                {
                    pstCheck->dInvalidCount++;
                    break;
                }

                dNext = pdData[i + 1];
                if ((0x40 <= dNext && dNext <= 0x7E) || (0x80 <= dNext && dNext <= 0xFC))
                {
                    pstCheck->dPairCount++;
                    i++;
                    continue;
                }
            }

            pstCheck->dInvalidCount++;
        }
    }

    VOID Cp949BufferCheck(const BYTE *pdData, INT cbSize, MBCCHECK *pstCheck)
    {
        INT i;
        BYTE dByte, dNext;

        ZeroMemory(pstCheck, sizeof(MBCCHECK));

        for (i = 0; cbSize > i; i++)
        {
            dByte = pdData[i];
            if (dByte < 0x80)
                continue;

            if (0x81 <= dByte && dByte <= 0xFE)
            {
                if ((i + 1) >= cbSize)
                {
                    pstCheck->dInvalidCount++;
                    break;
                }

                dNext = pdData[i + 1];
                if ((0x41 <= dNext && dNext <= 0x5A) ||
                    (0x61 <= dNext && dNext <= 0x7A) ||
                    (0x81 <= dNext && dNext <= 0xFE))
                {
                    pstCheck->dPairCount++;
                    i++;
                    continue;
                }
            }

            pstCheck->dInvalidCount++;
        }
    }

    TCHAR UniRefCheck(LPSTR pcStr)
    {
        CHAR acValue[10];
        PCHAR pcEnd;
        UINT i, code;
        INT radix = 10;
        BOOLEAN bXcode = FALSE;

        ZeroMemory(acValue, sizeof(acValue));
        if (0 == pcStr[2])
            return 0x0000;

        pcStr += 2;
        if ('x' == pcStr[0] || 'X' == pcStr[0])
        {
            bXcode = TRUE;
            pcStr++;
            radix = 16;
        }

        for (i = 0; 10 > i; i++)
        {
            if (';' == pcStr[i])
                break;
            if (0 == pcStr[i])
                return 0x0000;

            if (bXcode)
            {
                if (AsciiIsHexDigit((unsigned char)pcStr[i]))
                    acValue[i] = pcStr[i];
                else
                    return 0x0000;
            }
            else
            {
                if (AsciiIsDigit((unsigned char)pcStr[i]))
                    acValue[i] = pcStr[i];
                else
                    return 0x0000;
            }
        }
        if (10 <= i)
            return 0x0000;

        code = strtoul(acValue, &pcEnd, radix);
        if (0xFFFF < code)
            code = 0x0000;

        return (TCHAR)code;
    }

    LPTSTR MultiByteDecodeAllocInternal(LPCSTR pcBuff, UINT dCodePage)
    {
        DWORD cbWrtSize;
        LPSTR pcPos, pcChk;
        DWORD cchSize, cchWrtSize;
        LPTSTR ptBuffer, ptWrtpo;
        TCHAR chMozi;

        if (!(pcBuff))
            return nullptr;

        cchSize = MultiByteToWideChar(dCodePage, MB_PRECOMPOSED, pcBuff, -1, nullptr, 0);
        if (0 == cchSize)
            return nullptr;

        cchSize += 2;
        ptBuffer = (LPTSTR)malloc(cchSize * sizeof(TCHAR));
        if (!(ptBuffer))
            return nullptr;
        ZeroMemory(ptBuffer, cchSize * sizeof(TCHAR));

        ptWrtpo = ptBuffer;

        pcPos = (LPSTR)pcBuff;
        pcChk = AsciiEntityFind(pcPos);
        while (pcChk)
        {
            cbWrtSize = pcChk - pcPos;
            cchWrtSize = MultiByteToWideChar(dCodePage, MB_PRECOMPOSED, pcPos, cbWrtSize, ptWrtpo, cchSize);

            ptWrtpo += cchWrtSize;
            cchSize -= cchWrtSize;

            chMozi = UniRefCheck(pcChk);
            if (0 != chMozi)
            {
                *ptWrtpo = chMozi;
                ptWrtpo++;
                cchSize--;
                pcChk = strchr(pcChk, ';');
                pcChk++;
            }
            else
            {
                cchWrtSize = MultiByteToWideChar(dCodePage, MB_PRECOMPOSED, pcChk, 2, ptWrtpo, cchSize);

                ptWrtpo += cchWrtSize;
                cchSize -= cchWrtSize;
                pcChk += 2;
            }
            pcPos = pcChk;
            pcChk = AsciiEntityFind(pcPos);
        }

        MultiByteToWideChar(dCodePage, MB_PRECOMPOSED, pcPos, -1, ptWrtpo, cchSize);

        return ptBuffer;
    }

    LPTSTR Utf8DecodeAllocInternal(LPCSTR pcBuff, INT cbSize)
    {
        INT cchSize;
        LPTSTR ptBuffer;

        if (!(pcBuff))
            return nullptr;

        cchSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pcBuff, cbSize, nullptr, 0);
        if (0 >= cchSize)
            return nullptr;

        ptBuffer = (LPTSTR)malloc((cchSize + 1) * sizeof(TCHAR));
        if (!(ptBuffer))
            return nullptr;
        ZeroMemory(ptBuffer, (cchSize + 1) * sizeof(TCHAR));

        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pcBuff, cbSize, ptBuffer, cchSize);
        return ptBuffer;
    }

    UINT TextCodePageEstimate(const BYTE *pdData, INT cbSize, PINT piSkip)
    {
        UINT dUtf8Score = 0;
        MBCCHECK stCp932;
        MBCCHECK stCp949;
        BOOL bUtf8Valid;

        *piSkip = 0;

        if (2 <= cbSize && 0xFF == pdData[0] && 0xFE == pdData[1])
        {
            *piSkip = 2;
            return 1200;
        }
        if (3 <= cbSize && 0xEF == pdData[0] && 0xBB == pdData[1] && 0xBF == pdData[2])
        {
            *piSkip = 3;
            return CP_UTF8;
        }

        bUtf8Valid = Utf8BufferCheck(pdData, cbSize, &dUtf8Score);
        Cp932BufferCheck(pdData, cbSize, &stCp932);
        Cp949BufferCheck(pdData, cbSize, &stCp949);

        if (bUtf8Valid)
        {
            if (0 != stCp932.dInvalidCount && 0 != stCp949.dInvalidCount)
                return CP_UTF8;
            if (0 == stCp932.dPairCount && 0 == stCp932.dKanaCount && 0 == stCp949.dPairCount)
            {
                if (0 < dUtf8Score)
                    return CP_UTF8;
            }
            if (dUtf8Score > max((stCp932.dPairCount * 2) + stCp932.dKanaCount + 1, stCp949.dPairCount + 1))
            {
                return CP_UTF8;
            }
        }

        if (0 == stCp932.dInvalidCount && (0 < stCp932.dPairCount || 0 < stCp932.dKanaCount))
        {
            return 932;
        }

        if (0 == stCp949.dInvalidCount && 0 < stCp949.dPairCount)
        {
            return 949;
        }

        if (bUtf8Valid && 0 < dUtf8Score)
        {
            return CP_UTF8;
        }

        return CP_ACP;
    }
} // namespace

EXTERNED UINT gbSpMoziEnc;

static CONST TCHAR gatSpMoziList[] = {
    TEXT("①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳ⅠⅡⅢⅣⅤⅥⅦⅧⅨⅩⅰⅱⅲⅳⅴⅵⅶⅷⅸⅹ㍉㌔㌢㍍㌘㌧㌃㌶㍑㍗")
    TEXT("㌍㌦㌣㌫㍊㌻㎜㎝㎞㎎㎏㏄㎡㍻〝〟№㏍℡㊤㊥㊦㊧㊨㈱㈲㈹㍾㍽㍼≒≡∫∮∑√⊥∠∟⊿∵∩∪￢￤＇＂丨纊褜")
    TEXT("鍈銈蓜俉炻昱棈鋹曻彅仡仼伀伃伹佖侒侊侚侔俍偀倢俿倞偆偰偂傔僴僘兊兤冝冾凬刕劜劦勀勛匀匇匤卲厓厲叝﨎咜")
    TEXT("咊咩哿喆坙坥垬埈埇﨏塚增墲夋奓奛奝奣妤妺孖寀甯寘寬尞岦岺峵崧嵓﨑嵂嵭嶸嶹巐弡弴彧德忞恝悅悊惞惕愠惲愑")
    TEXT("愷愰憘戓抦揵摠撝擎敎昀昕昻昉昮昞昤晥晗晙晴晳暙暠暲暿曺朎朗杦枻桒柀栁桄棏﨓楨﨔榘槢樰橫橆橳橾櫢櫤毖氿")
    TEXT("汜沆汯泚洄涇浯涖涬淏淸淲淼渹湜渧渼溿澈澵濵瀅瀇瀨炅炫焏焄煜煆煇凞燁燾犱犾猤猪獷玽珉珖珣珒琇珵琦琪琩琮")
    TEXT("瑢璉璟甁畯皂皜皞皛皦益睆劯砡硎硤硺礰礼神祥禔福禛竑竧靖竫箞精絈絜綷綠緖繒罇羡羽茁荢荿菇菶葈蒴蕓蕙蕫﨟")
    TEXT("薰蘒﨡蠇裵訒訷詹誧誾諟諸諶譓譿賰賴贒赶﨣軏﨤逸遧郞都鄕鄧釚釗釞釭釮釤釥鈆鈐鈊鈺鉀鈼鉎鉙鉑鈹鉧銧鉷鉸鋧")
    TEXT("鋗鋙鋐﨧鋕鋠鋓錥錡鋻﨨錞鋿錝錂鍰鍗鎤鏆鏞鏸鐱鑅鑈閒隆﨩隝隯霳霻靃靍靏靑靕顗顥飯飼餧館馞驎髙髜魵魲鮏鮱")
    TEXT("鮻鰀鵰鵫鶴鸙黑")};

#define SPMOZI_CNT 457

UINT IsSpMozi(TCHAR tMozi)
{
    UINT i;

    if (!(gbSpMoziEnc))
        return 0;

    for (i = 0; SPMOZI_CNT > i; i++)
    {
        if (gatSpMoziList[i] == tMozi)
            return 1;
    }

    return 0;
}

LPTSTR TextDecodeAutoAlloc(LPCVOID pBuffer, INT cbSize, PUINT pdCodePage)
{
    const BYTE *pdData = (const BYTE *)pBuffer;
    UINT dCodePage;
    INT iSkip;
    INT cbBody;
    LPTSTR ptBuffer;

    if (!(pBuffer) || 0 >= cbSize)
        return nullptr;

    dCodePage = TextCodePageEstimate(pdData, cbSize, &iSkip);
    if (pdCodePage)
        *pdCodePage = dCodePage;

    if (1200 == dCodePage)
    {
        cbBody = cbSize - iSkip;
        ptBuffer = (LPTSTR)malloc(cbBody + (2 * sizeof(TCHAR)));
        if (!(ptBuffer))
            return nullptr;
        ZeroMemory(ptBuffer, cbBody + (2 * sizeof(TCHAR)));
        CopyMemory(ptBuffer, pdData + iSkip, cbBody);
        return ptBuffer;
    }

    cbBody = cbSize - iSkip;
    if (CP_UTF8 == dCodePage)
    {
        return Utf8DecodeAllocInternal((LPCSTR)(pdData + iSkip), cbBody);
    }

    return MultiByteDecodeAllocInternal((LPCSTR)(pdData + iSkip), dCodePage);
}

LPTSTR LegacyEncodedTextDecodeAlloc(LPSTR pcBuff)
{
    return MultiByteDecodeAllocInternal(pcBuff, CP_ACP);
}

LPSTR LegacyEncodedTextEncodeAlloc(LPCTSTR ptTexts)
{
    TCHAR atMozi[2];
    CHAR acNoSjisText[10];
    BOOL bCant = FALSE;
    INT iRslt;
    UINT_PTR cchSize, d, cbSize;
    LPSTR pcString;
    string sString;

    if (!(ptTexts))
        return nullptr;

    StringCchLength(ptTexts, STRSAFE_MAX_CCH, &cchSize);
    sString.clear();

    atMozi[1] = 0;
    for (d = 0; cchSize > d; d++)
    {
        atMozi[0] = ptTexts[d];
        ZeroMemory(acNoSjisText, sizeof(acNoSjisText));

        iRslt = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, atMozi, 1, acNoSjisText, 10, "?", &bCant);
        (void)iRslt;
        if (bCant)
        {
            StringCchPrintfA(acNoSjisText, 10, ("&#%d;"), ptTexts[d]);
        }

        if (IsSpMozi(ptTexts[d]))
        {
            StringCchPrintfA(acNoSjisText, 10, ("&#%d;"), ptTexts[d]);
        }

        sString += string(acNoSjisText);
    }

    cbSize = sString.size() + 2;
    pcString = (LPSTR)malloc(cbSize);
    ZeroMemory(pcString, cbSize);
    StringCchCopyA(pcString, cbSize, sString.c_str());

    return pcString;
}
