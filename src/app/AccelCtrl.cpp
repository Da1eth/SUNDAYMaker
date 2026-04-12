#include "Sunday.h"

#ifdef ACCELERATOR_EDIT

static BOOLEAN AccelKeyCommandAllowed(UINT dCommandID);
static ACCEL *AccelKeyFindByCommand(vector<ACCEL> *, WORD);
static const ACCEL *AccelKeyFindByCommand(const vector<ACCEL> &, WORD);

static INT_PTR CALLBACK AccelKeyDlgProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR AccelKeyNotify(HWND, INT, LPNMHDR, vector<ACCEL> *);

static HRESULT AccelKeyBindExistCheck(HWND, LPACCEL, vector<ACCEL> *);
static HRESULT AccelKeyBindListMod(HWND, INT, LPACCEL, vector<ACCEL> *);
static HRESULT AccelKeySettingReset(HWND, vector<ACCEL> *);
static HRESULT AccelKeyBindString(LPACCEL, LPTSTR, UINT_PTR);
static VOID AccelKeyListInit(HWND, vector<ACCEL> *);
static BYTE AccelHotModExchange(BYTE, BOOLEAN);
static HRESULT AccelKeyTableSave(vector<ACCEL> *);

static BOOLEAN AccelKeyCommandAllowed(UINT dCommandID)
{
    return AppCommandAllowedInAccel(dCommandID);
}

static ACCEL *AccelKeyFindByCommand(vector<ACCEL> *pAccel, WORD dCommand)
{
    auto itFound = find_if(pAccel->begin(), pAccel->end(),
                           [dCommand](const ACCEL &stAccel)
                           { return stAccel.cmd == dCommand; });

    return (pAccel->end() == itFound) ? nullptr : &(*itFound);
}

static const ACCEL *AccelKeyFindByCommand(const vector<ACCEL> &vcAccel,
                                          WORD dCommand)
{
    auto itFound = find_if(vcAccel.begin(), vcAccel.end(),
                           [dCommand](const ACCEL &stAccel)
                           { return stAccel.cmd == dCommand; });

    return (vcAccel.end() == itFound) ? nullptr : &(*itFound);
}

HRESULT AccelKeyDlgOpen(HWND hWnd)
{
    INT_PTR iRslt;
    LPACCEL pstAccel;
    INT iEntry;

    iRslt = DialogBoxParam(CntxInstanceGet(), MAKEINTRESOURCE(IDD_ACCEL_KEY_DLG),
                           hWnd, AccelKeyDlgProc, 0);
    if (IDOK == iRslt)
    {
        pstAccel = AccelKeyTableLoadAlloc(&iEntry);
        AccelKeyTableCreate(pstAccel, iEntry);

        ToolBarInfoChange(pstAccel, iEntry);
        PageToolBarInfoChange(pstAccel, iEntry);

        FREE(pstAccel);
    }

    return S_OK;
}

