#pragma once

#include "token_types.h"

#include <kernel/remorph/tokenizer/tokenizer.h>

#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/containers/comptrie/comptrie_trie.h>

#include <util/charset/unidata.h>


namespace NAddressFilter {

class TTokenizerOpts {
public:
    TString TriePath;
};

struct IAddressFilterTokenizerCallback {
    virtual void OnText(const TUtf16String& sentenceText, const TVector<NToken::TTokenInfo>& tokens,  const TVector<TTokenType>& tokenTypes) = 0;

    virtual ~IAddressFilterTokenizerCallback() {
    }
};


class TTokenizer : public NToken::ITokenizerCallback {
public:
    TTokenizer(TTokenizerOpts opts, IAddressFilterTokenizerCallback& cb)
        : Callback(cb),
          TokenizeOptions(NToken::TF_NO_SENTENCE_SPLIT, NToken::BD_DEFAULT,  NToken::MS_SMART, Max()),
          Tokenizer(*this, TokenizeOptions)
    {
        Trie.Init(TBlob::FromFile(opts.TriePath));
    }

    void ProcessLine(const TUtf16String& text) {
        TUtf16String wText;
        Tokenizer.Tokenize(text);
    }

    void OnSentence(const NToken::TSentenceInfo& sentence) override;

private:
    bool IsCapital(const TWtringBuf& token) const;
    bool IsNumber(const NToken::TTokenInfo& token) const;

    TTokenType GetTokenType(const NToken::TTokenInfo& tokenInfo, const TWtringBuf& token) const ;

private:
    TCompactTrie<wchar16>     Trie;
    IAddressFilterTokenizerCallback&    Callback;
    NToken::TTokenizeOptions            TokenizeOptions;
    NToken::TTokenizer                  Tokenizer;
};

} //NAddressFilter

