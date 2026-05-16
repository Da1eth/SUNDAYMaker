// 문서 파일/문자열 검색과 검색 상태 관리를 담당한다.
#include "DocAppBridgeInternal.h"
#include "DocContextInternal.h"
#include "DocViewBridgeInternal.h"
//-------------------------------------------------------------------------------------------------

// 마지막 검색 문자열과 다음 시작 위치를 유지해 F3 반복 검색에 사용한다.

//-------------------------------------------------------------------------------------------------

extern list<ONEFILE> gltMultiFiles; //    複数ファイル保持
// イテレータのtypedefはヘッダへ

extern FILES_ITR gitFileIt; //        今見てるファイルの本体

extern INT gixFocusPage; //        注目中のページ・とりあえず０・０インデックス

EXTERNED HWND ghFindDlg; // 検索ダイヤログのハンドル

static TCHAR gatLastPtn[MAX_PATH]; // 最新の検索文字列を覚えておく

static TCHAR atSetPattern[MAX_PATH]; // 検索開始した文字列・検索ボタン連打したら次々進むの判断に使う
static INT giSetRange;               // 検索開始したときの、検索範囲
static BOOLEAN gbSetModCrlf;         // 検索開始したときの、￥ｎ対応

static UINT gdNextStart; // 今回の検索終端位置＝次の検索開始位置
static INT giSearchPage; // 検索してるページ。ページ渡り検索用
//-------------------------------------------------------------------------------------------------

INT_PTR CALLBACK FindStrDlgProc(HWND, UINT, WPARAM, LPARAM);
HRESULT FindExecute(HWND);
INT_PTR FindPageSearch(LPTSTR, INT, FILES_ITR);

UINT_PTR SearchPatternStruct(LPTSTR, UINT_PTR, LPTSTR, BOOLEAN);
HRESULT FindPageSelectSet(INT, INT, INT, FILES_ITR);

//-------------------------------------------------------------------------------------------------

