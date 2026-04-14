#ifndef SUNDAY_UI_TEXT_H
#define SUNDAY_UI_TEXT_H

// 리소스 스크립트와 C++ 소스가 함께 쓰는 UI 문자열 모음
#ifdef RC_INVOKED
#define ORR_UI_TEXT(value) value
#else
#define ORR_UI_TEXT(value) TEXT(value)
#endif

// 파일
#define ORR_UI_LABEL_NEWFILE               ORR_UI_TEXT("새 파일")
#define ORR_UI_LABEL_OPEN                  ORR_UI_TEXT("불러오기")
#define ORR_UI_LABEL_OPEN_HISTORY          ORR_UI_TEXT("열어본 파일 기록")
#define ORR_UI_LABEL_OVERWRITESAVE         ORR_UI_TEXT("저장하기")
#define ORR_UI_LABEL_RENAMESAVE            ORR_UI_TEXT("다른 이름으로 저장하기")
#define ORR_UI_LABEL_IMAGE_SAVE            ORR_UI_TEXT("이미지 파일 내보내기")
#define ORR_UI_LABEL_FILE_CLOSE            ORR_UI_TEXT("파일 닫기")
#define ORR_UI_LABEL_GENERAL_OPTION        ORR_UI_TEXT("환경설정")
#define ORR_UI_LABEL_MENUEDIT_DLG_OPEN     ORR_UI_TEXT("우클릭 메뉴 설정")
#define ORR_UI_LABEL_COLOUR_EDIT_OPEN      ORR_UI_TEXT("글씨 색상 설정")
#define ORR_UI_LABEL_UNICODE_TOGGLE        ORR_UI_TEXT("유니코드 문자 사용")
#define ORR_UI_LABEL_ACCELKEY_EDIT_DLG_OPEN ORR_UI_TEXT("단축키 설정")
#define ORR_UI_LABEL_ABOUT                 ORR_UI_TEXT("버전 정보")
#define ORR_UI_LABEL_EXIT                  ORR_UI_TEXT("종료")

#define ORR_UI_MENU_NEWFILE                ORR_UI_TEXT("새 파일(&N)")
#define ORR_UI_MENU_OPEN                   ORR_UI_TEXT("불러오기(&O)")
#define ORR_UI_MENU_OPEN_HISTORY           ORR_UI_TEXT("열어본 파일 기록(&H)")
#define ORR_UI_MENU_OVERWRITESAVE          ORR_UI_TEXT("저장하기(&S)")
#define ORR_UI_MENU_RENAMESAVE             ORR_UI_TEXT("다른 이름으로 저장하기(&M)")
#define ORR_UI_MENU_IMAGE_SAVE             ORR_UI_TEXT("이미지 파일 내보내기(&I)")
#define ORR_UI_MENU_FILE_CLOSE             ORR_UI_TEXT("파일 닫기(&C)")
#define ORR_UI_MENU_GENERAL_OPTION         ORR_UI_TEXT("환경설정(&G)")
#define ORR_UI_MENU_MENUEDIT_DLG_OPEN      ORR_UI_TEXT("우클릭 메뉴 설정(&E)")
#define ORR_UI_MENU_COLOUR_EDIT_OPEN       ORR_UI_TEXT("글씨 색상 설정(&L)")
#define ORR_UI_MENU_ACCELKEY_EDIT_DLG_OPEN ORR_UI_TEXT("단축키 설정(&K)")
#define ORR_UI_MENU_ABOUT                  ORR_UI_TEXT("버전 정보(&A)")
#define ORR_UI_MENU_EXIT                   ORR_UI_TEXT("종료(&Q)")

