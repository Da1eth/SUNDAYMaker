# SUNDAY Uplifts Neothunnus to Deft ASCII Yielder
SUNDAY Maker는 일본의 [OrinrinEditor](https://github.com/SikigamiHNQ/OrinrinEditor) 의 [고속화 개조 버전](https://bbs.yaruyomi.com/test/read.cgi/ban/1711371396/131) 을 기반으로 하는 아스키 아트 편집 프로그램입니다.  

## 설치 방법
### 기본
빌드 완료된 파일은 [Release 탭](https://github.com/Da1eth/SUNDAYMaker/releases) 에서 다운로드받을 수 있습니다.  
원하는 곳에 압축을 해제한 후 동봉된 템플릿과 함께 사용하면 됩니다.

### 마이그레이션
기존에 사용하던 오린린 폴더를 복사한 후, 해당 폴더의 OrinrinEditor.exe를 삭제하고 Sunday.exe 만 집어넣으면 됩니다. 마이그레이션 진행 후 Utoho.ini 등 기존 설정 파일이 삭제됩니다.

### 직접 빌드하기
>git clone --recursive https://github.com/Da1eth/SUNDAYMaker

서브모듈이 포함되어 있기 때문에 꼭 "--recursive" 옵션을 포함해 주세요.  
빌드에는 Windows 10 또는 11 SDK, Windows용 C++ CMake 도구, x64/x86용 MSVC 빌드 도구(최신)이 필요합니다. [Visual Studio 공식 페이지](https://visualstudio.microsoft.com/ko/downloads/#build-tools-for-visual-studio-2026) 에서 필요한 도구를 설치할 수 있습니다.  
cmake --preset x64-release 및 cmake --build --preset x64-release 를 진행하면 빌드가 완료됩니다.

## 기능
기존 오린린과 달라진 기능은 다음과 같습니다.
|　이름　| 해설 |
|-------:|:-----|
| 한글 | 일본어가 아니고 한글입니다. |
| HeadKasen 내장 | 멋진 머리 카센이 한글을 완벽하게 16도트로 표시합니다. |
| ANSI 열기 | 파일의 인코딩을 추측해 로케일 에뮬레이터 없이도 파일이 깨지지 않게 합니다. |
| UTF-8 기본 저장 | 드디어 마침내 진짜로 MLT 파일을 UTF-8로 저장합니다. |
| 유니코드 문자 완벽 표시 | 로케일 에뮬레이터 없이도 Shift-JIS 가 아닌 문자만 색칠됩니다. |
| Webp 지원 | 트레이싱 모드에서 Webp 파일을 열 수 있습니다. |
| 트레이싱 모드 크기 확장 | 확대 축소 범위가 25% - 400% 가 됩니다. |
| 합성 개선 | AA 합성 시 점이 공백으로 판정되어 사라지던 문제를 해결했습니다. |
| 브러시 개선 | 현재 선택된 채우기 브러시를 확인하고, 브러시가 이제 더이상 AA를 깨트리지 않습니다. |
| 말풍선 개수 제한 삭제 | 이제 매우 많은 말풍선을 한 파일에 넣을 수 있습니다. |
| SPO 태그 입력 | 어장 연재용 SPO 태그를 빠르게 입력할 수 있습니다. |
| 단색 태그 및 그라데이션 태그 입력 | 어장 연재용 컬러 태그 및 그라데이션 채색을 해줍니다. |

## 라이선스
본 프로그램은 OrinrinEditor 원본과 동일한 GNU General Public License v3.0 을 적용받습니다.  
본 리포지토리에는 [stb](https://github.com/nothings/stb) 및 [JSON for Modern C++](https://github.com/nlohmann/json) 헤더 파일이 포함되어 있습니다.