// 検索ダイヤログを開く・モーダレスで
HRESULT FindDialogueOpen(HINSTANCE hInst, HWND hWnd)
{

    if (hInst == nullptr || hWnd == nullptr) //    変数初期化しておくだけ
    {
        gdNextStart = 0;
        giSearchPage = 0;

        ZeroMemory(gatLastPtn, sizeof(gatLastPtn));

        return S_OK;
    }

    if (ghFindDlg != nullptr)
    {
        SetForegroundWindow(ghFindDlg);
        return S_FALSE;
    }

    ghFindDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_FIND_STRING_DLG), hWnd, FindStrDlgProc, 0);

    ShowWindow(ghFindDlg, SW_SHOW);

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 検索ダイヤログのプロシージャ
INT_PTR CALLBACK FindStrDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWorkWnd;
    UINT id;

    switch (message)
    {
    default:
        break;

    case WM_INITDIALOG:
        EnableWindow(GetDlgItem(hDlg, IDB_FIND_CLEAR), FALSE);
        ShowWindow(GetDlgItem(hDlg, IDB_FIND_CLEAR), SW_HIDE);
        ZeroMemory(atSetPattern, sizeof(atSetPattern));
        giSetRange = 0;
        gbSetModCrlf = 0;
        gdNextStart = 0;
        giSearchPage = 0;

        //    コンボボックスに項目入れる
        hWorkWnd = GetDlgItem(hDlg, IDCB_FIND_TARGET);
        ComboBox_InsertString(hWorkWnd, 0, TEXT("현재 페이지"));
        ComboBox_InsertString(hWorkWnd, 1, TEXT("현재 파일 전체"));
        //        ComboBox_InsertString( hWorkWnd, 2, TEXT("開いている全てのファイル") );無しで
        ComboBox_SetCurSel(hWorkWnd, giSetRange); //    今の検索モードを反映する
        //    覚えとくのはあとでいい

        hWorkWnd = GetDlgItem(hDlg, IDE_FIND_TEXT);
        Edit_SetText(hWorkWnd, gatLastPtn); //    今の検索内容があれば転写する
        DocUiFocusWindow(hWorkWnd);

        return (INT_PTR)FALSE;

    case WM_COMMAND:
        id = LOWORD(wParam);
        hWorkWnd = GetDlgItem(hDlg, IDE_FIND_TEXT);
        switch (id)
        {
        case IDCANCEL:
            DestroyWindow(hDlg);
            return (INT_PTR)TRUE;
        case IDOK:
            FindExecute(hDlg);
            return (INT_PTR)TRUE; //    検索する

        case IDM_PASTE:
            DocUiSendWindowMessage(hWorkWnd, WM_PASTE, 0, 0);
            return (INT_PTR)TRUE;
        case IDM_COPY:
            DocUiSendWindowMessage(hWorkWnd, WM_COPY, 0, 0);
            return (INT_PTR)TRUE;
        case IDM_CUT:
            DocUiSendWindowMessage(hWorkWnd, WM_CUT, 0, 0);
            return (INT_PTR)TRUE;
        case IDM_UNDO:
            DocUiSendWindowMessage(hWorkWnd, WM_UNDO, 0, 0);
            return (INT_PTR)TRUE;
        default:
            break;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return (INT_PTR)TRUE;

    case WM_DESTROY:
        ghFindDlg = nullptr;
        DocViewFocusEditor();
        return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------

// アクセラレータで直接検索指定
HRESULT FindDirectly(HINSTANCE hInst, HWND hWnd, INT dCommand)
{
    BOOLEAN bOnCrLf = FALSE;
    INT cbSize;
    UINT_PTR cchSize, d;
    LPTSTR ptText = nullptr;

    TCHAR atGetPttn[MAX_PATH]; //    選択内容を確保

    if (IDM_FIND_JUMP_NEXT == dCommand) //    Ｆ３で通常検索
    {
        FindExecute(nullptr);
    }
    else if (IDM_FIND_TARGET_SET == dCommand) //    Ctrl+Ｆ３で選択範囲をけんさくキーワードにして検索
    {
        ZeroMemory(atGetPttn, sizeof(atGetPttn));

        //    未選択なら何もしない・返り値のバイトサイズにはヌルターミネータ含むので注意
        cbSize = DocSelectTextGetAlloc(D_UNI, (LPVOID *)(&ptText), nullptr);
        if (cbSize <= 0 || ptText == nullptr)
            return E_ABORT;

        StringCchLength(ptText, STRSAFE_MAX_CCH, &cchSize);
        if (0 == cchSize)
        {
            FREE(ptText);
            return E_ABORT;
        }
        if (MAX_PATH <= cchSize)
        {
            FREE(ptText);
            return E_ABORT;
        }

        //    改行含みチェック
        for (d = 0; cchSize > d; d++)
        {
            if (0x000D == ptText[d] && 0x000A == ptText[d + 1])
            {
                atGetPttn[d] = TEXT('\\');
                d++;
                atGetPttn[d] = TEXT('n');

                bOnCrLf = TRUE;
            }
            else
            {
                atGetPttn[d] = ptText[d];
            }
        }

        //    検索文字列を書き換える
        StringCchCopy(gatLastPtn, MAX_PATH, atGetPttn);
        gbSetModCrlf = bOnCrLf; //    改行チェック
        giSetRange = 1;         //    検索レンジはファイル全体固定にしちゃう
        gdNextStart = 0;        //    新規検索である
        giSearchPage = 0;

        FindExecute(nullptr);
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 検索実行
HRESULT FindExecute(HWND hDlg)
{
    UINT_PTR cchSzPtn;
    INT_PTR iPage;
    INT_PTR iFindTop;
    INT dRange; //    検索範囲　０頁　１ファイル 　キャンセル＞２全開きファイル
    BOOLEAN bModCrlf;

    TCHAR atPattern[MAX_PATH], atBuf[MAX_PATH];

    if (hDlg)
    {
        // 検索パヤーン確保
        Edit_GetText(GetDlgItem(hDlg, IDE_FIND_TEXT), atBuf, MAX_PATH);
        if (!(atBuf[0]))
            return E_ABORT; //    空文字列なら何もしない

        //    ￥ｎを改行、￥￥を￥にするか
        bModCrlf = IsDlgButtonChecked(hDlg, IDCB_MOD_CRLF_YEN);

        //    検索範囲    ０頁　１ファイル
        dRange = ComboBox_GetCurSel(GetDlgItem(hDlg, IDCB_FIND_TARGET));

        //    検索条件が全て同じなら、続きから
        if (!StrCmp(atSetPattern, atBuf) && (gbSetModCrlf == bModCrlf) && (giSetRange == dRange))
        {
        }
        else //    違うなら先頭から
        {
            gdNextStart = 0;
            giSearchPage = 0;
        }

        StringCchCopy(atSetPattern, MAX_PATH, atBuf);
        gbSetModCrlf = bModCrlf;
        giSetRange = dRange;

        StringCchCopy(gatLastPtn, MAX_PATH, atBuf); //    次にダイヤログ開いた時の表示用
    }
    else //    Ｆ３で直で来た
    {
        if (0 == gatLastPtn[0])
            return E_ABORT; //    何もしない

        //    直前の設定を流用
        StringCchCopy(atBuf, MAX_PATH, gatLastPtn);
        bModCrlf = gbSetModCrlf;
        dRange = giSetRange;
    }

    //    検索パヤーン確定
    cchSzPtn = SearchPatternStruct(atPattern, MAX_PATH, atBuf, bModCrlf);

    if (dRange) //    全頁検索しちゃったりして
    {
        iPage = DocNowFilePageCount(); //    頁数確保

        do
        {
            iFindTop = FindPageSearch(atPattern, giSearchPage, gitFileIt); //    対象頁を検索
            if (0 <= iFindTop)                                             //    発見
            {
                if (giSearchPage != gixFocusPage) //    なう頁でないなら該当の頁に移動する
                {
                    DocPageChange(giSearchPage); //    頁移動・gixFocusPage が書き換わる
                }
                FindPageSelectSet(iFindTop, cchSzPtn, gixFocusPage, gitFileIt); //    その場所にカーソルジャンプして選択状態にする
                gdNextStart = iFindTop + cchSzPtn;
                break;
            }
            else //    この頁には無かった
            {
                giSearchPage++;            //    次の頁に移動して
                gdNextStart = 0;           //    次はまた先頭から検索
                if (iPage <= giSearchPage) //    末端超えちゃったら
                {
                    giSearchPage = 0;
                    break;
                }
            }

        } while (0 > iFindTop); //    見つかるまで頁移動する
    }
    else //    なう頁のみ
    {
        iFindTop = FindPageSearch(atPattern, gixFocusPage, gitFileIt); //    単頁検索
        if (0 <= iFindTop)                                             //    先頭からの文字数
        {
            FindPageSelectSet(iFindTop, cchSzPtn, gixFocusPage, gitFileIt); //    その場所にカーソルジャンプして選択状態にする
            gdNextStart = iFindTop + cchSzPtn;
        }
        else
        {
            gdNextStart = 0;
        } //    先頭から
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------

// 検索パターンを確定する
UINT_PTR SearchPatternStruct(LPTSTR ptDest, UINT_PTR cchSize, LPTSTR ptSource, BOOLEAN bCrLf)
{
    UINT_PTR d, h;
    UINT_PTR cchSzPtn;

    ZeroMemory(ptDest, sizeof(TCHAR) * cchSize);

    if (bCrLf) //    エスケープシーケンスを展開
    {
        for (d = 0, h = 0; cchSize > d; d++, h++)
        {
            ptDest[h] = ptSource[d];
            if (0x005C == ptSource[d]) //    0x005Cは￥
            {
                d++;
                if (TEXT('n') == ptSource[d]) //    改行指示である場合
                {
                    ptDest[h] = TEXT('\r');
                    h++;
                    ptDest[h] = TEXT('\n');
                }
            }
            if (0x0000 == ptSource[d])
                break;
        }
    }
    else
    {
        StringCchCopy(ptDest, cchSize, ptSource);
    }

    StringCchLength(ptDest, cchSize, &cchSzPtn);

    return cchSzPtn;
}
//-------------------------------------------------------------------------------------------------

// 指定パヤーンを、指定ファイルの、指定頁で検索
INT_PTR FindPageSearch(LPTSTR ptPattern, INT iTgtPage, FILES_ITR itFile)
{
    INT dCch; //, dLeng;
    INT_PTR iRslt;
    INT_PTR dBytes;
    UINT_PTR cchSize, cchSzPtn;
    LPTSTR ptPage, ptCaret, ptFind = nullptr;

    //    TCHAR    ttBuf;

    if (!(ptPattern))
    {
        return -1;
    } //    nullptr不可

    StringCchLength(ptPattern, MAX_PATH, &cchSzPtn);

    //    頁全体確保
    dBytes = DocPageTextGetAlloc(itFile, iTgtPage, D_UNI, (LPVOID *)(&ptPage), FALSE);
    StringCchLength(ptPage, STRSAFE_MAX_CCH, &cchSize);

    ptCaret = ptPage;

    ptCaret += gdNextStart; //    直前の検索位置までオフセット

    iRslt = -1;

    ptFind = FindStringProc(ptCaret, ptPattern, &dCch); //    検索本体・エディタ側
    if (ptFind)                                         //    なんかあった
    {
        dCch += gdNextStart; //    オフセット量を足しておく

        //    FindPageSelectSet( dCch, cchSzPtn, iTgtPage, itFile );    //    その場所にカーソルジャンプして選択状態にする
        // 外でやるようにする
        iRslt = dCch;
    }

    FREE(ptPage);

    return iRslt;
}
//-------------------------------------------------------------------------------------------------

// 指定ファイルの指定頁の指定文字位置から指定文字数を選択状態にする。改行コード含む。
HRESULT FindPageSelectSet(INT iOffset, INT iRange, INT iPage, FILES_ITR itFile)
{
    UINT_PTR ln, iLetters; //, iLines;
    INT_PTR dMozis;
    INT iTotal, iDot, iLnTop, iSlide, mz, iNext, iWid = 0;
    INT iEndTotal, iEndOffset;

    LINE_ITR itLine, itLnEnd;

    itLine = itFile->vcCont.at(iPage).ltPage.begin();
    itLnEnd = itFile->vcCont.at(iPage).ltPage.end();

    iEndOffset = iOffset + iRange;
    iEndTotal = 0;
    iTotal = 0;
    iLnTop = 0;
    for (ln = 0; itLnEnd != itLine; itLine++, ln++)
    {
        dMozis = itLine->vcLine.size(); //    この行の文字数確認して
        iLetters = dMozis + 2;          //    改行コード

        iTotal += iLetters;

        if (iOffset < iTotal) //    行末端までの文字数よりオフセットが小さかったら、その行に含まれる
        {
            iSlide = iOffset - iLnTop; //    その行先頭からの文字数
            //    もし改行から検索だったら、iSlide = dMozis になる
            iNext = 0; //    改行が有る場合の残り文字数

            //    ここで改行の巻き込み状況を確認して、次の行かぶりとかチェック？
            iDot = 0;                       //    そこまでのドット数をため込む
            for (mz = 0; iSlide > mz; mz++) //    該当文字まで進めてドット数ためとく
            {
                //    もし改行から検索ならこれが成立
                if (dMozis <= mz)
                {
                    iDot += iWid;
                    break;
                }

                iDot += itLine->vcLine.at(mz).rdWidth;

                iWid = itLine->vcLine.at(mz).rdWidth; //    この文字の幅
            }

            //    該当範囲を選択状態にする
            DocPageSelStateToggle(FALSE); //    一旦選択状態は解除

            DocViewResetCaret(iDot, ln);
            DocViewSelectionMoveCheck(FALSE);
            DocViewSelectionPositionSet(nullptr);

            //    範囲選択、ViewSelAreaSelect を参考せよ

            break;
        }

        iLnTop += iLetters;

        iEndTotal += iLetters; //    検索内容の終端検出用
    }

    //    末端位置特定開始
    for (; itLnEnd != itLine; itLine++, ln++) //    オフセット発見された行から開始
    {
        dMozis = itLine->vcLine.size(); //    この行の文字数確認して
        iLetters = dMozis + 2;          //    改行コード

        iEndTotal += iLetters;

        if (iEndOffset < iEndTotal) //    行末端までの文字数よりオフセットが小さかったら、その行に含まれる
        {
            iSlide = iEndOffset - iLnTop; //    その行先頭からの文字数

            iWid = 0;
            iDot = 0;                       //    そこまでのドット数をため込む
            for (mz = 0; iSlide > mz; mz++) //    該当文字まで進めてドット数ためとく
            {
                //    もし改行から検索ならこれが成立
                if (dMozis <= mz)
                {
                    iDot += iWid;
                    break;
                }

                iDot += itLine->vcLine.at(mz).rdWidth;

                iWid = itLine->vcLine.at(mz).rdWidth; //    この文字の幅
            }

            DocViewResetCaret(iDot, ln);
            DocViewSelectionMoveCheck(TRUE);
            DocViewSelectionPositionSet(nullptr);

            break;
        }

        iLnTop += iLetters;
    }

    return S_OK;
}
//-------------------------------------------------------------------------------------------------