static INT_PTR CALLBACK AccelKeyDlgProc(HWND hDlg, UINT message, WPARAM wParam,
                                        LPARAM lParam)
{
    static vector<ACCEL> cvcAccel;

    INT iAccEntry;
    LPACCEL pstAccel;

    static HWND hHokyWnd;
    HWND hLvWnd;
    LRESULT lRslt;
    ACCEL stAcce;

    INT i;
    INT iItem;

    INT id;
    HWND hWndCtl;
    UINT codeNotify;

    switch (message)
    {
    case WM_INITDIALOG:
        hLvWnd = GetDlgItem(hDlg, IDLV_FUNCKEY_LIST);
        ListView_SetExtendedListViewStyle(
            hLvWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

        cvcAccel.clear();
        pstAccel = AccelKeyTableGetAlloc(&iAccEntry);
        for (i = 0; iAccEntry > i; i++)
        {
            cvcAccel.push_back(pstAccel[i]);
        }
        FREE(pstAccel);

        AccelKeyListInit(hDlg, &cvcAccel);

        hHokyWnd = GetDlgItem(hDlg, IDHKC_FUNCKEY_INPUT);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        id = LOWORD(wParam);
        hWndCtl = (HWND)lParam;
        codeNotify = HIWORD(wParam);
        UNREFERENCED_PARAMETER(hWndCtl);
        UNREFERENCED_PARAMETER(codeNotify);

        switch (id)
        {
        case IDOK:
            AccelKeyTableSave(&cvcAccel);
        case IDCANCEL:
            cvcAccel.clear();
            EndDialog(hDlg, id);
            return (INT_PTR)TRUE;

        case IDB_FUNCKEY_CLEAR:
            SendMessage(hHokyWnd, HKM_SETHOTKEY, 0, 0);
            iItem = WndTagGet(hHokyWnd);
            AccelKeyBindListMod(hDlg, iItem, nullptr, &cvcAccel);
            return (INT_PTR)TRUE;

        case IDB_FUNCKEY_SET:
            lRslt = SendMessage(hHokyWnd, HKM_GETHOTKEY, 0, 0);
            stAcce.key = LOBYTE(lRslt);
            if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDCB_FUNCKEY_SPACE))
            {
                stAcce.key = VK_SPACE;
            }
            stAcce.fVirt = AccelHotModExchange(HIBYTE(lRslt), 0);
            stAcce.cmd = 0;
            if (SUCCEEDED(AccelKeyBindExistCheck(hDlg, &stAcce, &cvcAccel)))
            {
                iItem = WndTagGet(hHokyWnd);
                AccelKeyBindListMod(hDlg, iItem, &stAcce, &cvcAccel);
            }
            return (INT_PTR)TRUE;

        case IDB_FUNCKEY_INIT:
            if (IDOK ==
                MessageBox(hDlg, TEXT("단축키 설정을 초기값으로 되돌립니다."),
                           TEXT("설정 리셋"), MB_OKCANCEL | MB_ICONQUESTION))
            {
                AccelKeySettingReset(hDlg, &cvcAccel);
                AccelKeyListInit(hDlg, &cvcAccel);
            }
            return (INT_PTR)TRUE;

        default:
            break;
        }
        break;

    case WM_NOTIFY:
        return AccelKeyNotify(hDlg, (INT)(wParam), (LPNMHDR)(lParam), &cvcAccel);

    default:
        break;
    }

    return (INT_PTR)FALSE;
}

