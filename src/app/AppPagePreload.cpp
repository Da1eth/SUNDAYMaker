#include "AppModuleInternal.h"
#include "DocContextInternal.h"

extern list<ONEFILE> gltMultiFiles;

static LPARAM gdPagePreloadFile;
static INT giPagePreloadNext = -1;
static BOOLEAN gbPagePreloadActive;

static VOID PagePreloadStopInternal(HWND hWnd)
{
    if (!hWnd)
    {
        hWnd = AppMainWindowGet();
    }

    if (hWnd)
    {
        KillTimer(hWnd, IDT_PAGE_PRELOAD_TIMER);
    }

    gdPagePreloadFile = 0;
    giPagePreloadNext = -1;
    gbPagePreloadActive = FALSE;
}

static INT PagePreloadFindNext(VOID)
{
    UINT_PTR iPageCount;
    UINT_PTR i;
    UINT_PTR iStart;

    iPageCount = DocCurrentFile().vcCont.size();
    if (0 == iPageCount)
        return -1;

    iStart =
        (0 <= giPagePreloadNext) ? static_cast<UINT_PTR>(giPagePreloadNext) : 0;

    for (i = 0; iPageCount > i; i++)
    {
        const UINT_PTR iPage = (iStart + i) % iPageCount;

        if (DocCurrentFile().vcCont.at(iPage).ptRawData)
        {
            return static_cast<INT>(iPage);
        }
    }

    return -1;
}

VOID AppBackgroundPageLoadTick(HWND hWnd)
{
    INT iPage;

    if (!(gbPagePreloadActive) || gltMultiFiles.empty())
    {
        PagePreloadStopInternal(hWnd);
        return;
    }

    if (gdPagePreloadFile != DocCurrentFile().dUnique)
    {
        PagePreloadStopInternal(hWnd);
        return;
    }

    iPage = PagePreloadFindNext();
    if (0 > iPage)
    {
        PagePreloadStopInternal(hWnd);
        return;
    }

    DocDelayPageLoad(DocCurrentFileIterator(), iPage);
    giPagePreloadNext = iPage + 1;
}

HRESULT AppBackgroundPageLoadRestart(HWND hWnd)
{
    PagePreloadStopInternal(hWnd);

    if (!hWnd)
    {
        hWnd = AppMainWindowGet();
    }

    if (!hWnd || gltMultiFiles.empty())
        return S_FALSE;
    if (DocCurrentFile().vcCont.empty())
        return S_FALSE;

    gdPagePreloadFile = DocCurrentFile().dUnique;
    giPagePreloadNext = DocCurrentFile().dNowPage + 1;
    gbPagePreloadActive = TRUE;

    if (0 == SetTimer(hWnd, IDT_PAGE_PRELOAD_TIMER, 15, nullptr))
    {
        PagePreloadStopInternal(hWnd);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT AppBackgroundPageLoadStop(HWND hWnd)
{
    PagePreloadStopInternal(hWnd);
    return S_OK;
}