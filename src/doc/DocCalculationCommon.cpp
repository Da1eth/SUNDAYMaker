#include "Sunday.h"

DOCLINEMETRICS DocLineMetricsCalculate(const ONELINE &stLine)
{
    DOCLINEMETRICS stMetrics{};
    stMetrics.iLetterCnt = static_cast<INT>(stLine.vcLine.size());

    for (const auto &stLetter : stLine.vcLine)
    {
        stMetrics.dDotCnt += stLetter.rdWidth;
        stMetrics.dByteCnt += static_cast<INT>(stLetter.mzByte);
    }

    return stMetrics;
}

INT DocLetterPosAdjustInLine(const ONELINE &stLine, PINT pNowDot, INT round)
{
    INT iLetter = 0;
    INT dDotCnt = 0;
    INT dPrvCnt = 0;

    assert(pNowDot);

    for (INT i = 0, iCount = static_cast<INT>(stLine.vcLine.size()); iCount > i;
         i++, iLetter++)
    {
        if (dDotCnt >= *pNowDot)
        {
            break;
        }

        dPrvCnt = dDotCnt;
        dDotCnt += stLine.vcLine.at(i).rdWidth;
    }

    if (dDotCnt != *pNowDot)
    {
        if (0 < round)
        {
            *pNowDot = dDotCnt;
        }
        else if (0 > round)
        {
            *pNowDot = dPrvCnt;
            iLetter--;
        }
        else if ((*pNowDot - dPrvCnt) < (dDotCnt - *pNowDot))
        {
            *pNowDot = dPrvCnt;
            iLetter--;
        }
        else
        {
            *pNowDot = dDotCnt;
        }
    }

    return iLetter;
}

DOCSELRANGE DocSelectionRangeNormalize(INT dTop, INT dBottom)
{
    DOCSELRANGE stRange{dTop, dBottom};

    if (0 <= stRange.dTop && 0 <= stRange.dBottom &&
        stRange.dTop > stRange.dBottom)
    {
        std::swap(stRange.dTop, stRange.dBottom);
    }

    return stRange;
}

DOCSELRANGE DocSelectionRangeCalculate(const ONEPAGE &stPage)
{
    DOCSELRANGE stRange{};
    INT iLine = 0;

    for (const auto &stLine : stPage.ltPage)
    {
        const auto itSelected = std::find_if(
            stLine.vcLine.begin(), stLine.vcLine.end(),
            [](const LETTER &stLetter)
            {
                return 0 != (CT_SELECT & stLetter.mzStyle);
            });

        if (itSelected != stLine.vcLine.end())
        {
            if (0 > stRange.dTop)
            {
                stRange.dTop = iLine;
            }
            stRange.dBottom = iLine;
        }

        iLine++;
    }

    return stRange;
}