static INT_PTR AccelKeyNotify(HWND hDlg, INT idFrom, LPNMHDR pstNmhdr,
                              vector<ACCEL> *pvcAccel)
{
    LPNMLISTVIEW pstLv;
    LVITEM stLvi;
    BYTE bMod;
    HWND hHokyWnd;

    if (IDLV_FUNCKEY_LIST == idFrom)
    {
        if (NM_CLICK == pstNmhdr->code)
        {
            pstLv = (LPNMLISTVIEW)pstNmhdr;

            stLvi = {};
            stLvi.mask = LVIF_PARAM;
            stLvi.iItem = pstLv->iItem;
            ListView_GetItem(pstNmhdr->hwndFrom, &stLvi);

            hHokyWnd = GetDlgItem(hDlg, IDHKC_FUNCKEY_INPUT);
            WndTagSet(hHokyWnd, stLvi.iItem);

            if (const ACCEL *pstAccel =
                    AccelKeyFindByCommand(*pvcAccel, (WORD)stLvi.lParam))
            {
                bMod = AccelHotModExchange(pstAccel->fVirt, 1);
                if (0x20 == pstAccel->key)
                {
                    SendMessage(hHokyWnd, HKM_SETHOTKEY,
                                MAKEWORD(pstAccel->key, bMod), 0);
                }
                else
                {
                    SendMessage(hHokyWnd, HKM_SETHOTKEY,
                                MAKEWORD(pstAccel->key, (bMod | HOTKEYF_EXT)),
                                0);
                }
            }
            else
            {
                SendMessage(hHokyWnd, HKM_SETHOTKEY, 0, 0);
            }

            SetFocus(GetDlgItem(hDlg, IDHKC_FUNCKEY_INPUT));
        }
        return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}

LPACCEL AccelKeyTableLoadAlloc(LPINT piEntry)
{
    UINT dCount, dValue;
    UINT i, aim = 0;
    TCHAR atKeyName[MIN_STRING];
    LPACCEL pstAccel = nullptr;
    LPCTSTR ptCntxIni = CntxIniPathGet();

    dCount =
        GetPrivateProfileInt(TEXT("Accelerator"), TEXT("Count"), 0, ptCntxIni);
    if (1 <= dCount)
    {
        pstAccel = (LPACCEL)malloc(dCount * sizeof(ACCEL));

        for (i = 0; dCount > i; i++)
        {
            StringCchPrintf(atKeyName, MIN_STRING, TEXT("AcCMD%d"), i);
            dValue =
                GetPrivateProfileInt(TEXT("Accelerator"), atKeyName, 0, ptCntxIni);
            if (0 == dValue)
            {
                continue;
            }
            if (!AccelKeyCommandAllowed(dValue))
            {
                continue;
            }
            pstAccel[aim].cmd = dValue;

            StringCchPrintf(atKeyName, MIN_STRING, TEXT("AcVirt%d"), i);
            dValue =
                GetPrivateProfileInt(TEXT("Accelerator"), atKeyName, 0, ptCntxIni);
            pstAccel[aim].fVirt = dValue;

            StringCchPrintf(atKeyName, MIN_STRING, TEXT("AcKey%d"), i);
            dValue =
                GetPrivateProfileInt(TEXT("Accelerator"), atKeyName, 0, ptCntxIni);
            pstAccel[aim].key = dValue;

            aim++;
        }
    }

    if (piEntry)
    {
        *piEntry = aim;
    }

    return pstAccel;
}

static HRESULT AccelKeyBindString(LPACCEL pstAccel, LPTSTR ptBuffer,
                                  UINT_PTR cchSize)
{
    TCHAR atKey[MIN_STRING];

    ZeroMemory(ptBuffer, cchSize * sizeof(TCHAR));

    if (FCONTROL & pstAccel->fVirt)
        StringCchCat(ptBuffer, cchSize, TEXT("Ctrl + "));
    if (FSHIFT & pstAccel->fVirt)
        StringCchCat(ptBuffer, cchSize, TEXT("Shift + "));
    if (FALT & pstAccel->fVirt)
        StringCchCat(ptBuffer, cchSize, TEXT("Alt + "));

    if ('0' <= pstAccel->key && pstAccel->key <= '9')
    {
        StringCchPrintf(atKey, MIN_STRING, TEXT("%c"), pstAccel->key);
    }
    else if ('A' <= pstAccel->key && pstAccel->key <= 'Z')
    {
        StringCchPrintf(atKey, MIN_STRING, TEXT("%c"), pstAccel->key);
    }
    else if (0x60 <= pstAccel->key && pstAccel->key <= 0x69)
    {
        StringCchPrintf(atKey, MIN_STRING, TEXT("NUMPAD%u"), pstAccel->key - 0x60);
    }
    else if (0x70 <= pstAccel->key && pstAccel->key <= 0x87)
    {
        StringCchPrintf(atKey, MIN_STRING, TEXT("F%u"), pstAccel->key - 0x70 + 1);
    }
    else
    {
        switch (pstAccel->key)
        {
        case VK_CANCEL:
            StringCchCopy(atKey, MIN_STRING, TEXT("Break"));
            break;
        case VK_BACK:
            StringCchCopy(atKey, MIN_STRING, TEXT("BackSpace"));
            break;
        case VK_TAB:
            StringCchCopy(atKey, MIN_STRING, TEXT("TAB"));
            break;
        case VK_CLEAR:
            StringCchCopy(atKey, MIN_STRING, TEXT("CLEAR"));
            break;
        case VK_RETURN:
            StringCchCopy(atKey, MIN_STRING, TEXT("Enter"));
            break;
        case VK_PAUSE:
            StringCchCopy(atKey, MIN_STRING, TEXT("Pause"));
            break;
        case VK_CAPITAL:
            StringCchCopy(atKey, MIN_STRING, TEXT("CAPITAL"));
            break;
        case VK_KANA:
            StringCchCopy(atKey, MIN_STRING, TEXT("KANA"));
            break;
        case VK_ESCAPE:
            StringCchCopy(atKey, MIN_STRING, TEXT("Esc"));
            break;
        case VK_CONVERT:
            StringCchCopy(atKey, MIN_STRING, TEXT("変換"));
            break;
        case VK_NONCONVERT:
            StringCchCopy(atKey, MIN_STRING, TEXT("无変換"));
            break;
        case VK_SPACE:
            StringCchCopy(atKey, MIN_STRING, TEXT("Space"));
            break;
        case VK_PRIOR:
            StringCchCopy(atKey, MIN_STRING, TEXT("PageUp"));
            break;
        case VK_NEXT:
            StringCchCopy(atKey, MIN_STRING, TEXT("PageDown"));
            break;
        case VK_END:
            StringCchCopy(atKey, MIN_STRING, TEXT("End"));
            break;
        case VK_HOME:
            StringCchCopy(atKey, MIN_STRING, TEXT("Home"));
            break;
        case VK_LEFT:
            StringCchCopy(atKey, MIN_STRING, TEXT("←"));
            break;
        case VK_UP:
            StringCchCopy(atKey, MIN_STRING, TEXT("↑"));
            break;
        case VK_RIGHT:
            StringCchCopy(atKey, MIN_STRING, TEXT("→"));
            break;
        case VK_DOWN:
            StringCchCopy(atKey, MIN_STRING, TEXT("↓"));
            break;
        case VK_SELECT:
            StringCchCopy(atKey, MIN_STRING, TEXT("SELECT"));
            break;
        case VK_PRINT:
            StringCchCopy(atKey, MIN_STRING, TEXT("PRINT"));
            break;
        case VK_EXECUTE:
            StringCchCopy(atKey, MIN_STRING, TEXT("EXECUTE"));
            break;
        case VK_SNAPSHOT:
            StringCchCopy(atKey, MIN_STRING, TEXT("PrintScr"));
            break;
        case VK_INSERT:
            StringCchCopy(atKey, MIN_STRING, TEXT("Insert"));
            break;
        case VK_DELETE:
            StringCchCopy(atKey, MIN_STRING, TEXT("Delete"));
            break;
        case VK_HELP:
            StringCchCopy(atKey, MIN_STRING, TEXT("Help"));
            break;
        case VK_LWIN:
            StringCchCopy(atKey, MIN_STRING, TEXT("左Win"));
            break;
        case VK_RWIN:
            StringCchCopy(atKey, MIN_STRING, TEXT("右Win"));
            break;
        case VK_APPS:
            StringCchCopy(atKey, MIN_STRING, TEXT("APPZ"));
            break;
        case VK_SLEEP:
            StringCchCopy(atKey, MIN_STRING, TEXT("SLEEP"));
            break;
        case VK_MULTIPLY:
            StringCchCopy(atKey, MIN_STRING, TEXT("NUM *"));
            break;
        case VK_ADD:
            StringCchCopy(atKey, MIN_STRING, TEXT("NUM +"));
            break;
        case VK_SEPARATOR:
            StringCchCopy(atKey, MIN_STRING, TEXT("NUM ,"));
            break;
        case VK_SUBTRACT:
            StringCchCopy(atKey, MIN_STRING, TEXT("NUM -"));
            break;
        case VK_DECIMAL:
            StringCchCopy(atKey, MIN_STRING, TEXT("NUM ."));
            break;
        case VK_DIVIDE:
            StringCchCopy(atKey, MIN_STRING, TEXT("NUM /"));
            break;
        case VK_NUMLOCK:
            StringCchCopy(atKey, MIN_STRING, TEXT("NumLock"));
            break;
        case VK_SCROLL:
            StringCchCopy(atKey, MIN_STRING, TEXT("ScrollLock"));
            break;
        case VK_OEM_NEC_EQUAL:
            StringCchCopy(atKey, MIN_STRING, TEXT("NUM ="));
            break;
        case VK_BROWSER_BACK:
            StringCchCopy(atKey, MIN_STRING, TEXT("戻る"));
            break;
        case VK_BROWSER_FORWARD:
            StringCchCopy(atKey, MIN_STRING, TEXT("進む"));
            break;
        case VK_BROWSER_REFRESH:
            StringCchCopy(atKey, MIN_STRING, TEXT("更新"));
            break;
        case VK_BROWSER_STOP:
            StringCchCopy(atKey, MIN_STRING, TEXT("停止"));
            break;
        case VK_BROWSER_SEARCH:
            StringCchCopy(atKey, MIN_STRING, TEXT("検索"));
            break;
        case VK_BROWSER_FAVORITES:
            StringCchCopy(atKey, MIN_STRING, TEXT("気入"));
            break;
        case VK_BROWSER_HOME:
            StringCchCopy(atKey, MIN_STRING, TEXT("ホム"));
            break;
        case VK_VOLUME_MUTE:
            StringCchCopy(atKey, MIN_STRING, TEXT("消音"));
            break;
        case VK_VOLUME_DOWN:
            StringCchCopy(atKey, MIN_STRING, TEXT("音下"));
            break;
        case VK_VOLUME_UP:
            StringCchCopy(atKey, MIN_STRING, TEXT("音上"));
            break;
        case VK_MEDIA_NEXT_TRACK:
            StringCchCopy(atKey, MIN_STRING, TEXT("次項"));
            break;
        case VK_MEDIA_PREV_TRACK:
            StringCchCopy(atKey, MIN_STRING, TEXT("前項"));
            break;
        case VK_MEDIA_STOP:
            StringCchCopy(atKey, MIN_STRING, TEXT("停止"));
            break;
        case VK_MEDIA_PLAY_PAUSE:
            StringCchCopy(atKey, MIN_STRING, TEXT("再生"));
            break;
        case VK_LAUNCH_MAIL:
            StringCchCopy(atKey, MIN_STRING, TEXT("メル"));
            break;
        case VK_LAUNCH_MEDIA_SELECT:
            StringCchCopy(atKey, MIN_STRING, TEXT("選択"));
            break;
        case VK_LAUNCH_APP1:
            StringCchCopy(atKey, MIN_STRING, TEXT("APP1"));
            break;
        case VK_LAUNCH_APP2:
            StringCchCopy(atKey, MIN_STRING, TEXT("APP2"));
            break;
        case VK_OEM_1:
            StringCchCopy(atKey, MIN_STRING, TEXT(":"));
            break;
        case VK_OEM_PLUS:
            StringCchCopy(atKey, MIN_STRING, TEXT(";"));
            break;
        case VK_OEM_COMMA:
            StringCchCopy(atKey, MIN_STRING, TEXT(","));
            break;
        case VK_OEM_MINUS:
            StringCchCopy(atKey, MIN_STRING, TEXT("-"));
            break;
        case VK_OEM_PERIOD:
            StringCchCopy(atKey, MIN_STRING, TEXT("."));
            break;
        case VK_OEM_2:
            StringCchCopy(atKey, MIN_STRING, TEXT("/"));
            break;
        case VK_OEM_3:
            StringCchCopy(atKey, MIN_STRING, TEXT("@"));
            break;
        case VK_OEM_4:
            StringCchCopy(atKey, MIN_STRING, TEXT("["));
            break;
        case VK_OEM_5:
            StringCchCopy(atKey, MIN_STRING, TEXT("\\"));
            break;
        case VK_OEM_6:
            StringCchCopy(atKey, MIN_STRING, TEXT("]"));
            break;
        case VK_OEM_7:
            StringCchCopy(atKey, MIN_STRING, TEXT("^"));
            break;
        case VK_OEM_8:
            StringCchCopy(atKey, MIN_STRING, TEXT("_"));
            break;
        case VK_OEM_102:
            StringCchCopy(atKey, MIN_STRING, TEXT("ろ"));
            break;
        case VK_OEM_ATTN:
            StringCchCopy(atKey, MIN_STRING, TEXT("CapsLock"));
            break;
        case VK_OEM_COPY:
            StringCchCopy(atKey, MIN_STRING, TEXT("カ夕ひら"));
            break;
        case VK_OEM_AUTO:
            StringCchCopy(atKey, MIN_STRING, TEXT("半/全 漢1"));
            break;
        case VK_OEM_ENLW:
            StringCchCopy(atKey, MIN_STRING, TEXT("半/全 漢2"));
            break;
        default:
            StringCchPrintf(atKey, MIN_STRING, TEXT("0x%02X"), pstAccel->key);
            break;
        }
    }
    StringCchCat(ptBuffer, cchSize, atKey);

    return S_OK;
}

HRESULT AccelKeyTextBuild(LPTSTR ptText, UINT_PTR cchSize, DWORD dCommand,
                          CONST LPACCEL pstAccel, INT iEntry)
{
    TCHAR atKeystr[SUB_STRING];
    INT i;

    for (i = 0; iEntry > i; i++)
    {
        if (pstAccel[i].cmd == dCommand)
        {
            AccelKeyBindString(&(pstAccel[i]), atKeystr, SUB_STRING);

            StringCchCat(ptText, cchSize, TEXT("\r\n"));
            StringCchCat(ptText, cchSize, atKeystr);

            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

static VOID AccelKeyListInit(HWND hDlg, vector<ACCEL> *pvcAccel)
{
    HWND hLvWnd;
    LVCOLUMN stLvColm;
    LVITEM stItem;
    RECT rect;
    LONG width;
    UINT i;
    INT j;
    UINT dCommandId;
    TCHAR atBuffer[SUB_STRING];

    hLvWnd = GetDlgItem(hDlg, IDLV_FUNCKEY_LIST);
    GetClientRect(hLvWnd, &rect);
    width = rect.right - 23;
    width /= 2;

    ListView_DeleteAllItems(hLvWnd);
    while (0 < Header_GetItemCount(ListView_GetHeader(hLvWnd)))
    {
        ListView_DeleteColumn(hLvWnd, 0);
    }

    ZeroMemory(&stLvColm, sizeof(LVCOLUMN));
    stLvColm.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    stLvColm.fmt = LVCFMT_LEFT;

    stLvColm.iSubItem = 0;
    stLvColm.pszText = const_cast<LPTSTR>(TEXT("기능"));
    stLvColm.cx = width;
    ListView_InsertColumn(hLvWnd, 0, &stLvColm);

    stLvColm.iSubItem = 1;
    stLvColm.pszText = const_cast<LPTSTR>(TEXT("설정된 단축키"));
    stLvColm.cx = width;
    ListView_InsertColumn(hLvWnd, 1, &stLvColm);

    ZeroMemory(&stItem, sizeof(LVITEM));

    for (i = 0, j = 0; AppCommandCount() > i; i++)
    {
        const APP_COMMAND_ITEM *pstItem = AppCommandAt(i);

        if (!(pstItem))
            continue;

        dCommandId = pstItem->dCommandId;
        stItem.iItem = j;

        if (0 == dCommandId || AppCommandHasChildMenu(dCommandId) ||
            !AccelKeyCommandAllowed(dCommandId))
        {
            continue;
        }

        StringCchCopy(atBuffer, SUB_STRING, AppCommandLabelGet(dCommandId));
        stItem.mask = LVIF_TEXT | LVIF_PARAM;
        stItem.pszText = atBuffer;
        stItem.lParam = dCommandId;
        stItem.iSubItem = 0;
        ListView_InsertItem(hLvWnd, &stItem);

        ZeroMemory(atBuffer, sizeof(atBuffer));

        if (const ACCEL *pstAccel = AccelKeyFindByCommand(*pvcAccel, (WORD)dCommandId))
        {
            AccelKeyBindString(const_cast<LPACCEL>(pstAccel), atBuffer, SUB_STRING);
        }

        stItem.mask = LVIF_TEXT;
        stItem.pszText = atBuffer;
        stItem.iSubItem = 1;
        ListView_SetItem(hLvWnd, &stItem);

        j++;
    }
}

static BYTE AccelHotModExchange(BYTE bSrc, BOOLEAN bDrct)
{
    BYTE bDest = 0;

    if (bDrct)
    {
        if (bSrc & FSHIFT)
            bDest |= HOTKEYF_SHIFT;
        if (bSrc & FCONTROL)
            bDest |= HOTKEYF_CONTROL;
        if (bSrc & FALT)
            bDest |= HOTKEYF_ALT;
    }
    else
    {
        if (bSrc & HOTKEYF_SHIFT)
            bDest |= FSHIFT;
        if (bSrc & HOTKEYF_CONTROL)
            bDest |= FCONTROL;
        if (bSrc & HOTKEYF_ALT)
            bDest |= FALT;

        bDest |= (FVIRTKEY | FNOINVERT);
    }

    return bDest;
}

static HRESULT AccelKeyTableSave(vector<ACCEL> *pvcAccel)
{
    INT_PTR i;
    TCHAR atKeyName[MIN_STRING], atBuff[MIN_STRING];
    LPCTSTR ptCntxIni = CntxIniPathGet();

    ZeroMemory(atBuff, sizeof(atBuff));
    WritePrivateProfileSection(TEXT("Accelerator"), atBuff, ptCntxIni);

    i = 0;
    for (const ACCEL &stAccel : *pvcAccel)
    {
        StringCchPrintf(atKeyName, MIN_STRING, TEXT("AcCMD%d"), i);
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), stAccel.cmd);
        WritePrivateProfileString(TEXT("Accelerator"), atKeyName, atBuff,
                                  ptCntxIni);

        StringCchPrintf(atKeyName, MIN_STRING, TEXT("AcVirt%d"), i);
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), stAccel.fVirt);
        WritePrivateProfileString(TEXT("Accelerator"), atKeyName, atBuff,
                                  ptCntxIni);

        StringCchPrintf(atKeyName, MIN_STRING, TEXT("AcKey%d"), i);
        StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), stAccel.key);
        WritePrivateProfileString(TEXT("Accelerator"), atKeyName, atBuff,
                                  ptCntxIni);

        i++;
    }

    StringCchPrintf(atBuff, MIN_STRING, TEXT("%u"), i);
    WritePrivateProfileString(TEXT("Accelerator"), TEXT("Count"), atBuff,
                              ptCntxIni);

    return S_OK;
}

