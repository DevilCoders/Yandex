#include "worditerator.h"

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>

namespace NGzt
{



// TWordIterator specialization for TVector<TUtf16String>
template <> TVectorWtrokIterator::TWordString TVectorWtrokIterator::GetWordString(size_t word_index) const {
    return (*Input)[word_index];
}

template <> TVectorWtrokIterator::TLemmaIter TVectorWtrokIterator::IterLemmas(size_t /*word_index*/) const {
    // no lemmata - TVector<TUtf16String> should only be used for EXACT_FORM search
    return TLemmaIter();
}

template <> TVectorWtrokIterator::TWordString TVectorWtrokIterator::GetLemmaString(const TLemmaIter& /*lemma*/) {
    // no lemmata
    return TWordString();
}

template <> TVectorWtrokIterator::TGrammemIter TVectorWtrokIterator::IterGrammems(const TLemmaIter& /*lemma*/) {
    // no lemmata
    return TGrammemIter();
}


// TWordIterator specialization for vector of TWordInstance*
template <> TWordInstanceIterator::TWordString TWordInstanceIterator::GetWordString(size_t word_index) const {
    return (*Input)[word_index]->GetNormalizedForm();
}

template <> TWordInstanceIterator::TLemmaIter TWordInstanceIterator::IterLemmas(size_t word_index) const {
    const TWordInstance* w = (*Input)[word_index];
    return TLemmaIter(w->LemsBegin(), w->LemsEnd());
}

template <> TWordInstanceIterator::TWordString TWordInstanceIterator::GetLemmaString(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return lemma->GetLemma();
}

template <> TWordInstanceIterator::TGrammemIter TWordInstanceIterator::IterGrammems(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return TGrammemIter(lemma->GetStemGrammar(), lemma->GetFlexGrammars());
}


} // namespace NGzt

