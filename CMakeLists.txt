# 버전 및 프로젝트 설정
cmake_minimum_required(VERSION 3.20)

project(Sunday LANGUAGES CXX C RC)

set(ORR_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(ORR_SRC_APP_DIR "${ORR_SRC_DIR}/app")
set(ORR_SRC_DOC_DIR "${ORR_SRC_DIR}/doc")
set(ORR_SRC_PALETTE_DIR "${ORR_SRC_DIR}/palette")
set(ORR_SRC_VIEW_DIR "${ORR_SRC_DIR}/view")
set(ORR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(ORR_RES_DIR "${CMAKE_SOURCE_DIR}/resource")
set(ORR_RESOURCE_MAIN "${ORR_RES_DIR}/Sunday.rc")
set(ORR_RESOURCE_FRAGMENTS
    ${ORR_RES_DIR}/rc/Icon.rc
    ${ORR_RES_DIR}/rc/SundayMain.rc
    ${ORR_RES_DIR}/rc/DocSearchCtrl.rc
    ${ORR_RES_DIR}/rc/FrameCtrl.rc
    ${ORR_RES_DIR}/rc/InsertColor.rc
    ${ORR_RES_DIR}/rc/LayerBox.rc
    ${ORR_RES_DIR}/rc/PageCtrl.rc
    ${ORR_RES_DIR}/rc/Setting.rc
    ${ORR_RES_DIR}/rc/ViewTrace.rc
)

# Ninja RC 컴파일 충돌 방지
if(CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_NINJA_CMCLDEPS_RC OFF)
endif()

set_source_files_properties(${ORR_RESOURCE_FRAGMENTS} PROPERTIES HEADER_FILE_ONLY TRUE)

# 소스 파일 세팅
set(APP_SOURCES
    ${ORR_SRC_APP_DIR}/AccelCtrl.cpp
    ${ORR_SRC_APP_DIR}/AppCommandRegistry.cpp
    ${ORR_SRC_APP_DIR}/AppBootstrap.cpp
    ${ORR_SRC_APP_DIR}/AppDialogs.cpp
    ${ORR_SRC_APP_DIR}/AppFileTab.cpp
    ${ORR_SRC_APP_DIR}/AppFonts.cpp
    ${ORR_SRC_APP_DIR}/AppLayout.cpp
    ${ORR_SRC_APP_DIR}/AppMainWindow.cpp
    ${ORR_SRC_APP_DIR}/AppMainWindowInput.cpp
    ${ORR_SRC_APP_DIR}/AppMainWindowLifecycle.cpp
    ${ORR_SRC_APP_DIR}/AppModelessDialogs.cpp
    ${ORR_SRC_APP_DIR}/AppPagePreload.cpp
    ${ORR_SRC_APP_DIR}/AppTrayIcon.cpp
    ${ORR_SRC_APP_DIR}/CommonUtil.cpp
    ${ORR_SRC_APP_DIR}/ContextCtrl.cpp
    ${ORR_SRC_APP_DIR}/ImageExport.cpp
    ${ORR_SRC_APP_DIR}/MenuPicker.cpp
    ${ORR_SRC_APP_DIR}/Sunday.cpp
    ${ORR_SRC_APP_DIR}/PageCtrl.cpp
    ${ORR_SRC_APP_DIR}/PageSneakPeek.cpp
    ${ORR_SRC_APP_DIR}/SettingsIniStore.cpp
    ${ORR_SRC_APP_DIR}/SettingsJson.cpp
    ${ORR_SRC_APP_DIR}/SettingsMigration.cpp
    ${ORR_SRC_APP_DIR}/TextEncoding.cpp
    ${ORR_SRC_APP_DIR}/ToolBar.cpp
    ${ORR_SRC_APP_DIR}/UiText.cpp
)

set(DOC_SOURCES
    ${ORR_SRC_DOC_DIR}/DocAdjustant.cpp
    ${ORR_SRC_DOC_DIR}/DocCalculationCommon.cpp
    ${ORR_SRC_DOC_DIR}/DocCalculator.cpp
    ${ORR_SRC_DOC_DIR}/DocCore.cpp
    ${ORR_SRC_DOC_DIR}/DocFileCtrl.cpp
    ${ORR_SRC_DOC_DIR}/DocFlipCtrl.cpp
    ${ORR_SRC_DOC_DIR}/DocInsDelCtrl.cpp
    ${ORR_SRC_DOC_DIR}/DocOperateLog.cpp
    ${ORR_SRC_DOC_DIR}/DocPreview.cpp
    ${ORR_SRC_DOC_DIR}/DocSearchCtrl.cpp
    ${ORR_SRC_DOC_DIR}/DocSelect.cpp
)

set(PALETTE_SOURCES
    ${ORR_SRC_PALETTE_DIR}/InsertColor.cpp
    ${ORR_SRC_PALETTE_DIR}/InsertFrame.cpp
    ${ORR_SRC_PALETTE_DIR}/InsertLayer.cpp
    ${ORR_SRC_PALETTE_DIR}/InsertUi.cpp
    ${ORR_SRC_PALETTE_DIR}/PaletteBucket.cpp
    ${ORR_SRC_PALETTE_DIR}/PaletteCommon.cpp
    ${ORR_SRC_PALETTE_DIR}/PaletteListUi.cpp
    ${ORR_SRC_PALETTE_DIR}/UserItem.cpp
)

set(VIEW_SOURCES
    ${ORR_SRC_VIEW_DIR}/EditorController.cpp
    ${ORR_SRC_VIEW_DIR}/ViewCaret.cpp
    ${ORR_SRC_VIEW_DIR}/ViewCentral.cpp
    ${ORR_SRC_VIEW_DIR}/ViewCentralColour.cpp
    ${ORR_SRC_VIEW_DIR}/ViewCentralCommand.cpp
    ${ORR_SRC_VIEW_DIR}/ViewCentralRedraw.cpp
    ${ORR_SRC_VIEW_DIR}/ViewKeyButton.cpp
    ${ORR_SRC_VIEW_DIR}/ViewSelect.cpp
    ${ORR_SRC_VIEW_DIR}/ViewTrace.cpp
)

set(SOURCES
    ${APP_SOURCES}
    ${DOC_SOURCES}
    ${PALETTE_SOURCES}
    ${VIEW_SOURCES}
)

set(HEADERS
    ${ORR_INCLUDE_DIR}/AppLayoutInternal.h
    ${ORR_INCLUDE_DIR}/AppCommandRegistry.h
    ${ORR_INCLUDE_DIR}/AppModuleInternal.h
    ${ORR_INCLUDE_DIR}/EditorController.h
    ${ORR_INCLUDE_DIR}/InsertUi.h
    ${ORR_INCLUDE_DIR}/SettingsIniStore.h
    ${ORR_INCLUDE_DIR}/Sunday.h
    ${ORR_INCLUDE_DIR}/Palette.h
    ${ORR_INCLUDE_DIR}/pch.h
    ${ORR_INCLUDE_DIR}/resource.h
    ${ORR_INCLUDE_DIR}/stb_image_write.h
    ${ORR_INCLUDE_DIR}/TextEncoding.h
    ${ORR_INCLUDE_DIR}/UiText.h
    ${ORR_INCLUDE_DIR}/ViewCentralInternal.h
)

# SQLite3 빌드
add_library(sqlite3 STATIC include/Sqlite3/sqlite3.c)
target_compile_definitions(sqlite3 PRIVATE SQLITE_API=)
set_property(TARGET sqlite3 PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
if(MSVC)
    target_compile_options(sqlite3 PRIVATE /utf-8 /W0)
endif()

# 실행 파일 빌드
add_executable(Sunday WIN32
    ${SOURCES}
    ${HEADERS}
    ${ORR_RESOURCE_MAIN}
    ${ORR_RESOURCE_FRAGMENTS}
)

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${SOURCES} ${HEADERS} ${ORR_RESOURCE_MAIN} ${ORR_RESOURCE_FRAGMENTS})

target_compile_features(Sunday PRIVATE cxx_std_23)

target_compile_definitions(Sunday PRIVATE
    WIN32
    _WINDOWS
    UNICODE
    _UNICODE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

target_compile_options(Sunday PRIVATE
    /utf-8
    /W3
    /Zc:strictStrings-
    $<$<CONFIG:Release>:/O2 /Oi /GL /GF>
    $<$<CONFIG:Debug>:/Od /W4 /RTC1>
)

# 정적 런타임 (/MT, /MTd)
set_property(TARGET Sunday PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_include_directories(Sunday PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${ORR_INCLUDE_DIR}
)

target_precompile_headers(Sunday PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${ORR_INCLUDE_DIR}/pch.h>"
)

# 링크
target_link_libraries(Sunday PRIVATE
    shell32
    Comdlg32
    ComCtl32
    imm32
    shlwapi
    sqlite3
)

target_link_options(Sunday PRIVATE
    /MACHINE:X64
    /MANIFEST:NO
    $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    $<$<CONFIG:Debug>:/DEBUG>
)

# 빌드 후 매니페스트 삽입
add_custom_command(TARGET Sunday POST_BUILD
    COMMAND "${CMAKE_MT}" -nologo -manifest "${ORR_RES_DIR}/Sunday.manifest"
            "-outputresource:$<TARGET_FILE:Sunday>;#1"
    COMMENT "Embedding manifest"
)
