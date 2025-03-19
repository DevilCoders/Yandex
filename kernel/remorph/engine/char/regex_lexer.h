#pragma once

#include "regex_token.h"

#include <kernel/remorph/common/source.h>
#include <kernel/remorph/engine/engine.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NReMorph {
namespace NPrivate {

class TRegexLexer {
private:
    typedef unsigned int TAlphType;

private:
    const TWtringBuf& OrigData;
    const TParseTokenDataFormat& DataFormat;
    TVector<TAlphType> Data;
    TVector<const wchar16*> OrigDataMap;
    const TSourceLocation& Location;

    const TAlphType* SetBegin;
    size_t NestedSets;

    TVector<int> Stack;

    int cs;
    int* stack;
    int top;
    int act;
    const TAlphType* ts;
    const TAlphType* te;
    const TAlphType* p;
    const TAlphType* pe;
    const TAlphType* eof;

public:
    explicit TRegexLexer(const TWtringBuf& data, const TSourceLocation& location, const TParseTokenDataFormat& dataFormat);

    TRegexToken GetToken();

private:
    void PrepareData();
    TRegexToken YieldToken(ERegexTokenType type, const TAlphType* pos, const TAlphType* posEnd);
    TRegexToken YieldToken(ERegexTokenType type);
    TRegexToken YieldToken(ERegexTokenType type, TAlphType symbol, const TAlphType* pos, const TAlphType* posEnd);
    TRegexToken YieldToken(ERegexTokenType type, const TAlphType* dataBegin, size_t dataSize, const TAlphType* pos, const TAlphType* posEnd);
    const wchar16* GetOrigData(const TAlphType* p) const;
    TWtringBuf GetOrigData(const TAlphType* dataBegin, size_t dataSize) const;
    void AddTokenLocation(TRegexToken& token, const TAlphType* pos, const TAlphType* posEnd);
    void Reset();
    TSourceLocation GetLocation(const TAlphType* p) const;
    TLexingError Error() const;
    TLexingError Error(const TAlphType* p) const;
};

} // NPrivate
} // NReMorph