// 편집
#define ORR_UI_LABEL_UNDO                  ORR_UI_TEXT("실행 취소")
#define ORR_UI_LABEL_REDO                  ORR_UI_TEXT("다시 실행")
#define ORR_UI_LABEL_CUT                   ORR_UI_TEXT("잘라내기")
#define ORR_UI_LABEL_COPY                  ORR_UI_TEXT("복사하기")
#define ORR_UI_LABEL_PASTE                 ORR_UI_TEXT("붙여넣기")
#define ORR_UI_LABEL_DELETE                ORR_UI_TEXT("지우기")
#define ORR_UI_LABEL_SJISCOPY_ALL          ORR_UI_TEXT("AA 전체를 코드 형식으로 복사")
#define ORR_UI_LABEL_ALLSEL                ORR_UI_TEXT("전체 선택")
#define ORR_UI_LABEL_SQSELECT              ORR_UI_TEXT("직사각형 선택 모드")
#define ORR_UI_LABEL_SQUARE_PASTE          ORR_UI_TEXT("직사각형 붙여넣기")
#define ORR_UI_LABEL_LAYERBOX              ORR_UI_TEXT("레이어 박스")
#define ORR_UI_LABEL_EXTRACTION_MODE       ORR_UI_TEXT("분할 선택 모드")
#define ORR_UI_LABEL_FIND_DLG_OPEN         ORR_UI_TEXT("찾기")
#define ORR_UI_LABEL_FIND_HIGHLIGHT_OFF    ORR_UI_TEXT("검색 설정 초기화")

#define ORR_UI_MENU_UNDO                   ORR_UI_TEXT("실행 취소(&U)")
#define ORR_UI_MENU_REDO                   ORR_UI_TEXT("다시 실행(&R)")
#define ORR_UI_MENU_CUT                    ORR_UI_TEXT("잘라내기(&T)")
#define ORR_UI_MENU_COPY                   ORR_UI_TEXT("복사하기(&C)")
#define ORR_UI_MENU_PASTE                  ORR_UI_TEXT("붙여넣기(&P)")
#define ORR_UI_MENU_DELETE                 ORR_UI_TEXT("지우기(&D)")
#define ORR_UI_MENU_SJISCOPY_ALL           ORR_UI_TEXT("AA 전체를 코드 형식으로 복사(&S)")
#define ORR_UI_MENU_ALLSEL                 ORR_UI_TEXT("전체 선택(&A)")
#define ORR_UI_MENU_SQSELECT               ORR_UI_TEXT("직사각형 선택 모드(&B)")
#define ORR_UI_MENU_SQUARE_PASTE           ORR_UI_TEXT("직사각형 붙여넣기(&Q)")
#define ORR_UI_MENU_LAYERBOX               ORR_UI_TEXT("레이어 박스(&L)")
#define ORR_UI_MENU_EXTRACTION_MODE        ORR_UI_TEXT("분할 선택 모드(&E)")
#define ORR_UI_MENU_FIND_DLG_OPEN          ORR_UI_TEXT("찾기(&F)")
#define ORR_UI_MENU_FIND_HIGHLIGHT_OFF     ORR_UI_TEXT("검색 설정 초기화(&H)")

#define ORR_UI_LABEL_FIND_JUMP_NEXT        ORR_UI_TEXT("다음 찾기")
#define ORR_UI_LABEL_FIND_JUMP_PREV        ORR_UI_TEXT("이전 찾기")
#define ORR_UI_LABEL_FIND_TARGET_SET       ORR_UI_TEXT("검색 대상 설정")

// 삽입
#define ORR_UI_LABEL_MN_UNISPACE           ORR_UI_TEXT("유니코드 공백")
#define ORR_UI_LABEL_IN_01SPACE            ORR_UI_TEXT("1도트 공백 [ U8202 ]")
#define ORR_UI_LABEL_IN_02SPACE            ORR_UI_TEXT("2도트 공백 [ U8201 ]")
#define ORR_UI_LABEL_IN_03SPACE            ORR_UI_TEXT("3도트 공백 [ U8198 ]")
#define ORR_UI_LABEL_IN_04SPACE            ORR_UI_TEXT("4도트 공백 [ U8197 ]")
#define ORR_UI_LABEL_IN_05SPACE            ORR_UI_TEXT("5도트 공백 [ U8196 ]")
#define ORR_UI_LABEL_IN_08SPACE            ORR_UI_TEXT("8도트 공백 [ U8194 ]")
#define ORR_UI_LABEL_IN_10SPACE            ORR_UI_TEXT("10도트 공백 [ U8199 ]")
#define ORR_UI_LABEL_IN_16SPACE            ORR_UI_TEXT("16도트 공백 [ U8195 ]")
#define ORR_UI_LABEL_IN_UNI_SPACE          ORR_UI_TEXT("유니코드 공백")
#define ORR_UI_LABEL_MN_COLOUR_SEL         ORR_UI_TEXT("컬러 태그")
#define ORR_UI_LABEL_INSTAG_COLOUR         ORR_UI_TEXT("단색 태그")
#define ORR_UI_LABEL_INSTAG_GRADIENT       ORR_UI_TEXT("그라데이션 태그")
#define ORR_UI_LABEL_INSTAG_SPO            ORR_UI_TEXT("SPO 태그")
#define ORR_UI_LABEL_FRMINSBOX_OPEN        ORR_UI_TEXT("말풍선 박스")
#define ORR_UI_LABEL_MN_INSFRAME_SEL       ORR_UI_TEXT("말풍선 입력")
#define ORR_UI_LABEL_INSFRAME_EDIT         ORR_UI_TEXT("말풍선 편집")
#define ORR_UI_LABEL_MN_USER_REFS          ORR_UI_TEXT("유저 아이템")
#define ORR_UI_LABEL_USERINS_NA            ORR_UI_TEXT("유저 아이템")

