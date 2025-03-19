#pragma once

#include <kernel/remorph/common/source.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NReMorph {
namespace NPrivate {

enum ERegexTokenType {
    RTT_EOS             /* "eos" */,
    RTT_SYMBOL          /* "symbol" */,
    RTT_QUOTED_PAIR     /* "quoted-pair" */,
    RTT_SET             /* "set" */,
    RTT_ANY             /* "any" */,
    RTT_GROUP           /* "group" */,
    RTT_LPAREN          /* "lparen" */,
    RTT_RPAREN          /* "rparen" */,
    RTT_ALTERNATIVE     /* "alternative" */,
    RTT_ASTERISK        /* "asterisk" */,
    RTT_PLUS            /* "plus" */,
    RTT_QUESTION        /* "question" */,
    RTT_BEGIN           /* "begin" */,
    RTT_END             /* "end" */,
    RTT_REPEAT          /* "repeat" */,
    RTT_REFERENCE       /* "reference" */,
};

struct TRegexToken {
    ERegexTokenType Type;
    wchar32 Symbol;
    TWtringBuf Data;
    TSourceLocation Location;
    TSourceLocation LocationEnd;

    explicit TRegexToken()
        : Type(RTT_EOS)
        , Symbol(0)
        , Data()
    {
    }

    explicit TRegexToken(ERegexTokenType regexTokenType)
        : Type(regexTokenType)
        , Symbol(0)
        , Data()
    {
    }

    explicit TRegexToken(ERegexTokenType regexTokenType, wchar32 symbol)
        : Type(regexTokenType)
        , Symbol(symbol)
        , Data()
    {
    }

    explicit TRegexToken(ERegexTokenType regexTokenType, const TWtringBuf& data)
        : Type(regexTokenType)
        , Symbol(0)
        , Data(data)
    {
    }
};

} // NPrivate
} // NReMorph

Y_DECLARE_OUT_SPEC(inline, NReMorph::NPrivate::TRegexToken, output, regexToken) {
    output << regexToken.Type;
    if (regexToken.Symbol) {
        output << ":" << ::UTF32ToWide(&regexToken.Symbol, 1u).Quote();
    }
    if (!regexToken.Data.empty()) {
        output << ":" << TUtf16String(regexToken.Data).Quote();
    }
}
