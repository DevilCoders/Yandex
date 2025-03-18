#include "event.h"

#include <util/string/ascii.h>

///////////////////////////////////////////////////////////////////////////////

THtmlChunk::THtmlChunk(PARSED_TYPE type)
    : text(nullptr)
    , leng(0U)
    , Format(IRREG_none)
    , Attrs(nullptr)
    , AttrCount(0)
    , Tag(nullptr)
    , IsCDATA(false)
    , IsWhitespace(false)
    , Namespace(ETagNamespace::HTML)
{
    flags.type = (i16)type;
}

///////////////////////////////////////////////////////////////////////////////

TStringBuf GetTagName(const THtmlChunk& chunk) {
    Y_ASSERT(chunk.GetLexType() == HTLEX_EMPTY_TAG ||
             chunk.GetLexType() == HTLEX_START_TAG ||
             chunk.GetLexType() == HTLEX_END_TAG);

    if (chunk.text == nullptr) {
        return TStringBuf();
    }

    if (chunk.GetLexType() == HTLEX_END_TAG) {
        Y_ASSERT(chunk.leng >= 3);
        Y_ASSERT(chunk.text[1] == '/');

        return TStringBuf(chunk.text + 2, chunk.leng - 3);
    } else {
        const char* text = chunk.text + 1;
        ui32 leng = chunk.leng - 2;

        for (const char* c = text; c != text + leng; ++c) {
            if (isspace(*c) || *c == '/') {
                leng = c - text;
                break;
            }
        }

        return TStringBuf(text, leng);
    }
}

TStringBuf GetTagAttributeValue(const THtmlChunk& chunk,
                                const TStringBuf& attrName) {
    Y_ASSERT(chunk.GetLexType() == HTLEX_EMPTY_TAG ||
             chunk.GetLexType() == HTLEX_START_TAG);

    for (size_t i = 0; i < chunk.AttrCount; ++i) {
        const auto& attr = chunk.Attrs[i];

        const TStringBuf name(chunk.text + attr.Name.Start, attr.Name.Leng);

        if (AsciiEqualsIgnoreCase(name, attrName)) {
            if (attr.IsBoolean()) {
                return TStringBuf();
            }
            return TStringBuf(chunk.text + attr.Value.Start, attr.Value.Leng);
        }
    }
    return TStringBuf();
}

///////////////////////////////////////////////////////////////////////////////
