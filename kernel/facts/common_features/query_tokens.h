#pragma once

#include <kernel/lemmer/core/lemmer.h>

#include <library/cpp/tokenizer/split.h>
#include <library/cpp/tokenizer/tokenizer.h>

namespace NFacts {
    struct TTypedLemmedToken {
        TUtf16String Word;
        NLP_TYPE Type;
        TWLemmaArray Lemmas;
        TYandexLemma BestLemma;

        TTypedLemmedToken(const TWtringBuf& word, const NLP_TYPE& type) :
                Word(TUtf16String(word)),
                Type(type),
                BestLemma(GetBestLemma(Word, Type, Lemmas))
        {}

    private:
        //NLP_TYPE integer and float are being ignored
        static TYandexLemma GetBestLemma(const TUtf16String& word, const NLP_TYPE& nlp_type, TWLemmaArray& lemmas);
    };


    class TTypedLemmedTokenHandler : public ITokenHandler {
    public:
        TTypedLemmedTokenHandler(TVector<TTypedLemmedToken>* outTokens, const TTokenizerSplitParams& params)
                : Tokens(outTokens)
                , Params(params) {
            Y_ASSERT(outTokens != nullptr);
        }

        void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override {
            if (!Params.HandledMask.SafeTest(type)) {
                return;
            }

            Tokens->emplace_back(token.Text(), type);
        }

        TVector<TWLemmaArray> GetAllLemmas() {
            TVector <TWLemmaArray> res;
            if (Tokens == nullptr)
                return res;
            for (const auto& token : *Tokens) {
                res.push_back(token.Lemmas);
            }
            return res;
        }

    private:
        TVector<TTypedLemmedToken>* Tokens;
        TTokenizerSplitParams Params;
    };


    TUtf16String SubstNumericTokenOrGetLemma(const TTypedLemmedToken& token);
}