static HRESULT AccelKeySettingReset(HWND hDlg, vector<ACCEL> *pvcAccel)
{
    HWND hLvWnd;
    HACCEL hAccel;
    LPACCEL pstAccel = nullptr;
    INT iItems, i;

    hAccel = LoadAccelerators(CntxInstanceGet(),
                              MAKEINTRESOURCE(IDC_SUNDAY));

    iItems = CopyAcceleratorTable(hAccel, nullptr, 0);
    if (0 >= iItems)
        return E_POINTER;

    pstAccel = (LPACCEL)malloc(iItems * sizeof(ACCEL));
    if (!(pstAccel))
        return E_OUTOFMEMORY;

    iItems = CopyAcceleratorTable(hAccel, pstAccel, iItems);

    DestroyAcceleratorTable(hAccel);

    pvcAccel->clear();

    for (i = 0; iItems > i; i++)
    {
        pvcAccel->push_back(pstAccel[i]);
    }

    FREE(pstAccel);

    hLvWnd = GetDlgItem(hDlg, IDLV_FUNCKEY_LIST);
    ListView_DeleteAllItems(hLvWnd);

    return S_OK;
}

static HRESULT AccelKeyBindExistCheck(HWND hDlg, LPACCEL pstAccel,
                                      vector<ACCEL> *pvcAccel)
{
    WORD dCommand;
    TCHAR atFuncName[MIN_STRING], atMsg[MAX_STRING];

    for (const ACCEL &stAccel : *pvcAccel)
    {
        if (pstAccel->key == stAccel.key && pstAccel->fVirt == stAccel.fVirt)
        {
            dCommand = stAccel.cmd;
            AppCommandNameCopy(dCommand, atFuncName, MIN_STRING);

            StringCchPrintf(atMsg, MAX_STRING,
                            TEXT("이 단축키는「%s」로 지정되어 있습니다."),
                            atFuncName);
            MessageBox(hDlg, atMsg, TEXT("중복 단축키 안내"), MB_OK | MB_ICONWARNING);

            return E_ACCESSDENIED;
        }
    }

    return S_OK;
}

