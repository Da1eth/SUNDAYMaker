# ── 시스템 설정 ──
cmake_minimum_required(VERSION 3.20)
set(CMAKE_SYSTEM_NAME Windows)

set(_TGT "x64")
set(_HOST "Hostx64")

# ── Visual Studio 탐지 ──
set(_VSWHERE "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe")
if(NOT EXISTS "${_VSWHERE}")
    message(FATAL_ERROR "vswhere.exe를 찾을 수 없습니다: ${_VSWHERE}")
endif()

execute_process(
    COMMAND "${_VSWHERE}" -latest -products *
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
            -property installationPath
    OUTPUT_VARIABLE _VS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _RES
)
if(NOT _RES EQUAL 0 OR "${_VS}" STREQUAL "")
    message(FATAL_ERROR "Visual Studio Build Tools를 찾을 수 없습니다")
endif()
cmake_path(NORMAL_PATH _VS)

# ── Ninja 탐지 ──
if(CMAKE_GENERATOR MATCHES "Ninja")
    find_program(_NINJA_EXECUTABLE
        NAMES ninja ninja.exe
        HINTS "${_VS}/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja"
    )
    if(NOT _NINJA_EXECUTABLE)
        message(FATAL_ERROR "Ninja를 찾을 수 없습니다.")
    endif()
    set(CMAKE_MAKE_PROGRAM "${_NINJA_EXECUTABLE}" CACHE FILEPATH "Ninja executable" FORCE)
endif()

# ── MSVC 툴셋 탐지 ──
file(GLOB _MSVC_VERS LIST_DIRECTORIES true "${_VS}/VC/Tools/MSVC/*")
if(NOT _MSVC_VERS)
    message(FATAL_ERROR "MSVC 툴셋을 찾을 수 없습니다: ${_VS}/VC/Tools/MSVC/")
endif()
list(SORT _MSVC_VERS COMPARE NATURAL ORDER DESCENDING)
list(GET _MSVC_VERS 0 _MSVC)

set(_BIN "${_MSVC}/bin/${_HOST}/${_TGT}")

# ── Windows SDK 탐지 ──
execute_process(
    COMMAND reg query "HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0"
            /v InstallationFolder
    OUTPUT_VARIABLE _REG ERROR_QUIET RESULT_VARIABLE _REG_RES
)
set(_SDK_ROOT "")
if(_REG_RES EQUAL 0)
    string(REGEX MATCH "REG_SZ[ \t]+([^\r\n]+)" _MATCH "${_REG}")
    if(CMAKE_MATCH_1)
        string(STRIP "${CMAKE_MATCH_1}" _SDK_ROOT)
    endif()
endif()
if("${_SDK_ROOT}" STREQUAL "")
    set(_SDK_ROOT "C:/Program Files (x86)/Windows Kits/10")
endif()
cmake_path(NORMAL_PATH _SDK_ROOT)

file(GLOB _SDK_VERS LIST_DIRECTORIES true "${_SDK_ROOT}/include/*")
if(NOT _SDK_VERS)
    message(FATAL_ERROR "Windows SDK를 찾을 수 없습니다: ${_SDK_ROOT}/include/")
endif()
list(SORT _SDK_VERS COMPARE NATURAL ORDER DESCENDING)
list(GET _SDK_VERS 0 _SDK_VER_DIR)
cmake_path(GET _SDK_VER_DIR FILENAME _SDK_VER)

# ── 컴파일러/도구 경로 ──
set(CMAKE_C_COMPILER   "${_BIN}/cl.exe"  CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${_BIN}/cl.exe"  CACHE FILEPATH "" FORCE)
set(CMAKE_LINKER       "${_BIN}/link.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_AR           "${_BIN}/lib.exe"  CACHE FILEPATH "" FORCE)
set(CMAKE_RC_COMPILER  "${_SDK_ROOT}/bin/${_SDK_VER}/${_TGT}/rc.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_MT           "${_SDK_ROOT}/bin/${_SDK_VER}/${_TGT}/mt.exe" CACHE FILEPATH "" FORCE)

# ── 인클루드 경로 ──
set(_INC_DIRS
    "${_MSVC}/include"
    "${_MSVC}/ATLMFC/include"
    "${_VS}/VC/Auxiliary/VS/include"
    "${_SDK_ROOT}/include/${_SDK_VER}/ucrt"
    "${_SDK_ROOT}/include/${_SDK_VER}/um"
    "${_SDK_ROOT}/include/${_SDK_VER}/shared"
    "${_SDK_ROOT}/include/${_SDK_VER}/winrt"
    "${_SDK_ROOT}/include/${_SDK_VER}/cppwinrt"
)

# 컴파일러 테스트용
list(JOIN _INC_DIRS ";" _ENV_INCLUDE)
set(ENV{INCLUDE} "${_ENV_INCLUDE}")

# 빌드 시 /I 플래그 삽입
foreach(_D IN LISTS _INC_DIRS)
    if(EXISTS "${_D}")
        list(APPEND CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${_D}")
        list(APPEND CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${_D}")
    endif()
endforeach()

# RC 컴파일러용 인클루드
set(_RC_DIRS
    "${_SDK_ROOT}/include/${_SDK_VER}/um"
    "${_SDK_ROOT}/include/${_SDK_VER}/shared"
    "${_MSVC}/include"
)
set(_RC_FLAGS "")
foreach(_D IN LISTS _RC_DIRS)
    if(EXISTS "${_D}")
        string(APPEND _RC_FLAGS " /I\"${_D}\"")
    endif()
endforeach()
string(STRIP "${_RC_FLAGS}" _RC_FLAGS)
set(CMAKE_RC_FLAGS_INIT "${_RC_FLAGS}" CACHE STRING "" FORCE)

# ── 라이브러리 경로 ──
set(_LIB_DIRS
    "${_MSVC}/ATLMFC/lib/${_TGT}"
    "${_MSVC}/lib/${_TGT}"
    "${_SDK_ROOT}/lib/${_SDK_VER}/ucrt/${_TGT}"
    "${_SDK_ROOT}/lib/${_SDK_VER}/um/${_TGT}"
)

# 링크 테스트용
list(JOIN _LIB_DIRS ";" _ENV_LIB)
set(ENV{LIB} "${_ENV_LIB}")

# 빌드 시 /LIBPATH: 플래그 삽입
set(_LIB_FLAGS "")
foreach(_D IN LISTS _LIB_DIRS)
    if(EXISTS "${_D}")
        string(APPEND _LIB_FLAGS " /LIBPATH:\"${_D}\"")
    endif()
endforeach()
string(STRIP "${_LIB_FLAGS}" _LIB_FLAGS)
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${_LIB_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_LIB_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_LIB_FLAGS}" CACHE STRING "" FORCE)

# PATH에 bin 디렉토리 추가
set(ENV{PATH} "${_BIN};$ENV{PATH}")

message(STATUS "MSVC: ${_MSVC}")
message(STATUS "Windows SDK: ${_SDK_ROOT} (${_SDK_VER})")
message(STATUS "Target: ${_TGT}")
