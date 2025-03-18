#pragma once

#include "parstypes.h"
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/spec/attrs.h>
#include <library/cpp/html/spec/lextype.h>

#include <util/generic/strbuf.h>

struct THtmlChunk {
    PARSED_FLAGS flags;
    const char* text; /* text itself (case sensitive) - usually points to source document */
    ui32 leng;        /* length of text */
    TIrregTag Format;
    const NHtml::TAttribute* Attrs;
    size_t AttrCount;
    const NHtml::TTag* Tag;
    bool IsCDATA;
    bool IsWhitespace;
    ETagNamespace Namespace;

public:
    THtmlChunk(PARSED_TYPE type);

    inline HTLEX_TYPE GetLexType() const {
        return (HTLEX_TYPE)flags.apos;
    }

    inline TStringBuf GetText() const {
        return TStringBuf(text, leng);
    }
};

// Get tag name from original text. Suitable for non standard tags.
TStringBuf GetTagName(const THtmlChunk& chunk);

TStringBuf GetTagAttributeValue(const THtmlChunk& chunk, const TStringBuf& attrName);
