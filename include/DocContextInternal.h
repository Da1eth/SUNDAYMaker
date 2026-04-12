#pragma once

#include "Sunday.h"

extern FILES_ITR gitFileIt;
extern INT gixFocusPage;

// 문서 세션 상태: 활성 파일/페이지 선택 상태 접근점
struct DocSessionContext
{
    FILES_ITR itCurrentFile;
    INT ixFocusPage{};
};

inline DocSessionContext DocSessionContextGet()
{
    DocSessionContext stContext{gitFileIt, gixFocusPage};
    return stContext;
}

inline FILES_ITR DocCurrentFileIterator()
{
    return DocSessionContextGet().itCurrentFile;
}

inline INT DocCurrentPageIndex()
{
    return DocSessionContextGet().ixFocusPage;
}

inline ONEFILE &DocCurrentFile()
{
    return *DocCurrentFileIterator();
}

inline ONEPAGE &DocCurrentPage()
{
    return DocCurrentFile().vcCont.at(DocCurrentPageIndex());
}

inline LINE_ITR DocCurrentLine(INT iLine)
{
    auto itLine = DocCurrentPage().ltPage.begin();
    std::advance(itLine, iLine);
    return itLine;
}