#define ORR_UI_MENU_MN_UNISPACE            ORR_UI_TEXT("유니코드 공백(&S)")
#define ORR_UI_MENU_IN_01SPACE             ORR_UI_TEXT(" &1도트 공백 [ U8202 ]")
#define ORR_UI_MENU_IN_02SPACE             ORR_UI_TEXT(" &2도트 공백 [ U8201 ]")
#define ORR_UI_MENU_IN_03SPACE             ORR_UI_TEXT(" &3도트 공백 [ U8198 ]")
#define ORR_UI_MENU_IN_04SPACE             ORR_UI_TEXT(" &4도트 공백 [ U8197 ]")
#define ORR_UI_MENU_IN_05SPACE             ORR_UI_TEXT(" &5도트 공백 [ U8196 ]")
#define ORR_UI_MENU_IN_08SPACE             ORR_UI_TEXT(" &8도트 공백 [ U8194 ]")
#define ORR_UI_MENU_IN_10SPACE             ORR_UI_TEXT("&10도트 공백 [ U8199 ]")
#define ORR_UI_MENU_IN_16SPACE             ORR_UI_TEXT("&16도트 공백 [ U8195 ]")
#define ORR_UI_MENU_MN_COLOUR_SEL          ORR_UI_TEXT("컬러 태그(&C)")
#define ORR_UI_MENU_INSTAG_SPO             ORR_UI_TEXT("SPO 태그(&W)")
#define ORR_UI_MENU_INSTAG_COLOUR          ORR_UI_TEXT("단색 태그(&C)")
#define ORR_UI_MENU_INSTAG_GRADIENT        ORR_UI_TEXT("그라데이션 태그(&G)")
#define ORR_UI_MENU_FRMINSBOX_OPEN         ORR_UI_TEXT("말풍선 박스(&I)")
#define ORR_UI_MENU_MN_INSFRAME_SEL        ORR_UI_TEXT("말풍선 입력(&F)")
#define ORR_UI_MENU_INSFRAME_EDIT          ORR_UI_TEXT("말풍선 편집(&Z)")
#define ORR_UI_MENU_MN_USER_REFS           ORR_UI_TEXT("유저 아이템(&U)")

