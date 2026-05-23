#pragma once

#include "Sunday.h"

extern FILES_ITR gitFileIt;
extern INT gixFocusPage;

inline FILES_ITR DocCurrentFileIterator()
{
    return gitFileIt;
}

inline INT DocCurrentPageIndex()
{
    return gixFocusPage;
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
