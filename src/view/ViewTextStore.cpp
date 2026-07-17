#include "ViewTextStore.h"

#include "EditorController.h"
#include "ViewCentralInternal.h"

#include <deque>
#include <set>

namespace
{
constexpr TsViewCookie kViewCookie = 1;
constexpr HRESULT kConnectNoConnection = static_cast<HRESULT>(0x80040200L);
constexpr HRESULT kConnectAdviseLimit = static_cast<HRESULT>(0x80040201L);

class SundayTextStore final : public ITextStoreACP,
                              public ITfContextOwnerCompositionSink,
                              public ITfTextEditSink
{
public:
    explicit SundayTextStore(HWND hWnd) : hWnd_(hWnd) {}

    HRESULT Initialise()
    {
        const HRESULT coHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(coHr))
            uninitialiseCom_ = true;
        else if (coHr != RPC_E_CHANGED_MODE)
            return coHr;

        HRESULT hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&threadMgr_));
        if (FAILED(hr))
            return hr;
        hr = threadMgr_->Activate(&clientId_);
        if (FAILED(hr))
            return hr;
        hr = threadMgr_->CreateDocumentMgr(&documentMgr_);
        if (FAILED(hr))
            return hr;
        hr = documentMgr_->CreateContext(clientId_, 0,
                                         static_cast<ITextStoreACP *>(this),
                                         &context_, &editCookie_);
        if (FAILED(hr))
            return hr;
        hr = documentMgr_->Push(context_);
        if (FAILED(hr))
            return hr;
        ITfSource *source = nullptr;
        if (SUCCEEDED(context_->QueryInterface(IID_PPV_ARGS(&source))))
        {
            source->AdviseSink(IID_ITfTextEditSink,
                static_cast<ITfTextEditSink *>(this), &textEditSinkCookie_);
            source->Release();
        }
        RefreshSnapshot();
        return S_OK;
    }

    void Shutdown()
    {
        FlushPendingActions();
        if (undoActiveGroup_)
        {
            DocUndoGroupEnd();
            DocUndoGroupFlush();
            undoActiveGroup_ = 0;
        }
        composing_ = false;
        if (threadMgr_)
            threadMgr_->SetFocus(nullptr);
        if (documentMgr_)
            documentMgr_->Pop(TF_POPF_ALL);
        if (context_ && textEditSinkCookie_ != TF_INVALID_COOKIE)
        {
            ITfSource *source = nullptr;
            if (SUCCEEDED(context_->QueryInterface(IID_PPV_ARGS(&source))))
            {
                source->UnadviseSink(textEditSinkCookie_);
                source->Release();
            }
            textEditSinkCookie_ = TF_INVALID_COOKIE;
        }
        if (context_)
            context_->Release();
        if (documentMgr_)
            documentMgr_->Release();
        if (threadMgr_)
        {
            threadMgr_->Deactivate();
            threadMgr_->Release();
        }
        if (sink_)
            sink_->Release();
        context_ = nullptr;
        documentMgr_ = nullptr;
        threadMgr_ = nullptr;
        sink_ = nullptr;
        if (uninitialiseCom_)
        {
            CoUninitialize();
            uninitialiseCom_ = false;
        }
    }

    void SetFocus(BOOLEAN focused)
    {
        if (!threadMgr_)
            return;
        if (focused)
        {
            RefreshSnapshot();
            threadMgr_->SetFocus(documentMgr_);
        }
        else
            threadMgr_->SetFocus(nullptr);
    }

    void NotifyExternalChange()
    {
        const LONG oldLength = static_cast<LONG>(text_.size());
        const LONG oldSelectionStart = selectionStart_;
        const LONG oldSelectionEnd = selectionEnd_;
        RefreshSnapshot();

        if (!sink_ || lock_)
            return;
        if (oldLength != static_cast<LONG>(text_.size()) &&
            (sinkMask_ & TS_AS_TEXT_CHANGE))
        {
            TS_TEXTCHANGE change{};
            change.acpStart = 0;
            change.acpOldEnd = oldLength;
            change.acpNewEnd = static_cast<LONG>(text_.size());
            sink_->OnTextChange(0, &change);
        }
        if ((oldSelectionStart != selectionStart_ ||
             oldSelectionEnd != selectionEnd_) &&
            (sinkMask_ & TS_AS_SEL_CHANGE))
        {
            sink_->OnSelectionChange();
        }
        if (sinkMask_ & TS_AS_LAYOUT_CHANGE)
            sink_->OnLayoutChange(TS_LC_CHANGE, kViewCookie);
    }

    bool IsComposing() const { return composing_; }

    STDMETHODIMP QueryInterface(REFIID iid, void **result) override
    {
        if (!result)
            return E_INVALIDARG;
        *result = nullptr;
        if (iid == IID_IUnknown || iid == IID_ITextStoreACP)
            *result = static_cast<ITextStoreACP *>(this);
        else if (iid == IID_ITfContextOwnerCompositionSink)
            *result = static_cast<ITfContextOwnerCompositionSink *>(this);
        else if (iid == IID_ITfTextEditSink)
            *result = static_cast<ITfTextEditSink *>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release() override
    {
        const ULONG refs = InterlockedDecrement(&refs_);
        if (!refs)
            delete this;
        return refs;
    }

    STDMETHODIMP AdviseSink(REFIID iid, IUnknown *unknown, DWORD mask) override
    {
        if (iid != IID_ITextStoreACPSink || !unknown)
            return E_INVALIDARG;
        ITextStoreACPSink *candidate = nullptr;
        const HRESULT hr = unknown->QueryInterface(IID_PPV_ARGS(&candidate));
        if (FAILED(hr))
            return hr;
        if (sink_ && candidate != sink_)
        {
            candidate->Release();
            return kConnectAdviseLimit;
        }
        if (sink_)
            sink_->Release();
        sink_ = candidate;
        sinkMask_ = mask;
        return S_OK;
    }

    STDMETHODIMP UnadviseSink(IUnknown *unknown) override
    {
        if (!sink_ || !unknown)
            return kConnectNoConnection;
        ITextStoreACPSink *candidate = nullptr;
        unknown->QueryInterface(IID_PPV_ARGS(&candidate));
        const bool same = candidate == sink_;
        if (candidate)
            candidate->Release();
        if (!same)
            return kConnectNoConnection;
        sink_->Release();
        sink_ = nullptr;
        sinkMask_ = 0;
        return S_OK;
    }

    STDMETHODIMP RequestLock(DWORD flags, HRESULT *sessionResult) override
    {
        if (!sessionResult || !sink_)
            return E_INVALIDARG;
        if (lock_)
        {
            if (flags & TS_LF_SYNC)
                return TS_E_SYNCHRONOUS;
            pendingLock_ = flags;
            *sessionResult = TS_S_ASYNC;
            return S_OK;
        }
        if (!composing_)
            RefreshSnapshot();
        GrantLock(flags, sessionResult);
        while (pendingLock_)
        {
            const DWORD pending = pendingLock_;
            pendingLock_ = 0;
            HRESULT ignored = S_OK;
            GrantLock(pending, &ignored);
        }
        return S_OK;
    }

    STDMETHODIMP GetStatus(TS_STATUS *status) override
    {
        if (!status)
            return E_INVALIDARG;
        status->dwDynamicFlags = 0;
        status->dwStaticFlags = TS_SS_NOHIDDENTEXT;
        return S_OK;
    }

    STDMETHODIMP QueryInsert(LONG start, LONG end, ULONG count,
                             LONG *resultStart, LONG *resultEnd) override
    {
        if (!resultStart || !resultEnd || !ValidRange(start, end))
            return E_INVALIDARG;
        *resultStart = start;
        *resultEnd = start + static_cast<LONG>(count);
        return S_OK;
    }

    STDMETHODIMP GetSelection(ULONG index, ULONG count,
                              TS_SELECTION_ACP *selection,
                              ULONG *fetched) override
    {
        if (!HasReadLock())
            return TS_E_NOLOCK;
        if (!selection || !fetched || !count ||
            (index != TS_DEFAULT_SELECTION && index != 0))
            return E_INVALIDARG;
        selection[0].acpStart = selectionStart_;
        selection[0].acpEnd = selectionEnd_;
        selection[0].style.ase = TS_AE_END;
        selection[0].style.fInterimChar = FALSE;
        *fetched = 1;
        return S_OK;
    }

    STDMETHODIMP SetSelection(ULONG count,
                              const TS_SELECTION_ACP *selection) override
    {
        if (!HasWriteLock())
            return TS_E_NOLOCK;
        if (count != 1 || !selection ||
            !ValidRange(selection->acpStart, selection->acpEnd))
            return E_INVALIDARG;
        selectionStart_ = selection->acpStart;
        selectionEnd_ = selection->acpEnd;
        return S_OK;
    }

    STDMETHODIMP GetText(LONG start, LONG end, WCHAR *plain, ULONG plainSize,
                         ULONG *plainCount, TS_RUNINFO *runs, ULONG runSize,
                         ULONG *runCount, LONG *next) override
    {
        if (!HasReadLock())
            return TS_E_NOLOCK;
        if (!plainCount || !runCount || !next || start < 0 ||
            start > static_cast<LONG>(text_.size()))
            return E_INVALIDARG;
        if (end == -1)
            end = static_cast<LONG>(text_.size());
        if (!ValidRange(start, end))
            return TS_E_INVALIDPOS;
        const ULONG available = static_cast<ULONG>(end - start);
        const ULONG copied = min(available, plainSize);
        if (copied && plain)
            CopyMemory(plain, text_.data() + start, copied * sizeof(WCHAR));
        *plainCount = copied;
        *next = start + copied;
        *runCount = 0;
        if (runSize && runs && copied)
        {
            runs[0].uCount = copied;
            runs[0].type = TS_RT_PLAIN;
            *runCount = 1;
        }
        return S_OK;
    }

    STDMETHODIMP SetText(DWORD, LONG start, LONG end, const WCHAR *text,
                         ULONG count, TS_TEXTCHANGE *change) override
    {
        if (!HasWriteLock())
            return TS_E_NOLOCK;
        if (!change || (count && !text) || !ValidRange(start, end))
            return E_INVALIDARG;
        return ReplaceText(start, end, text, count, change);
    }

    STDMETHODIMP InsertTextAtSelection(DWORD flags, const WCHAR *text,
                                       ULONG count, LONG *start, LONG *end,
                                       TS_TEXTCHANGE *change) override
    {
        if (!HasWriteLock())
            return TS_E_NOLOCK;
        if ((count && !text) || !change)
            return E_INVALIDARG;
        const LONG oldStart = selectionStart_;
        const LONG oldEnd = selectionEnd_;
        if (flags & TS_IAS_QUERYONLY)
        {
            if (start) *start = oldStart;
            if (end) *end = oldStart + static_cast<LONG>(count);
            change->acpStart = oldStart;
            change->acpOldEnd = oldEnd;
            change->acpNewEnd = oldStart + static_cast<LONG>(count);
            return S_OK;
        }
        const HRESULT hr = ReplaceText(oldStart, oldEnd, text, count, change);
        if (SUCCEEDED(hr))
        {
            if (start) *start = oldStart;
            if (end) *end = change->acpNewEnd;
        }
        return hr;
    }

    STDMETHODIMP GetEndACP(LONG *end) override
    {
        if (!HasReadLock())
            return TS_E_NOLOCK;
        if (!end)
            return E_INVALIDARG;
        *end = static_cast<LONG>(text_.size());
        return S_OK;
    }

    STDMETHODIMP GetActiveView(TsViewCookie *view) override
    {
        if (!view)
            return E_INVALIDARG;
        *view = kViewCookie;
        return S_OK;
    }

    STDMETHODIMP GetWnd(TsViewCookie view, HWND *hWnd) override
    {
        if (!hWnd || view != kViewCookie)
            return E_INVALIDARG;
        *hWnd = hWnd_;
        return S_OK;
    }

    STDMETHODIMP GetScreenExt(TsViewCookie view, RECT *rect) override
    {
        if (!rect || view != kViewCookie)
            return E_INVALIDARG;
        GetClientRect(hWnd_, rect);
        MapWindowPoints(hWnd_, nullptr, reinterpret_cast<POINT *>(rect), 2);
        return S_OK;
    }

    STDMETHODIMP GetTextExt(TsViewCookie view, LONG start, LONG end,
                            RECT *rect, BOOL *clipped) override
    {
        if (!HasReadLock())
            return TS_E_NOLOCK;
        if (!rect || !clipped || view != kViewCookie || !ValidRange(start, end))
            return E_INVALIDARG;
        POINT point{};
        GetCaretPos(&point);
        ClientToScreen(hWnd_, &point);
        SetRect(rect, point.x, point.y, point.x + 2, point.y + LINE_HEIGHT);
        *clipped = FALSE;
        return S_OK;
    }

    STDMETHODIMP GetACPFromPoint(TsViewCookie, const POINT *, DWORD,
                                 LONG *) override { return E_NOTIMPL; }
    STDMETHODIMP GetFormattedText(LONG, LONG, IDataObject **) override
        { return E_NOTIMPL; }
    STDMETHODIMP GetEmbedded(LONG, REFGUID, REFIID, IUnknown **) override
        { return TS_E_NOOBJECT; }
    STDMETHODIMP QueryInsertEmbedded(const GUID *, const FORMATETC *,
                                     BOOL *insertable) override
        { if (insertable) *insertable = FALSE; return S_OK; }
    STDMETHODIMP InsertEmbedded(DWORD, LONG, LONG, IDataObject *,
                                TS_TEXTCHANGE *) override
        { return TS_E_FORMAT; }
    STDMETHODIMP InsertEmbeddedAtSelection(DWORD, IDataObject *, LONG *,
                                           LONG *, TS_TEXTCHANGE *) override
        { return TS_E_FORMAT; }
    STDMETHODIMP RequestSupportedAttrs(DWORD, ULONG,
                                       const TS_ATTRID *) override { return S_OK; }
    STDMETHODIMP RequestAttrsAtPosition(LONG, ULONG, const TS_ATTRID *,
                                        DWORD) override { return S_OK; }
    STDMETHODIMP RequestAttrsTransitioningAtPosition(
        LONG, ULONG, const TS_ATTRID *, DWORD) override { return S_OK; }
    STDMETHODIMP FindNextAttrTransition(LONG start, LONG, ULONG,
        const TS_ATTRID *, DWORD, LONG *next, BOOL *found,
        LONG *offset) override
    {
        if (!next || !found || !offset) return E_INVALIDARG;
        *next = start; *found = FALSE; *offset = 0; return S_OK;
    }
    STDMETHODIMP RetrieveRequestedAttrs(ULONG, TS_ATTRVAL *,
                                        ULONG *fetched) override
        { if (!fetched) return E_INVALIDARG; *fetched = 0; return S_OK; }

    STDMETHODIMP OnStartComposition(ITfCompositionView *, BOOL *ok) override
    {
        if (!ok)
            return E_INVALIDARG;
        composing_ = true;
        activeCompositionGroup_ = ++nextCompositionGroup_;
        compositionStart_ = selectionStart_;
        compositionLength_ = selectionEnd_ - selectionStart_;
        *ok = TRUE;
        return S_OK;
    }
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *,
                                     ITfRange *) override { return S_OK; }
    STDMETHODIMP OnEndComposition(ITfCompositionView *) override
    {
        if (activeCompositionGroup_)
            endedCompositionGroups_.insert(activeCompositionGroup_);
        activeCompositionGroup_ = 0;
        composing_ = false;
        compositionLength_ = 0;
        PostMessage(hWnd_, WM_APP + 22, 0, 0);
        return S_OK;
    }

    STDMETHODIMP OnEndEdit(ITfContext *, TfEditCookie,
                           ITfEditRecord *) override
    {
        return S_OK;
    }

    void FlushPendingActions()
    {
        while (!pendingActions_.empty())
        {
            PENDING_ACTION action = std::move(pendingActions_.front());
            pendingActions_.pop_front();

            if (undoActiveGroup_ != action.dCompositionGroup)
            {
                if (undoActiveGroup_)
                {
                    DocUndoGroupEnd();
                    DocUndoGroupFlush();
                    endedCompositionGroups_.erase(undoActiveGroup_);
                }
                DocUndoGroupBegin();
                undoActiveGroup_ = action.dCompositionGroup;
            }

            ApplyReplacement(action.acpStart, action.acpEnd,
                             action.wsText.c_str(),
                             static_cast<ULONG>(action.wsText.size()));
        }

        if (undoActiveGroup_ &&
            endedCompositionGroups_.contains(undoActiveGroup_))
        {
            DocUndoGroupEnd();
            DocUndoGroupFlush();
            endedCompositionGroups_.erase(undoActiveGroup_);
            undoActiveGroup_ = 0;
        }
    }

