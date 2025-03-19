#pragma once

#include "callback.h"
#include "multitoken_split.h"

#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/token/token_structure.h>
#include <library/cpp/enumbitset/enumbitset.h>

#include <util/stream/file.h>
#include <util/stream/str.h>
#include <library/cpp/charset/wide.h>
#include <util/string/split.h>
#include <util/string/vector.h>

namespace NToken {

enum EBlockDetection {
    BD_NONE,
    BD_PER_LINE,
    BD_DEFAULT,
};

enum ETokenizeFlags {
    TF_NO_SENTENCE_SPLIT,
    TF_OFFSET_PER_BLOCK,

    TF_MAX
};

class TTokenizeOptions: private TSfEnumBitSet<ETokenizeFlags, TF_NO_SENTENCE_SPLIT, TF_MAX> {
private:
    typedef TSfEnumBitSet<ETokenizeFlags, TF_NO_SENTENCE_SPLIT, TF_MAX> TBase;

public:
    EBlockDetection BlockDetection;
    EMultitokenSplit MultitokenSplit;
    size_t MaxSentenceTokens;

public:
    TTokenizeOptions(EBlockDetection bd = BD_DEFAULT, EMultitokenSplit ms = MS_MINIMAL, size_t maxTokens = 100)
        : BlockDetection(bd)
        , MultitokenSplit(ms)
        , MaxSentenceTokens(maxTokens)
    {
    }

    TTokenizeOptions(ETokenizeFlags f1, EBlockDetection bd = BD_DEFAULT, EMultitokenSplit ms = MS_MINIMAL, size_t maxTokens = 100)
        : TBase(f1)
        , BlockDetection(bd)
        , MultitokenSplit(ms)
        , MaxSentenceTokens(maxTokens)
    {
    }

    TTokenizeOptions(ETokenizeFlags f1, ETokenizeFlags f2, EBlockDetection bd = BD_DEFAULT, EMultitokenSplit ms = MS_MINIMAL, size_t maxTokens = 100)
        : TBase(f1, f2)
        , BlockDetection(bd)
        , MultitokenSplit(ms)
        , MaxSentenceTokens(maxTokens)
    {
    }

    TTokenizeOptions(ETokenizeFlags f1, ETokenizeFlags f2, ETokenizeFlags f3, EBlockDetection bd = BD_DEFAULT, EMultitokenSplit ms = MS_MINIMAL, size_t maxTokens = 100)
        : TBase(f1, f2, f3)
        , BlockDetection(bd)
        , MultitokenSplit(ms)
        , MaxSentenceTokens(maxTokens)
    {
    }

    TTokenizeOptions& Set(ETokenizeFlags f) {
        TBase::SafeSet(f);
        return *this;
    }

    bool Has(ETokenizeFlags f) const {
        return TBase::SafeTest(f);
    }
};

class TTokenizer: public ITokenHandler {
private:
    friend class ::TNlpTokenizer;

protected:
    ITokenizerCallback& Callback;
    const TTokenizeOptions Opts;
    bool SpaceBefore;
    TSentenceInfo CurSentInfo;
    TUtf16String Block;
    size_t Offset;

protected:
    inline size_t GetCurrentSentencePos() const {
        return CurSentInfo.Pos.second - CurSentInfo.Pos.first;
    }

    inline void NextSentence() {
        CurSentInfo.Pos.first = CurSentInfo.Pos.second;
        CurSentInfo.Text.clear();
        CurSentInfo.Tokens.clear();
        SpaceBefore = false;
    }

    void OnToken(const TWideToken& tok, size_t len, NLP_TYPE type) override;

    void OnMiscToken(const TWideToken& tok);
    void OnNormalToken(const TWideToken& tok);
    void OnSentenceEnd(const TWideToken& tok);

    // Called for normal tokens
    void PutToken(const TWideToken& token);

    // Called for punctuation
    inline void PutToken(const TUtf16String& token) {
        CurSentInfo.Tokens.emplace_back();
        CurSentInfo.Tokens.back().Fill(GetCurrentSentencePos(), SpaceBefore, token);
        CurSentInfo.Pos.second += token.size();
        CurSentInfo.Text.append(token);
        SpaceBefore = false;
        CheckLimit();
    }

    void AddSubtokensRange(const TWideToken& tok, size_t iFirst, size_t iLast);

    void PutDelims(const TUtf16String& punct);

    void FlushTokens();

    inline void ResetSentence(size_t pos) {
        // Don't reset SentenceNum. Keep it straight-through
        CurSentInfo.Pos.first = CurSentInfo.Pos.second = pos;
        CurSentInfo.Text.clear();
        CurSentInfo.Tokens.clear();
    }

    static bool IsBlockStart(const TWtringBuf& line);

    inline void CheckLimit() {
        if (CurSentInfo.Tokens.size() >= Opts.MaxSentenceTokens) {
            FlushTokens();
        }
    }

public:
    TTokenizer(ITokenizerCallback& cb, const TTokenizeOptions& opts)
        : Callback(cb)
        , Opts(opts)
        , SpaceBefore(false)
        , Offset(0)
    {
    }

    void Tokenize(const TUtf16String& text, size_t textPosition = 0);
    void ConsumeLine(const TWtringBuf& line);

    inline void ConsumeLine(const TString& line, ECharset enc = CODES_UTF8) {
        ConsumeLine(CharToWide<true>(line, enc));
    }

    // Allow to participate in SplitString routine
    inline bool Consume(const wchar16* b, const wchar16* /*d*/, const wchar16* e) {
        ConsumeLine(TWtringBuf(b, e));
        return true;
    }

    void Flush();
};

class TWorkerPool;

// Tokenizes the stream and calls the specified callback for all detected sentences.
// The method doesn't close the stream.
void TokenizeStream(ITokenizerCallback& cb, IInputStream& input, ECharset enc, const TTokenizeOptions& opts,
    TWorkerPool* pool = nullptr);

// Tokenizes the file and calls the specified callback for all detected sentences.
inline void TokenizeFile(ITokenizerCallback& cb, const TString& file, ECharset enc, const TTokenizeOptions& opts,
    TWorkerPool* pool = nullptr) {

    TIFStream input(file);
    TokenizeStream(cb, input, enc, opts, pool);
}

inline void TokenizeText(ITokenizerCallback& cb, const TString& text, ECharset enc, const TTokenizeOptions& opts,
    TWorkerPool* pool = nullptr) {

    TStringInput input(text);
    TokenizeStream(cb, input, enc, opts, pool);
}

void TokenizeText(ITokenizerCallback& cb, const TWtringBuf& text, const TTokenizeOptions& opts,
    TWorkerPool* pool = nullptr);

} // NToken
