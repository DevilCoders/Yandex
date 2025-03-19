#pragma once

#include "lemmas.h"

#include <kernel/lemmer/core/lemmer.h>

#include <library/cpp/containers/sorted_vector/sorted_vector.h>
#include <util/generic/bitmap.h>
#include <util/generic/vector.h>

namespace NSymbol {

class TInputSymbol;
class TCtxYemmaIterator;

class TCtxLemmas: public ILemmas {
protected:
    const ILemmas* Lemmas;                              // Non-restricted lemmas.
    TVector<size_t> LemmaMap;                           // Contextually restricted lemmas mapping.
    NSorted::TSimpleMap<size_t, size_t> FlexGramMap;    // Contextually restricted flexemes mapping.

protected:
    void FillMapping(size_t offset, const TDynBitMap& ctx);

public:
    TCtxLemmas()
        : Lemmas(nullptr)
    {
    }

    TCtxLemmas(const TInputSymbol& s, const TDynBitMap& ctx)
        : Lemmas(nullptr)
    {
        Set(s, ctx);
    }

    void Set(const TInputSymbol& s, const TDynBitMap& ctx);
    void Reset() {
        Lemmas = nullptr;
        LemmaMap.clear();
        FlexGramMap.clear();
    }

    inline const ILemmas* GetSrcLemmas() const {
        return Lemmas;
    }

    size_t GetLemmaCount() const override;
    TWtringBuf GetLemmaText(size_t lemma) const override;
    ELanguage GetLemmaLang(size_t lemma) const override;
    TGramBitSet GetStemGram(size_t lemma) const override;
    size_t GetFlexGramCount(size_t lemma) const override;
    TGramBitSet GetFlexGram(size_t lemma, size_t gram) const override;
    ELemmaQuality GetQuality(size_t lemma) const override;
    TYemmaIteratorPtr GetYemmas() const override;

    friend class TCtxYemmaIterator;
};

} // namespace NSymbol
