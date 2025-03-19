#pragma once

#include "lemma_quality.h"

#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/dictlib/grambitset.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NSymbol {

struct IYemmaIterator {
    virtual bool Ok() const = 0;
    virtual void operator++() = 0;
    virtual const TYandexLemma& operator*() const = 0;

    virtual ~IYemmaIterator() {
    }
};

typedef TAutoPtr<IYemmaIterator> TYemmaIteratorPtr;

struct ILemmas {
    virtual ~ILemmas() {}

    virtual size_t GetLemmaCount() const = 0;
    virtual TWtringBuf GetLemmaText(size_t lemma) const = 0;
    virtual ELanguage GetLemmaLang(size_t lemma) const = 0;
    virtual TGramBitSet GetStemGram(size_t lemma) const = 0;
    virtual size_t GetFlexGramCount(size_t lemma) const = 0;
    virtual TGramBitSet GetFlexGram(size_t lemma, size_t gram) const = 0;
    virtual ELemmaQuality GetQuality(size_t lemma) const = 0;

    virtual TYemmaIteratorPtr GetYemmas() const {
        return TYemmaIteratorPtr();
    }
};

} // NSymbol

Y_DECLARE_OUT_SPEC(inline, NSymbol::ILemmas, output, lemmas) {
    output << "lemmas " << lemmas.GetLemmaCount() << Endl;
    for (size_t l = 0; l < lemmas.GetLemmaCount(); ++l) {
        output << lemmas.GetLemmaText(l)
            << " " << NameByLanguage(lemmas.GetLemmaLang(l))
            << " " << static_cast<ui32>(lemmas.GetQuality(l))
            << " [" << lemmas.GetStemGram(l).ToString() << "]"
            << " (";
        for (size_t f = 0; f < lemmas.GetFlexGramCount(l); ++f) {
            output << (f ? "," : "") << "[" << lemmas.GetFlexGram(l, f).ToString() << "]";
        }
        output << ")" << Endl;
    }
}