static HRESULT AccelKeyBindListMod(HWND hDlg, INT iItem, LPACCEL pstAccel,
                                   vector<ACCEL> *pvcAccel)
{
    HWND hLvWnd = GetDlgItem(hDlg, IDLV_FUNCKEY_LIST);
    HWND hHkcWnd = GetDlgItem(hDlg, IDHKC_FUNCKEY_INPUT);
    LVITEM stLvi;
    TCHAR atBuffer[SUB_STRING];
    WORD dCommand;
    stLvi = {};
    stLvi.mask = LVIF_PARAM;
    stLvi.iItem = iItem;
    ListView_GetItem(hLvWnd, &stLvi);
    dCommand = stLvi.lParam;

    ACCEL *pstBoundAccel = AccelKeyFindByCommand(pvcAccel, dCommand);

    if (pstAccel)
    {
        if (!(pstBoundAccel))
        {
            pstAccel->cmd = dCommand;
            pvcAccel->push_back(*pstAccel);
        }
        else
        {
            pstBoundAccel->key = pstAccel->key;
            pstBoundAccel->fVirt = pstAccel->fVirt;
        }

        AccelKeyBindString(pstAccel, atBuffer, SUB_STRING);
        stLvi = {};
        stLvi.mask = LVIF_TEXT;
        stLvi.iItem = iItem;
        stLvi.iSubItem = 1;
        stLvi.pszText = atBuffer;
        ListView_SetItem(hLvWnd, &stLvi);
    }
    else
    {
        auto itAccel = find_if(pvcAccel->begin(), pvcAccel->end(),
                               [dCommand](const ACCEL &stAccel)
                               { return stAccel.cmd == dCommand; });

        if (pvcAccel->end() == itAccel)
        {
            return E_HANDLE;
        }

        pvcAccel->erase(itAccel);
        SendMessage(hHkcWnd, HKM_SETHOTKEY, 0, 0);
        ZeroMemory(atBuffer, sizeof(atBuffer));
        stLvi = {};
        stLvi.mask = LVIF_TEXT;
        stLvi.iItem = iItem;
        stLvi.iSubItem = 1;
        stLvi.pszText = atBuffer;
        ListView_SetItem(hLvWnd, &stLvi);
    }

    return S_OK;
}

