#include "Sunday.h"
#include "Palette.h"
#include "AppModuleInternal.h"
#include "UiText.h"

static HANDLE ghAppMutex;

INT APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    INT msRslt;
    MSG msg;

    INT iArgc;
    TCHAR atArgv[MAX_PATH];
    LPTSTR *pptArgs;

    INT iCode, dMultiEna;

    LPACCEL pstAccel;
    INT iEntry;

    HWND hWndActed;
    COPYDATASTRUCT stCopyData;

    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&iccex);

    LoadString(hInstance, IDS_APP_TITLE, gatAppTitle, MAX_STRING);
    LoadString(hInstance, IDC_SUNDAY, gatMainWindowClass, MAX_STRING);

    ZeroMemory(atArgv, sizeof(atArgv));
    pptArgs = CommandLineToArgvW(GetCommandLine(), &iArgc);
    if (2 <= iArgc)
    {
        StringCchCopy(atArgv, MAX_PATH, pptArgs[1]);
    }
    LocalFree(pptArgs);

    ghAppMutex = nullptr;

    GetModuleFileName(hInstance, gatExePath, MAX_PATH);
    PathRemoveFileSpec(gatExePath);

    StringCchCopy(gatIniPath, MAX_PATH, gatExePath);
    PathAppend(gatIniPath, SETTINGS_INI_FILE);

    AppMigrateLegacySettings(gatExePath);
    AppUiResourcesInitialise(hInstance);

    dMultiEna = InitParamValue(INIT_LOAD, VL_MULTI_ACT_E, 0);
    if (!(dMultiEna))
    {
        SECURITY_DESCRIPTOR stSeqDes;
        SECURITY_ATTRIBUTES secAttribute;
        InitializeSecurityDescriptor(&stSeqDes, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&stSeqDes, TRUE, 0, FALSE);
        secAttribute.nLength = sizeof(secAttribute);
        secAttribute.lpSecurityDescriptor = &stSeqDes;
        secAttribute.bInheritHandle = TRUE;

        ghAppMutex = CreateMutex(&secAttribute, TRUE, TEXT("SUNDAYMaker"));
        if (ghAppMutex && GetLastError() == ERROR_ALREADY_EXISTS)
        {
            hWndActed = FindWindow(gatMainWindowClass, nullptr);
            SetForegroundWindow(hWndActed);

            if (0 != atArgv[0])
            {
                stCopyData.dwData = 1;
                stCopyData.cbData = sizeof(atArgv);
                stCopyData.lpData = atArgv;

                SendMessage(hWndActed, WM_COPYDATA, 0, (LPARAM)(&stCopyData));
            }
            ReleaseMutex(ghAppMutex);
            CloseHandle(ghAppMutex);
            return 0;
        }
    }

    InitWndwClass(hInstance);

    DocBackupDirectoryInit(gatExePath);
    FrameInitialise(gatExePath, hInstance);
    MenuPickerInitialise(hInstance);

    gbUniPad = 0;
    gbDockTmplView = TRUE;

    iCode = InitParamValue(INIT_LOAD, VL_CLASHCOVER, 0);
    if (iCode)
    {
        iCode = MessageBox(nullptr, TEXT("프로그램이 비정상적으로 종료된 흔적이 있습니다.\r\n프로그램 기동 전에 백업 파일을 먼저 확인해주세요.\r\n「아니오」를 누르면 파일을 확인할 수 있도록 프로그램을 종료합니다."), TEXT("미안합니다"), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (IDNO == iCode)
        {
            return 0;
        }
    }
    InitParamValue(INIT_SAVE, VL_CLASHCOVER, 1);

    if (!InitInstance(hInstance, nCmdShow, atArgv))
    {
        return FALSE;
    }

    CntxEditInitialise(gatExePath, hInstance);

    RegisterHotKey(ghMainWnd, IDHK_THREAD_DROP, MOD_CONTROL | MOD_SHIFT, VK_D);

    pstAccel = AccelKeyTableLoadAlloc(&iEntry);
    AccelKeyTableCreate(pstAccel, iEntry);
    ToolBarInfoChange(pstAccel, iEntry);
    PageToolBarInfoChange(pstAccel, iEntry);
    FREE(pstAccel);

    for (;;)
    {
        msRslt = GetMessage(&msg, nullptr, 0, 0);
        if (1 != msRslt)
            break;

        if (AppModelessDialogHandleMessage(&msg))
            continue;

        if (!TranslateAccelerator(msg.hwnd, ghMainAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    InitParamValue(INIT_SAVE, VL_CLASHCOVER, 0);

    UnregisterHotKey(ghMainWnd, IDHK_THREAD_DROP);

    if (ghAppMutex)
    {
        ReleaseMutex(ghAppMutex);
        CloseHandle(ghAppMutex);
    }

    return (int)msg.wParam;
}

LPTSTR ExePathGet(VOID)
{
    return gatExePath;
}

ATOM InitWndwClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SUNDAY));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SUNDAY);
    wcex.lpszClassName = gatMainWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, INT nCmdShow, LPTSTR ptArgv)
{
    HWND hWnd;
    RECT rect;
    INT isMaxim = 0, sptBuf = PLIST_DOCK;
    HMENU hSubMenu;

    ghAppInst = hInstance;

    AppLayoutSplitBarClassRegister(hInstance);

    isMaxim = InitParamValue(INIT_LOAD, VL_MAXIMISED, 0);

    InitWindowPos(INIT_LOAD, WDP_MVIEW, &rect);
    if (0 == rect.right || 0 == rect.bottom)
    {
        hWnd = GetDesktopWindow();
        GetWindowRect(hWnd, &rect);
        rect.left = (rect.right - W_WIDTH) / 3;
        rect.top = (rect.bottom - W_HEIGHT) / 2;
        rect.right = W_WIDTH;
        rect.bottom = W_HEIGHT;
        InitWindowPos(INIT_SAVE, WDP_MVIEW, &rect);
    }

    hWnd = CreateWindowEx(0, gatMainWindowClass, gatAppTitle, WS_OVERLAPPEDWINDOW, rect.left, rect.top, rect.right, rect.bottom, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return FALSE;
    }

    gbUniPad = InitParamValue(INIT_LOAD, VL_USE_UNICODE, 1);
    gbNoSjisSkipHangul = InitParamValue(INIT_LOAD, VL_NOSJIS_SKIP_HANGUL, 0);

    gdBackupInterval = InitParamValue(INIT_LOAD, VL_BACKUP_INTVL, 3);
    gbAutoBUmsg = InitParamValue(INIT_LOAD, VL_BACKUP_MSGON, 1);
    gbCrLfCode = InitParamValue(INIT_LOAD, VL_CRLF_CODE, 0);
    gbSaveMsgOn = InitParamValue(INIT_LOAD, VL_SAVE_MSGON, 1);
    gbTmpltDock = InitParamValue(INIT_LOAD, VL_PLS_LN_DOCK, 1);

    gdPageByteMax = PAGE_BYTE_MAX;

    gbSpMoziEnc = InitParamValue(INIT_LOAD, VL_SPMOZI_ENC, 0);

    ghMainWnd = hWnd;

    hSubMenu = GetSystemMenu(hWnd, FALSE);
    InsertMenu(hSubMenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
    InsertMenu(hSubMenu, 0, MF_BYPOSITION, IDM_POSITION_RESET, UiTextGetLabel(IDM_POSITION_RESET));

    ghMainMenu = GetMenu(hWnd);

    hSubMenu = GetSubMenu(ghMainMenu, 0);
    DeleteMenu(hSubMenu, IDM_TREE_RECONSTRUCT, MF_BYCOMMAND);

    hSubMenu = GetSubMenu(ghMainMenu, 1);

    hSubMenu = GetSubMenu(ghMainMenu, 4);
    if (gbTmpltDock)
    {
        DeleteMenu(hSubMenu, IDM_PAGELIST_VIEW, MF_BYCOMMAND);
        DeleteMenu(hSubMenu, IDM_INSERT_PALETTE, MF_BYCOMMAND);
        DeleteMenu(hSubMenu, IDM_BRUSH_PALETTE, MF_BYCOMMAND);
    }

    AaFontCreate(1);
    AppClientAreaCalc(&rect);

    SqnSetting();

    if (gbTmpltDock)
    {
        grdSplitPos = AppDockSplitPosClamp(InitParamValue(INIT_LOAD, VL_MAIN_SPLIT, PLIST_DOCK), &rect, PLIST_DOCK);
        sptBuf = grdSplitPos;

        ghMainSplitWnd = AppLayoutSplitBarCreate(hInstance, hWnd, &rect, grdSplitPos);
    }
    else
    {
        ghMainSplitWnd = nullptr;
        grdSplitPos = 0;
    }

    ghPgVwWnd = PageListInitialise(hInstance, hWnd, &rect);
    ghPalInsertWnd = PaletteCommonInitialise(hInstance, hWnd, &rect);
    ViewInitialise(hInstance, hWnd, &rect, ptArgv);

    LayerBoxInitialise(hInstance, &rect);
    LayerBoxAlphaSet(InitParamValue(INIT_LOAD, VL_LAYER_TRANS, 192));

    UserItemInitialise(hWnd, TRUE);
    PreviewInitialise(hInstance, hWnd);
    FrameNameModifyMenu(hWnd);
    TraceInitialise(hWnd, TRUE);
    OpenHistoryInitialise(hWnd);
    DocInverseInit(TRUE);

    FindDialogueOpen(nullptr, nullptr);

    SetFocus(ghViewWnd);

    if (1 <= gdBackupInterval)
    {
        SetTimer(hWnd, IDT_BACKUP_TIMER, (gdBackupInterval * 60000), nullptr);
    }

    ghPalBucketWnd = PaletteBucketInitialise(hInstance, hWnd, &rect, nullptr);

    MenuItemCheckOnOff(IDM_UNICODE_TOGGLE, gbUniPad);

    if (isMaxim)
    {
        ShowWindow(hWnd, SW_MAXIMIZE);
        AppClientAreaCalc(&rect);
        if (ghMainSplitWnd)
        {
            grdSplitPos = sptBuf;
            AppLayoutSplitBarSync(ghMainSplitWnd, &rect, grdSplitPos);
        }
        Cls_OnSize(hWnd, SIZE_MAXIMIZED, rect.right, rect.top);
    }
    else
    {
        ShowWindow(hWnd, nCmdShow);
    }

    UpdateWindow(hWnd);

    return TRUE;
}
