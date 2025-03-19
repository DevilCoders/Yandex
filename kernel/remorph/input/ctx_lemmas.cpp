#include "ctx_lemmas.h"

#include "input_symbol.h"

#include <kernel/lemmer/core/lemmer.h>

namespace NSymbol {

class TCtxYemmaIterator: public IYemmaIterator {
private:
    const TCtxLemmas& CtxLemmas;
    TWLemmaArray OrigYemmas;
    TVector<size_t>::const_iterator LemmaMapIndex;

public:
    explicit TCtxYemmaIterator(const TCtxLemmas& ctxLemmas)
        : CtxLemmas(ctxLemmas)
    {
        Y_ASSERT(ctxLemmas.Lemmas);
        TYemmaIteratorPtr origYemmaIteratorPtr = CtxLemmas.Lemmas->GetYemmas();
        Y_ASSERT(origYemmaIteratorPtr.Get());
        for (IYemmaIterator& origYemmaIterator = *origYemmaIteratorPtr; origYemmaIterator.Ok(); ++origYemmaIterator) {
            OrigYemmas.push_back(*origYemmaIterator);
        }
        LemmaMapIndex = CtxLemmas.LemmaMap.begin();
    }

    bool Ok() const override {
        return LemmaMapIndex != CtxLemmas.LemmaMap.end();
    }

    void operator++() override {
        ++LemmaMapIndex;
    }

    const TYandexLemma& operator*() const override {
        Y_ASSERT(Ok());
        return OrigYemmas[*LemmaMapIndex];
    }

    ~TCtxYemmaIterator() override
    {};
};

void TCtxLemmas::Set(const TInputSymbol& s, const TDynBitMap& ctx) {
    Lemmas = &s.GetLemmas();
    LemmaMap.clear();
    FlexGramMap.clear();
    FillMapping(s.GetGztArticles().size(), ctx);
}

void TCtxLemmas::FillMapping(size_t offset, const TDynBitMap& ctx) {
    const bool ctxEmpty = (offset ? ctx.NextNonZeroBit(offset - 1) : ctx.FirstNonZeroBit()) >= ctx.Size();
    if (ctxEmpty) {
        return;
    }

    const size_t lemmaCount = Lemmas->GetLemmaCount();

    size_t pos = 0;
    for (size_t iLemma = 0; iLemma < lemmaCount; ++iLemma) {
        const size_t gramCount = Lemmas->GetFlexGramCount(iLemma);
        if (gramCount > 0) {
            bool hasGram = false;
            for (size_t iGram = 0; iGram < gramCount; ++iGram) {
                if (ctx.Test(offset + pos + iGram)) {
                    hasGram = true;
                    FlexGramMap.push_back(std::make_pair(LemmaMap.size(), iGram));
                }
            }
            if (hasGram) {
                LemmaMap.push_back(iLemma);
            }
            pos += gramCount;
        } else if (!Lemmas->GetStemGram(iLemma).Empty()) {
            if (ctx.Test(offset + pos)) {
                LemmaMap.push_back(iLemma);
            }
            ++pos;
        }
    }
}

size_t TCtxLemmas::GetLemmaCount() const {
    return Y_UNLIKELY(LemmaMap.empty()) ? Lemmas->GetLemmaCount() : LemmaMap.size();
}

TWtringBuf TCtxLemmas::GetLemmaText(size_t lemma) const {
    if (Y_UNLIKELY(LemmaMap.empty()))
        return Lemmas->GetLemmaText(lemma);
    Y_ASSERT(lemma < LemmaMap.size());
    Y_ASSERT(LemmaMap[lemma] < Lemmas->GetLemmaCount());
    return Lemmas->GetLemmaText(LemmaMap[lemma]);
}

ELanguage TCtxLemmas::GetLemmaLang(size_t lemma) const {
    if (Y_UNLIKELY(LemmaMap.empty()))
        return Lemmas->GetLemmaLang(lemma);
    Y_ASSERT(lemma < LemmaMap.size());
    Y_ASSERT(LemmaMap[lemma] < Lemmas->GetLemmaCount());
    return Lemmas->GetLemmaLang(LemmaMap[lemma]);
}

TGramBitSet TCtxLemmas::GetStemGram(size_t lemma) const {
    if (Y_UNLIKELY(LemmaMap.empty()))
        return Lemmas->GetStemGram(lemma);
    Y_ASSERT(lemma < LemmaMap.size());
    Y_ASSERT(LemmaMap[lemma] < Lemmas->GetLemmaCount());
    return Lemmas->GetStemGram(LemmaMap[lemma]);
}

size_t TCtxLemmas::GetFlexGramCount(size_t lemma) const {
    if (Y_UNLIKELY(LemmaMap.empty()))
        return Lemmas->GetFlexGramCount(lemma);
    return FlexGramMap.count(lemma);
}

TGramBitSet TCtxLemmas::GetFlexGram(size_t lemma, size_t gram) const {
    if (Y_UNLIKELY(LemmaMap.empty()))
        return Lemmas->GetFlexGram(lemma, gram);
    Y_ASSERT(lemma < LemmaMap.size());
    Y_ASSERT(LemmaMap[lemma] < Lemmas->GetLemmaCount());
    return Lemmas->GetFlexGram(LemmaMap[lemma], (FlexGramMap.LowerBound(lemma) + gram)->second);
}

ELemmaQuality TCtxLemmas::GetQuality(size_t lemma) const {
    if (Y_UNLIKELY(LemmaMap.empty()))
        return Lemmas->GetQuality(lemma);
    Y_ASSERT(lemma < LemmaMap.size());
    Y_ASSERT(LemmaMap[lemma] < Lemmas->GetLemmaCount());
    return Lemmas->GetQuality(LemmaMap[lemma]);
}

TYemmaIteratorPtr TCtxLemmas::GetYemmas() const {
    if (Y_UNLIKELY(LemmaMap.empty()))
        return Lemmas->GetYemmas();
    return TYemmaIteratorPtr(new TCtxYemmaIterator(*this));
}

} // namespace NSymbol
