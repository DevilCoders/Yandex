#pragma once

#include <library/cpp/token/token_structure.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NToken {

struct TTokenInfo {
    size_t TokenOffset;         // Offset in reconstructed sentence text
    size_t OriginalOffset;      // Offset in original sentence
    size_t Length;              // Token length
    bool SpaceBefore;           // Is any space chars before the current token in the sentence?
    TTokenStructure SubTokens;  // Sub-tokens
    TUtf16String Punctuation;         // Text of punctuation tokens

    TTokenInfo()
        : TokenOffset(0)
        , OriginalOffset(0)
        , Length(0)
        , SpaceBefore(false)
    {
    }

    // Fill normal token
    void Fill(size_t tokenOffset, size_t originalOffset, bool spaceBefore, const TWideToken& token) {
        TokenOffset = tokenOffset;
        OriginalOffset = originalOffset;
        Length = token.Leng;
        SpaceBefore = spaceBefore;
        SubTokens = token.SubTokens;
    }

    // Fill punctuation
    void Fill(size_t originalOffset, bool spaceBefore, const TUtf16String& token) {
        OriginalOffset = originalOffset;
        Punctuation = token;
        SpaceBefore = spaceBefore;
        SubTokens.clear();
    }

    inline bool IsNormalToken() const {
        return 0 != Length;
    }

    // Should be called only for normal tokens
    TWideToken ToWideToken(const wchar16* text) const {
        TWideToken res;
        res.Token = text + TokenOffset;
        res.Leng = Length;
        res.SubTokens = SubTokens;
        return res;
    }
};

struct TSentenceInfo {
    std::pair<size_t, size_t> Pos;
    size_t SentenceNum;
    TUtf16String Text;
    TVector<TTokenInfo> Tokens;

    TSentenceInfo()
        : Pos(0, 0)
        , SentenceNum(0)
    {
    }
};

struct ITokenizerCallback {
    virtual void OnSentence(const TSentenceInfo& sentInfo) = 0;

    virtual ~ITokenizerCallback()
    {
    }
};

} // NToken