private:
    ~SundayTextStore() = default;

    void GrantLock(DWORD flags, HRESULT *result)
    {
        lock_ = flags;
        *result = sink_->OnLockGranted(flags);
        lock_ = 0;
    }

    bool HasReadLock() const
        { return (lock_ & TS_LF_READ) || (lock_ & TS_LF_READWRITE); }
    bool HasWriteLock() const { return (lock_ & TS_LF_READWRITE) != 0; }
    bool ValidRange(LONG start, LONG end) const
        { return start >= 0 && end >= start &&
                 end <= static_cast<LONG>(text_.size()); }

    void RefreshSnapshot()
    {
        LPTSTR page = nullptr;
        if (0 <= DocPageGetAlloc(D_UNI, reinterpret_cast<LPVOID *>(&page)) &&
            page)
        {
            text_ = page;
            FREE(page);
        }
        else
            text_.clear();

        auto caret = ViewCurrentCaret();
        LONG acp = 0;
        for (INT line = 0; line < caret.dLine; ++line)
        {
            INT letters = 0;
            DocLineParamGet(line, &letters, nullptr);
            acp += letters + 2; // DocPageGetAlloc uses CRLF between lines.
        }
        selectionStart_ = selectionEnd_ =
            min(acp + caret.dMozi, static_cast<LONG>(text_.size()));
    }

    void PositionCaretAtACP(LONG target)
    {
        LONG base = 0;
        const INT lineCount = static_cast<INT>(DocNowFilePageLineCount());
        for (INT line = 0; line < lineCount; ++line)
        {
            INT letters = 0;
            DocLineParamGet(line, &letters, nullptr);
            if (target <= base + letters || line + 1 == lineCount)
            {
                const INT wanted = max(0, min(letters,
                    static_cast<INT>(target - base)));
                LPLETTER data = nullptr;
                INT fetched = 0;
                UINT flags = 0;
                INT dot = 0;
                DocLineDataGetAlloc(line, 0, &data, &fetched, &flags);
                for (INT i = 0; data && i < wanted && i < fetched; ++i)
                    dot += data[i].rdWidth;
                FREE(data);
                ViewPosResetCaret(dot, line);
                return;
            }
            base += letters + 2;
        }
    }

    HRESULT ReplaceText(LONG start, LONG end, const WCHAR *replacement,
                        ULONG count, TS_TEXTCHANGE *change)
    {
        const LONG removeCount = end - start;
        const WCHAR *safeReplacement = replacement ? replacement : L"";
        ULONG group = activeCompositionGroup_;
        if (!group)
        {
            group = ++nextCompositionGroup_;
            endedCompositionGroups_.insert(group);
        }
        pendingActions_.push_back(
            {start, end, wstring(safeReplacement, count), group});
        PostMessage(hWnd_, WM_APP + 22, 0, 0);

        text_.replace(static_cast<size_t>(start),
                      static_cast<size_t>(removeCount), safeReplacement, count);
        selectionStart_ = selectionEnd_ = start + static_cast<LONG>(count);
        compositionStart_ = start;
        compositionLength_ = static_cast<LONG>(count);
        change->acpStart = start;
        change->acpOldEnd = end;
        change->acpNewEnd = selectionEnd_;
        return S_OK;
    }

    void ApplyReplacement(LONG start, LONG end, const WCHAR *replacement,
                          ULONG count)
    {
        const LONG removeCount = end - start;
        if (IsSelecting(nullptr))
            ViewSelPageAll(-1);
        PositionCaretAtACP(end);
        for (LONG i = 0; i < removeCount; ++i)
            ViewEditDeleteBackward();
        for (ULONG i = 0; i < count; ++i)
        {
            if (replacement[i] == L'\r')
            {
                if (i + 1 < count && replacement[i + 1] == L'\n')
                    ++i;
                ViewEditInsertLineBreak(FALSE);
            }
            else if (replacement[i] != L'\n')
                ViewEditInputCharacter(replacement[i]);
        }
    }

    struct PENDING_ACTION
    {
        LONG acpStart;
        LONG acpEnd;
        wstring wsText;
        ULONG dCompositionGroup;
    };

    LONG refs_ = 1;
    HWND hWnd_{};
    ITfThreadMgr *threadMgr_{};
    ITfDocumentMgr *documentMgr_{};
    ITfContext *context_{};
    ITextStoreACPSink *sink_{};
    TfClientId clientId_{};
    TfEditCookie editCookie_{};
    DWORD textEditSinkCookie_{TF_INVALID_COOKIE};
    DWORD sinkMask_{};
    DWORD lock_{};
    DWORD pendingLock_{};
    wstring text_;
    LONG selectionStart_{};
    LONG selectionEnd_{};
    LONG compositionStart_{};
    LONG compositionLength_{};
    bool composing_{};
    bool uninitialiseCom_{};
    deque<PENDING_ACTION> pendingActions_;
    set<ULONG> endedCompositionGroups_;
    ULONG nextCompositionGroup_{};
    ULONG activeCompositionGroup_{};
    ULONG undoActiveGroup_{};
};

SundayTextStore *gTextStore;
}

HRESULT ViewTextStoreInitialise(HWND hWnd)
{
    if (gTextStore)
        return S_FALSE;
    gTextStore = new (nothrow) SundayTextStore(hWnd);
    if (!gTextStore)
        return E_OUTOFMEMORY;
    const HRESULT hr = gTextStore->Initialise();
    if (FAILED(hr))
    {
        gTextStore->Shutdown();
        gTextStore->Release();
        gTextStore = nullptr;
    }
    return hr;
}

VOID ViewTextStoreDestroy()
{
    if (!gTextStore)
        return;
    gTextStore->Shutdown();
    gTextStore->Release();
    gTextStore = nullptr;
}

VOID ViewTextStoreSetFocus(BOOLEAN focused)
{
    if (gTextStore)
        gTextStore->SetFocus(focused);
}

BOOLEAN ViewTextStoreIsComposing()
{
    return gTextStore && gTextStore->IsComposing();
}

VOID ViewTextStoreFlushPendingActions()
{
    if (gTextStore)
        gTextStore->FlushPendingActions();
}

VOID ViewTextStoreNotifyExternalChange()
{
    if (gTextStore)
        gTextStore->NotifyExternalChange();
}
