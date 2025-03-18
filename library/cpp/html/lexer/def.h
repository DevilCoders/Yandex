#pragma once

#include <util/system/defaults.h>
#include <library/cpp/html/spec/lextype.h>
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/spec/attrs.h>

namespace NHtmlLexer {
    struct TToken {
        HTLEX_TYPE Type;  ///< Text or markup
        const char* Text; ///< Pointer to original text
        unsigned Leng;    ///< token length in bytes

        const unsigned char* Tag; ///< in source case! see HTag // NB: to offset
        unsigned TagLen;
        const NHtml::TTag* HTag; ///< for tags - set to tag definition instance

        unsigned AttStart; ///< for tag opens - offset of first attribute in external attribute array
        unsigned NAtt;     ///< for tag opens - number of attributes
        const NHtml::TAttribute* Attrs;
        bool IsWhitespace; ///< for text - "is whitespace only" token

        TToken() {
        }

        TToken(HTLEX_TYPE type, const unsigned char* text, unsigned leng,
               const unsigned char* tag, unsigned tagLen, const NHtml::TTag* hTag,
               unsigned attStart, unsigned nAtt, bool isWhite)
            : Type(type)
            , Text((const char*)text)
            , Leng(leng)
            , Tag(tag)
            , TagLen(tagLen)
            , HTag(hTag)
            , AttStart(attStart)
            , NAtt(nAtt)
            , Attrs(nullptr)
            , IsWhitespace(isWhite)
        {
        }
    };

}
