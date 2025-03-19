#pragma once

#include <kernel/remorph/common/source.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NReMorph {

enum EParseTokenType {
    PTT_UNKNOWN     /* "unknown" */,
    PTT_ID          /* "id" */,
    PTT_SEMICOLON   /* "semicolon" */,
    PTT_EQUALS      /* "equals" */,
    PTT_LPAREN      /* "lparen" */,
    PTT_RPAREN      /* "rparen" */,
    PTT_LBRACKET    /* "lbracket" */,
    PTT_RBRACKET    /* "rbracket" */,
    PTT_NUMBER      /* "number" */,
    PTT_STRING      /* "string" */,
    PTT_REGEX       /* "regex" */,
    PTT_REGEX_MOD   /* "regex-mod" */,
    PTT_GROUP       /* "group" */,
};

enum EParseTokenDataType {
    PTDT_NONE,
    PTDT_DATA,
    PTDT_DATA_UNESCAPED,
    PTDT_RAW_DATA,
    PTDT_NUMBER,
};

struct TParseTokenDataFormat {
    size_t ExtraOffset;
    TVector<size_t> ExtraOffsetMap;

    explicit TParseTokenDataFormat()
        : ExtraOffset(0)
    {
    }

    size_t GetExtraOffset(size_t pos) const;
};

struct TParseToken {
    EParseTokenType Type;
    EParseTokenDataType DataType;
    TUtf16String Data;
    TString RawData;
    double Number;
    TSourceLocation Location;
    TParseTokenDataFormat DataFormat;

    explicit TParseToken()
        : Type(PTT_UNKNOWN)
        , DataType(PTDT_NONE)
    {
    }

    explicit TParseToken(EParseTokenType parseTokenType)
        : Type(parseTokenType)
        , DataType(PTDT_DATA)
    {
    }

    explicit TParseToken(EParseTokenType parseTokenType, EParseTokenDataType parseTokenDataType)
        : Type(parseTokenType)
        , DataType(parseTokenDataType)
    {
    }
};

typedef TVector<TParseToken> TParseTokens;

struct TParseStatement {
    TString Head;
    TParseTokens Body;
    TSourceLocation Location;
};

} // NReMorph

Y_DECLARE_OUT_SPEC(inline, NReMorph::TParseToken, output, parseToken) {
    output << parseToken.Type;
    switch (parseToken.DataType) {
    case NReMorph::PTDT_NONE:
        break;
    case NReMorph::PTDT_DATA:
    case NReMorph::PTDT_DATA_UNESCAPED:
        output << ":" << parseToken.Data.Quote();
        break;
    case NReMorph::PTDT_RAW_DATA:
        output << ":" << parseToken.RawData.Quote();
        break;
    case NReMorph::PTDT_NUMBER:
        output << ":" << parseToken.Number;
        break;
    }
}

Y_DECLARE_OUT_SPEC(inline, NReMorph::TParseStatement, output, parseStatement) {
    output << parseStatement.Head << " (";
    bool first = true;
    for (NReMorph::TParseTokens::const_iterator iToken = parseStatement.Body.begin(); iToken != parseStatement.Body.end(); ++iToken) {
        if (!first) {
            output << " ";
        } else {
            first = false;
        }
        output << *iToken;
    }
    output << ")";
}
