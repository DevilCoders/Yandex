#pragma once

#include "errors.h"
#include "parse_token.h"

#include <kernel/remorph/common/source.h>

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <functional>
#include <util/stream/input.h>

namespace NReMorph {
namespace NPrivate {

using TAlphType = wchar32;

class TLexer {
public:
    typedef std::function<void(const TParseToken&)> TTokenHandler;

private:
    IInputStream& Input;
    TString Source;
    size_t BufferGrowthRate;
    size_t BufferChoppingPart;

    TBuffer LoadBuffer;
    size_t LoadCached;
    TVector<TAlphType> Buffer;
    size_t Cached;
    TSourcePos BufferOffset;

    TVector<TAlphType> UnescapedData;
    TVector<size_t> UnescapedPosMap;
    size_t UnescapedPosOffset;

    TVector<int> Stack;

    int cs;
    int* stack;
    int top;
    int act;
    const TAlphType* ts;
    const TAlphType* te;

public:
    explicit TLexer(IInputStream& input, const TString& source, size_t baseBufferSize = 0);

    void Scan(const TTokenHandler& tokenHandler);
    void SetBufferGrowthRate(size_t bufferGrowthRate);
    void SetBufferChoppingPart(size_t bufferChoppingPart);

    inline size_t GetBufferGrowthRate() const {
        return BufferGrowthRate;
    }

    inline size_t GetBufferChoppingPart() const {
        return BufferChoppingPart;
    }

private:
    size_t Load();
    void YieldToken(const TTokenHandler& tokenHandler, EParseTokenType type, const TAlphType* pos, EParseTokenDataType dataType = PTDT_NONE, const TAlphType* data = nullptr, size_t size = 0, size_t dataOffset = 0) const;
    TSourceLocation GetLocation(const TAlphType* p) const;
    TSourcePos GetPosOffset(const TAlphType* p) const;
    TLexingError Error(const TAlphType* p) const;
};

} // NPrivate
} // NReMorph