HRESULT AccelKeyMenuRewrite(HWND hWnd, LPACCEL pstAccel, CONST INT iEntry)
{
    HMENU hMenu;
    WORD dCmd;
    INT i, j, iRslt;
    UINT mRslt;
    UINT_PTR d, cchSz;
    BOOLEAN bModify;
    TCHAR atBuffer[MAX_STRING], atBind[SUB_STRING];

    hMenu = GetMenu(hWnd);

    for (i = 0; (INT)AppCommandCount() > i; i++)
    {
        const APP_COMMAND_ITEM *pstItem = AppCommandAt((UINT)i);

        if (!(pstItem))
            continue;

        dCmd = (WORD)pstItem->dCommandId;
        if (0 == dCmd || !AccelKeyCommandAllowed(dCmd))
        {
            continue;
        }

        ZeroMemory(atBuffer, sizeof(atBuffer));
        iRslt = GetMenuString(hMenu, dCmd, atBuffer, MAX_STRING, MF_BYCOMMAND);
        if (!(iRslt))
        {
            continue;
        }

        bModify = FALSE;

        StringCchLength(atBuffer, MAX_STRING, &cchSz);
        for (d = 0; cchSz > d; d++)
        {
            if (TEXT('\t') == atBuffer[d])
            {
                atBuffer[d] = 0;
                bModify = TRUE;
                break;
            }
        }

        for (j = 0; iEntry > j; j++)
        {
            if (dCmd == pstAccel[j].cmd)
            {
                ZeroMemory(atBind, sizeof(atBind));
                AccelKeyBindString(&(pstAccel[j]), atBind, SUB_STRING);

                StringCchCat(atBuffer, MAX_STRING, TEXT("\t"));
                StringCchCat(atBuffer, MAX_STRING, atBind);

                bModify = TRUE;
                break;
            }
        }

        if (bModify)
        {
            mRslt = GetMenuState(hMenu, dCmd, MF_BYCOMMAND);
            ModifyMenu(hMenu, dCmd, (MF_CHECKED & mRslt), dCmd, atBuffer);
        }
    }

    DrawMenuBar(hWnd);

    return S_OK;
}

#endif