// 변형
#define ORR_UI_LABEL_RIGHT_GUIDE_SET       ORR_UI_TEXT("우측에 직선 추가")
#define ORR_UI_LABEL_INS_TOPSPACE          ORR_UI_TEXT("좌측 공백을 오른쪽으로 밀기")
#define ORR_UI_LABEL_DEL_TOPSPACE          ORR_UI_TEXT("좌측 공백을 왼쪽으로 당기기")
#define ORR_UI_LABEL_DEL_LASTSPACE         ORR_UI_TEXT("줄 끝에 남는 공백 제거")
#define ORR_UI_LABEL_DEL_LASTLETTER        ORR_UI_TEXT("줄 끝 마지막 문자 제거")
#define ORR_UI_LABEL_FILL_SPACE            ORR_UI_TEXT("선택 범위 공백으로 채우기")
#define ORR_UI_LABEL_FILL_ZENSP            ORR_UI_TEXT("빈 공간을 공백으로 채우기")
#define ORR_UI_LABEL_HEADHALF_EXCHANGE     ORR_UI_TEXT("첫 반각 공백을 유니코드로 변환")
#define ORR_UI_LABEL_MIRROR_INVERSE        ORR_UI_TEXT("좌우 반전")
#define ORR_UI_LABEL_UPSET_INVERSE         ORR_UI_TEXT("상하 반전")
#define ORR_UI_LABEL_INCREMENT_DOT         ORR_UI_TEXT("공백을 1도트 늘리기")
#define ORR_UI_LABEL_DECREMENT_DOT         ORR_UI_TEXT("공백을 1도트 줄이기")
#define ORR_UI_LABEL_INCR_DOT_LINES        ORR_UI_TEXT("AA 전체를 오른쪽으로 1도트 밀기")
#define ORR_UI_LABEL_DECR_DOT_LINES        ORR_UI_TEXT("AA 전체를 왼쪽으로 1도트 당기기")
#define ORR_UI_LABEL_DOT_SPLIT_LEFT        ORR_UI_TEXT("커서를 기준으로 왼쪽으로 당기기")
#define ORR_UI_LABEL_DOT_SPLIT_RIGHT       ORR_UI_TEXT("커서를 기준으로 오른쪽으로 밀기")
#define ORR_UI_LABEL_DOTDIFF_LOCK          ORR_UI_TEXT("조정 기준 위치 잠그기")
#define ORR_UI_LABEL_DOTDIFF_ADJT          ORR_UI_TEXT("커서 우측 공백 전부 지우기")

#define ORR_UI_MENU_RIGHT_GUIDE_SET        ORR_UI_TEXT("우측에 직선 추가(&R)")
#define ORR_UI_MENU_INS_TOPSPACE           ORR_UI_TEXT("좌측 공백을 오른쪽으로 밀기(&I)")
#define ORR_UI_MENU_DEL_TOPSPACE           ORR_UI_TEXT("좌측 공백을 왼쪽으로 당기기(&U)")
#define ORR_UI_MENU_DEL_LASTSPACE          ORR_UI_TEXT("줄 끝에 남는 공백 제거(&G)")
#define ORR_UI_MENU_DEL_LASTLETTER         ORR_UI_TEXT("줄 끝 마지막 문자 제거(&E)")
#define ORR_UI_MENU_FILL_SPACE             ORR_UI_TEXT("선택 범위 공백으로 채우기(&F)")
#define ORR_UI_MENU_FILL_ZENSP             ORR_UI_TEXT("빈 공간을 공백으로 채우기(&B)")
#define ORR_UI_MENU_HEADHALF_EXCHANGE      ORR_UI_TEXT("첫 반각 공백을 유니코드로 변환(&L)")
#define ORR_UI_MENU_MIRROR_INVERSE         ORR_UI_TEXT("좌우 반전(&M)")
#define ORR_UI_MENU_UPSET_INVERSE          ORR_UI_TEXT("상하 반전(&N)")
#define ORR_UI_MENU_INCREMENT_DOT          ORR_UI_TEXT("공백을 1도트 늘리기(&D)")
#define ORR_UI_MENU_DECREMENT_DOT          ORR_UI_TEXT("공백을 1도트 줄이기(&O)")
#define ORR_UI_MENU_INCR_DOT_LINES         ORR_UI_TEXT("AA 전체를 오른쪽으로 1도트 밀기(&J)")
#define ORR_UI_MENU_DECR_DOT_LINES         ORR_UI_TEXT("AA 전체를 왼쪽으로 1도트 당기기(&K)")
#define ORR_UI_MENU_DOT_SPLIT_LEFT         ORR_UI_TEXT("커서를 기준으로 왼쪽으로 당기기(&Q)")
#define ORR_UI_MENU_DOT_SPLIT_RIGHT        ORR_UI_TEXT("커서를 기준으로 오른쪽으로 밀기(&P)")
#define ORR_UI_MENU_DOTDIFF_LOCK           ORR_UI_TEXT("조정 기준 위치 잠그기(&R)")
#define ORR_UI_MENU_DOTDIFF_ADJT           ORR_UI_TEXT("커서 우측 공백 전부 지우기(&D)")

