#pragma once

#include <kernel/text_marks/hilite.h>

#include <util/charset/wide.h>

struct THiliteMark {
    const TUtf16String OpenTag;
    const TUtf16String CloseTag;

    THiliteMark(const TUtf16String& opentag, const TUtf16String& closetag)
        : OpenTag(opentag)
        , CloseTag(closetag)
    {
    }

    THiliteMark(const char* opentag, const char* closetag)
        : OpenTag(ASCIIToWide(opentag))
        , CloseTag(ASCIIToWide(closetag))
    {
    }
};
static const THiliteMark DEFAULT_MARKS(NTextMarks::HILITE_OPEN, NTextMarks::HILITE_CLOSE);
