#include "Sunday.h"
#include "AppModuleInternal.h"
#include "ViewCentralInternal.h"

static CONST TCHAR gcatLicense[] = {
    TEXT("이 프로그램은 자유 소프트웨어로, GNU 일반 공중 사용 허가서 버전 3 또는 그 이후 버전이 정하는 조건 하에 재배포 또는 수정할 수 있습니다. \r\n\r\n")
    TEXT("이 프로그램은 유용하게 사용될 목적으로 제작되어 배포되고 있지만 정상 작동 및 기능에 대한 보증은 전혀 하지 않습니다. \r\n\r\n")
    TEXT("자세한 내용은 <http://www.gnu.org/licenses/> 해당 링크에서 GNU 일반 공중 사용 허가서 본문을 확인해 주시기 바랍니다. \r\n\r\n")
    TEXT("이 프로그램은 OrinrinEditor을 기반으로 하고 있으며, 원본을 훼손할 의도가 없습니다. \r\n\r\n")};

static INT_PTR CALLBACK OptionDlgProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        AppUiFontApply(hDlg, TRUE);
        SetDlgItemText(hDlg, IDE_ABOUT_DISP, gcatLicense);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

BOOLEAN AppHandleMainCommand(HWND hWnd, INT id)
{
    INT iRslt;

    switch (id)
    {
    case IDM_ABOUT:
        DialogBox(ghAppInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
        return TRUE;

    case IDM_EXIT:
        iRslt = DocFileCloseCheck(hWnd);
        if (iRslt)
        {
            DestroyWindow(hWnd);
        }
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

HRESULT OptionDialogueOpen(VOID)
{
    UINT bSkipHangulBuff, bABUIbuff;

    bSkipHangulBuff = gbNoSjisSkipHangul;
    bABUIbuff = gdBackupInterval;
    DialogBoxParam(ghAppInst, MAKEINTRESOURCE(IDD_GENERAL_OPTION_DLG), ghMainWnd, OptionDlgProc, 0);

    if (bSkipHangulBuff != gbNoSjisSkipHangul)
    {
        HangulNoSjisToggle();
    }
    if (bABUIbuff != gdBackupInterval)
    {
        if (1 <= bABUIbuff)
            KillTimer(ghMainWnd, IDT_BACKUP_TIMER);
        if (1 <= gdBackupInterval)
            SetTimer(ghMainWnd, IDT_BACKUP_TIMER, (gdBackupInterval * 60000), nullptr);
    }

    MenuItemCheckOnOff(IDM_UNICODE_TOGGLE, gbUniPad);

    SqnSetting();

    return S_OK;
}

static INT_PTR CALLBACK OptionDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static UINT cdGrXp, cdGrYp, cdRtRr, cdUdRr;

    UINT id;
    INT dValue;
    TCHAR atBuff[SUB_STRING];

    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        const VIEWDISPLAYSTATE stDisplayState = ViewDisplayStateGet();
        AppUiFontApply(hDlg, TRUE);

        SendDlgItemMessage(hDlg, IDSL_LAYERBOX_TRANCED, TBM_SETRANGE, TRUE, MAKELPARAM(0, 0xE0));

        cdGrXp = stDisplayState.dGridXpos;
        StringCchPrintf(atBuff, SUB_STRING, TEXT("%d"), stDisplayState.dGridXpos);
        Edit_SetText(GetDlgItem(hDlg, IDE_GRID_X_POS), atBuff);

        cdGrYp = stDisplayState.dGridYpos;
        StringCchPrintf(atBuff, SUB_STRING, TEXT("%d"), stDisplayState.dGridYpos);
        Edit_SetText(GetDlgItem(hDlg, IDE_GRID_Y_POS), atBuff);

        cdRtRr = stDisplayState.dRightRuler;
        StringCchPrintf(atBuff, SUB_STRING, TEXT("%d"), stDisplayState.dRightRuler);
        Edit_SetText(GetDlgItem(hDlg, IDE_RIGHT_RULER_POS), atBuff);

        cdUdRr = stDisplayState.dUnderRuler;
        StringCchPrintf(atBuff, SUB_STRING, TEXT("%d"), stDisplayState.dUnderRuler);
        Edit_SetText(GetDlgItem(hDlg, IDE_UNDER_RULER_POS), atBuff);

        dValue = InitParamValue(INIT_LOAD, VL_BACKUP_INTVL, 3);
        StringCchPrintf(atBuff, SUB_STRING, TEXT("%d"), dValue);
        Edit_SetText(GetDlgItem(hDlg, IDE_AUTO_BU_INTVL), atBuff);

        CheckDlgButton(hDlg, IDCB_AUTOBU_MSG_ON, gbAutoBUmsg ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDCB_SAVE_MSG_ON, gbSaveMsgOn ? BST_CHECKED : BST_UNCHECKED);
        CheckRadioButton(hDlg, IDRB_CRLF_STRB, IDRB_CRLF_2CH_YY, gbCrLfCode ? IDRB_CRLF_2CH_YY : IDRB_CRLF_STRB);
        CheckDlgButton(hDlg, IDCB_USE_UNISPACE_SET, gbUniPad ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDCB_NOSJIS_SKIP_HANGUL, gbNoSjisSkipHangul ? BST_CHECKED : BST_UNCHECKED);

        dValue = InitParamValue(INIT_LOAD, VL_MULTI_ACT_E, 0);
        CheckDlgButton(hDlg, IDCB_MULTIACT_ENA, dValue ? BST_CHECKED : BST_UNCHECKED);

        dValue = InitParamValue(INIT_LOAD, VL_PLS_LN_DOCK, 1);
        CheckDlgButton(hDlg, IDCB_DOCKING_STYLE, dValue ? BST_CHECKED : BST_UNCHECKED);

        dValue = InitParamValue(INIT_LOAD, VL_LAST_OPEN, LASTOPEN_DO);
        CheckRadioButton(hDlg, IDRB_LASTOPEN_DO, IDRB_LASTOPEN_ASK, (IDRB_LASTOPEN_DO + dValue));
#ifdef SPMOZI_ENCODE
        dValue = InitParamValue(INIT_LOAD, VL_SPMOZI_ENC, 0);
        CheckDlgButton(hDlg, IDCB_SPMOZI_ENCODE, dValue ? BST_CHECKED : BST_UNCHECKED);
#endif
        dValue = InitParamValue(INIT_LOAD, VL_LAYER_TRANS, 192);
        SendDlgItemMessage(hDlg, IDSL_LAYERBOX_TRANCED, TBM_SETPOS, TRUE, (dValue - 0x1F));

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        id = LOWORD(wParam);
        switch (id)
        {
        case IDB_APPLY:
        case IDOK:
        {
            auto stDisplayState = ViewDisplayStateGet();
            Edit_GetText(GetDlgItem(hDlg, IDE_GRID_X_POS), atBuff, SUB_STRING);
            stDisplayState.dGridXpos = StrToInt(atBuff);
            InitParamValue(INIT_SAVE, VL_GRID_X_POS, stDisplayState.dGridXpos);
            if (cdGrXp != stDisplayState.dGridXpos)
                ViewRedrawSetLine(-1);

            Edit_GetText(GetDlgItem(hDlg, IDE_GRID_Y_POS), atBuff, SUB_STRING);
            stDisplayState.dGridYpos = StrToInt(atBuff);
            InitParamValue(INIT_SAVE, VL_GRID_Y_POS, stDisplayState.dGridYpos);
            if (cdGrYp != stDisplayState.dGridYpos)
                ViewRedrawSetLine(-1);

            Edit_GetText(GetDlgItem(hDlg, IDE_RIGHT_RULER_POS), atBuff, SUB_STRING);
            stDisplayState.dRightRuler = StrToInt(atBuff);
            InitParamValue(INIT_SAVE, VL_R_RULER_POS, stDisplayState.dRightRuler);
            if (cdRtRr != stDisplayState.dRightRuler)
                ViewRedrawSetLine(-1);

            Edit_GetText(GetDlgItem(hDlg, IDE_UNDER_RULER_POS), atBuff, SUB_STRING);
            stDisplayState.dUnderRuler = StrToInt(atBuff);
            InitParamValue(INIT_SAVE, VL_U_RULER_POS, stDisplayState.dUnderRuler);
            if (cdUdRr != stDisplayState.dUnderRuler)
                ViewRedrawSetLine(-1);

            Edit_GetText(GetDlgItem(hDlg, IDE_AUTO_BU_INTVL), atBuff, SUB_STRING);
            gdBackupInterval = StrToInt(atBuff);
            InitParamValue(INIT_SAVE, VL_BACKUP_INTVL, gdBackupInterval);

            dValue = IsDlgButtonChecked(hDlg, IDCB_AUTOBU_MSG_ON);
            gbAutoBUmsg = dValue ? 1 : 0;
            InitParamValue(INIT_SAVE, VL_BACKUP_MSGON, gbAutoBUmsg);

            dValue = IsDlgButtonChecked(hDlg, IDCB_SAVE_MSG_ON);
            gbSaveMsgOn = dValue ? 1 : 0;
            InitParamValue(INIT_SAVE, VL_SAVE_MSGON, gbSaveMsgOn);

            gbCrLfCode = 0;
            if (IsDlgButtonChecked(hDlg, IDRB_CRLF_2CH_YY))
            {
                gbCrLfCode = 1;
            }
            InitParamValue(INIT_SAVE, VL_CRLF_CODE, gbCrLfCode);

            dValue = IsDlgButtonChecked(hDlg, IDCB_USE_UNISPACE_SET);
            gbUniPad = dValue ? 1 : 0;
            InitParamValue(INIT_SAVE, VL_USE_UNICODE, gbUniPad);

            dValue = IsDlgButtonChecked(hDlg, IDCB_NOSJIS_SKIP_HANGUL);
            gbNoSjisSkipHangul = dValue ? 1 : 0;
            InitParamValue(INIT_SAVE, VL_NOSJIS_SKIP_HANGUL, gbNoSjisSkipHangul);

            dValue = IsDlgButtonChecked(hDlg, IDCB_MULTIACT_ENA);
            InitParamValue(INIT_SAVE, VL_MULTI_ACT_E, dValue ? 1 : 0);

            dValue = IsDlgButtonChecked(hDlg, IDCB_DOCKING_STYLE);
            InitParamValue(INIT_SAVE, VL_PLS_LN_DOCK, dValue ? 1 : 0);
#ifdef SPMOZI_ENCODE
            dValue = IsDlgButtonChecked(hDlg, IDCB_SPMOZI_ENCODE);
            gbSpMoziEnc = dValue ? 1 : 0;
            InitParamValue(INIT_SAVE, VL_SPMOZI_ENC, gbSpMoziEnc);
#endif
            if (IsDlgButtonChecked(hDlg, IDRB_LASTOPEN_NON))
            {
                dValue = LASTOPEN_NON;
            }
            else if (IsDlgButtonChecked(hDlg, IDRB_LASTOPEN_ASK))
            {
                dValue = LASTOPEN_ASK;
            }
            else
            {
                dValue = LASTOPEN_DO;
            }
            InitParamValue(INIT_SAVE, VL_LAST_OPEN, dValue);

            dValue = SendDlgItemMessage(hDlg, IDSL_LAYERBOX_TRANCED, TBM_GETPOS, 0, 0);
            dValue += 0x1F;
            InitParamValue(INIT_SAVE, VL_LAYER_TRANS, dValue);
            LayerBoxAlphaSet(dValue);

            if (IDOK == id)
            {
                EndDialog(hDlg, IDOK);
            }
            return (INT_PTR)TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return (INT_PTR)FALSE;
}