// 표시
#define ORR_UI_LABEL_SPACE_VIEW_TOGGLE     ORR_UI_TEXT("공백 문자 표시")
#define ORR_UI_LABEL_GRID_VIEW_TOGGLE      ORR_UI_TEXT("그리드 표시")
#define ORR_UI_LABEL_RIGHT_RULER_TOGGLE    ORR_UI_TEXT("우측 가이드라인 표시")
#define ORR_UI_LABEL_UNDER_RULER_TOGGLE    ORR_UI_TEXT("하단 가이드라인 표시")
#define ORR_UI_LABEL_PAGELIST_VIEW         ORR_UI_TEXT("페이지 목록")
#define ORR_UI_LABEL_INSERT_PALETTE        ORR_UI_TEXT("AA 팔레트")
#define ORR_UI_LABEL_BRUSH_PALETTE         ORR_UI_TEXT("채우기 팔레트")
#define ORR_UI_LABEL_TRACE_MODE_ON         ORR_UI_TEXT("트레이싱 모드")
#define ORR_UI_LABEL_ON_PREVIEW            ORR_UI_TEXT("미리보기")

#define ORR_UI_MENU_SPACE_VIEW_TOGGLE      ORR_UI_TEXT("공백 문자 표시(&W)")
#define ORR_UI_MENU_GRID_VIEW_TOGGLE       ORR_UI_TEXT("그리드 표시(&H)")
#define ORR_UI_MENU_RIGHT_RULER_TOGGLE     ORR_UI_TEXT("우측 가이드라인 표시(&M)")
#define ORR_UI_MENU_UNDER_RULER_TOGGLE     ORR_UI_TEXT("하단 가이드라인 표시(&S)")
#define ORR_UI_MENU_PAGELIST_VIEW          ORR_UI_TEXT("페이지 목록(&L)")
#define ORR_UI_MENU_INSERT_PALETTE         ORR_UI_TEXT("AA 팔레트(&T)")
#define ORR_UI_MENU_BRUSH_PALETTE          ORR_UI_TEXT("채우기 팔레트(&F)")
#define ORR_UI_MENU_TRACE_MODE_ON          ORR_UI_TEXT("트레이싱 모드(&R)")
#define ORR_UI_MENU_ON_PREVIEW             ORR_UI_TEXT("미리보기(&P)")

// 페이지/탐색/보조 메뉴
#define ORR_UI_LABEL_PAGEL_AATIP_TOGGLE    ORR_UI_TEXT("페이지 내용 팝업 표시")
#define ORR_UI_LABEL_PAGEL_RETURN_FOCUS    ORR_UI_TEXT("선택 후 편집창 포커스")
#define ORR_UI_LABEL_PAGEL_ADD             ORR_UI_TEXT("새 페이지 추가")
#define ORR_UI_LABEL_PAGEL_INSERT          ORR_UI_TEXT("새 페이지 삽입")
#define ORR_UI_LABEL_PAGEL_DUPLICATE       ORR_UI_TEXT("선택한 페이지를 복제")
#define ORR_UI_LABEL_PAGEL_DELETE          ORR_UI_TEXT("선택한 페이지를 삭제")

#define ORR_UI_MENU_PAGEL_AATIP_TOGGLE     ORR_UI_TEXT("페이지 내용 팝업 표시(&T)")
#define ORR_UI_MENU_PAGEL_ADD              ORR_UI_TEXT("새 페이지 추가(&N)")
#define ORR_UI_MENU_PAGEL_INSERT           ORR_UI_TEXT("새 페이지 삽입(&I)")
#define ORR_UI_MENU_PAGEL_DUPLICATE        ORR_UI_TEXT("선택한 페이지를 복제(&C)")
#define ORR_UI_MENU_PAGEL_DELETE           ORR_UI_TEXT("선택한 페이지를 삭제(&D)")

#define ORR_UI_MSG_PAGEL_DELETE_TITLE      ORR_UI_TEXT("삭제 확인")
#define ORR_UI_MSG_PAGEL_DELETE_LINE1      ORR_UI_TEXT("페이지를 삭제하면 되돌릴 수 없습니다.")
#define ORR_UI_MSG_PAGEL_DELETE_LINE2      ORR_UI_TEXT("정말로 삭제할까요?")
#define ORR_UI_MSG_PAGEL_MULTI_DELETE      ORR_UI_TEXT("여러 페이지를 삭제하려고 합니다.\r\n정말로 삭제할까요?")

#define ORR_UI_LABEL_PAGEL_UPFLOW          ORR_UI_TEXT("페이지를 위로 이동")
#define ORR_UI_LABEL_PAGEL_DOWNSINK        ORR_UI_TEXT("페이지를 아래로 이동")
#define ORR_UI_LABEL_PAGEL_RENAME          ORR_UI_TEXT("페이지 이름 변경")
#define ORR_UI_LABEL_TOPMOST_TOGGLE        ORR_UI_TEXT("항상 위에 표시")

#define ORR_UI_MENU_PAGEL_UPFLOW           ORR_UI_TEXT("페이지를 위로 이동(&W)")
#define ORR_UI_MENU_PAGEL_DOWNSINK         ORR_UI_TEXT("페이지를 아래로 이동(&S)")
#define ORR_UI_MENU_PAGEL_RENAME           ORR_UI_TEXT("페이지 이름 변경(&R)")
#define ORR_UI_MENU_TOPMOST_TOGGLE         ORR_UI_TEXT("항상 위에 표시(&Z)")

#define ORR_UI_LABEL_TRC_VIEWTOGGLE        ORR_UI_TEXT("트레이싱용 이미지 표시")
#define ORR_UI_LABEL_TMPLT_GROUP_PREV      ORR_UI_TEXT("템플릿 그룹 전환 ↑")
#define ORR_UI_LABEL_TMPLT_GROUP_NEXT      ORR_UI_TEXT("템플릿 그룹 전환 ↓")
#define ORR_UI_LABEL_WINDOW_CHANGE         ORR_UI_TEXT("윈도우 포커스 전환 ↑")
#define ORR_UI_LABEL_WINDOW_CHG_RVRS       ORR_UI_TEXT("윈도우 포커스 전환 ↓")
#define ORR_UI_LABEL_FILE_PREV             ORR_UI_TEXT("파일 전환 ↑")
#define ORR_UI_LABEL_FILE_NEXT             ORR_UI_TEXT("파일 전환 ↓")
#define ORR_UI_LABEL_PAGE_PREV             ORR_UI_TEXT("이전 페이지로")
#define ORR_UI_LABEL_PAGE_NEXT             ORR_UI_TEXT("다음 페이지로")
#define ORR_UI_LABEL_NOW_PAGE_REFRESH      ORR_UI_TEXT("현재 화면 새로고침")

#define ORR_UI_LABEL_LYB_INSERT            ORR_UI_TEXT("끼워넣기")
#define ORR_UI_LABEL_LYB_OVERRIDE          ORR_UI_TEXT("덮어쓰기")
#define ORR_UI_LABEL_LYB_COPY              ORR_UI_TEXT("클립보드로 복사")
#define ORR_UI_LABEL_LYB_DO_EDIT           ORR_UI_TEXT("내용 편집")
#define ORR_UI_LABEL_LYB_DELETE            ORR_UI_TEXT("내용 삭제")

#define ORR_UI_LABEL_BRUSH_ON_OFF          ORR_UI_TEXT("채우기 모드 켜기/끄기")

#define ORR_UI_LABEL_FRAME_INS_DECIDE      ORR_UI_TEXT("삽입하기")
#define ORR_UI_LABEL_FRMINSBOX_QCLOSE      ORR_UI_TEXT("삽입하고 창 닫기")
#define ORR_UI_LABEL_FRMINSBOX_PADDING     ORR_UI_TEXT("박스 크기에 말풍선 맞추기")

#define ORR_UI_LABEL_POSITION_RESET        ORR_UI_TEXT("창 배치 리셋")
#define ORR_UI_LABEL_OPEN_HIS_CLEAR        ORR_UI_TEXT("열어본 파일 기록 지우기")
#define ORR_UI_LABEL_REBER_DORESET         ORR_UI_TEXT("툴바를 초기 상태로 되돌리기")

#ifndef RC_INVOKED

// 명령 ID → 라벨/메뉴 텍스트 테이블 항목
typedef struct {
    UINT    dCommandID;     // IDM_* 명령 ID
    LPCTSTR ptLabel;        // 기본 UI 문자열
} UI_TEXT_ENTRY;

LPCTSTR UiTextGetLabel( UINT dCommandID );
LPCTSTR UiTextGetMenuText( UINT dCommandID );
const UI_TEXT_ENTRY* UiTextFindEntry( UINT dCommandID );
#endif

#endif
