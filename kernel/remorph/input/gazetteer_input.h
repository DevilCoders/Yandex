#pragma once

#include "lemmas.h"

#include <kernel/gazetteer/worditerator.h>
#include <library/cpp/deprecated/iter/value.h>

namespace NSymbol {

class TLemmaIter {
private:
    const ILemmas* Lemmas;
    size_t Current;
    size_t Count;

public:
    TLemmaIter()
        : Lemmas(nullptr)
        , Current(0)
        , Count(0)
    {
    }

    TLemmaIter(const ILemmas& lemmas)
        : Lemmas(&lemmas)
        , Current(0)
        , Count(lemmas.GetLemmaCount())
    {
    }

    Y_FORCE_INLINE const ILemmas& GetLemmas() const {
        Y_ASSERT(Ok());
        return *Lemmas;
    }

    Y_FORCE_INLINE bool Ok() const {
        return Current < Count;
    }

    Y_FORCE_INLINE void operator++() {
        ++Current;
    }

    Y_FORCE_INLINE size_t operator*() const {
        return Current;
    }
};

class TGrammemIter {
private:
    const ILemmas* Lemmas;
    size_t LemmaIndex;
    size_t GramIndex;
    size_t GramCount;
    TGramBitSet StemGramBitSet;
public:
    // return a COPY instead of ref
    typedef TGramBitSet TValueRef;

    TGrammemIter()
        : Lemmas(nullptr)
        , LemmaIndex(0)
        , GramIndex(0)
        , GramCount(0)
    {
    }

    TGrammemIter(const ILemmas& lemmas, size_t index)
        : Lemmas(&lemmas)
        , LemmaIndex(index)
        , GramIndex(0)
    {
        Y_ASSERT(index < lemmas.GetLemmaCount());
        StemGramBitSet = Lemmas->GetStemGram(LemmaIndex);
        GramCount = Lemmas->GetFlexGramCount(LemmaIndex);
    }

    Y_FORCE_INLINE bool Ok() const {
        return GramIndex < GramCount || (0 == GramIndex && !StemGramBitSet.Empty());
    }

    Y_FORCE_INLINE void operator++() {
        ++GramIndex;
    }

    inline TGramBitSet operator*() const {
        Y_ASSERT(Ok());
        if (GramIndex < GramCount)
            return StemGramBitSet | Lemmas->GetFlexGram(LemmaIndex, GramIndex);
        else if (0 == GramIndex && !StemGramBitSet.Empty())
            return StemGramBitSet;
        else
            return TGramBitSet();
    }
};

} // NSymbol

#define DECLARE_GAZETTEER_SUPPORT(SymbolsType, IterName) \
    \
    namespace NGzt { \
    \
    typedef NGzt::TWordIterator<SymbolsType> IterName; \
    \
    template<> class TWordIteratorTraits<SymbolsType> { \
    public: \
        typedef wchar16 TChar; \
        typedef TWtringBuf TWordString; \
        typedef NIter::TValueIterator<const TWordString> TExactFormIter; \
        typedef NSymbol::TLemmaIter TLemmaIter; \
        typedef NSymbol::TGrammemIter TGrammemIter; \
        typedef TWideCharTester TCharTester; \
    }; \
    template<> inline IterName::TWordString IterName::GetWordString(size_t word_index) const { \
        return (*Input)[word_index]->GetNormalizedText(); \
    } \
    template<> inline IterName::TWordString IterName::GetOriginalWordString(size_t word_index) const { \
        return (*Input)[word_index]->GetText(); \
    } \
    template<> inline IterName::TLemmaIter IterName::IterLemmas(size_t word_index) const { \
        return IterName::TLemmaIter((*Input)[word_index]->GetLemmas()); \
    } \
    template<> inline IterName::TWordString IterName::GetLemmaString(const TLemmaIter& iter) { \
        Y_ASSERT(iter.Ok()); \
        return iter.GetLemmas().GetLemmaText(*iter); \
    } \
    template<> inline IterName::TGrammemIter IterName::IterGrammems(const TLemmaIter& iter) { \
        Y_ASSERT(iter.Ok()); \
        return IterName::TGrammemIter(iter.GetLemmas(), *iter); \
    } \
    template<> inline ELanguage IterName::GetLanguage(const TLemmaIter& iter) { \
        Y_ASSERT(iter.Ok()); \
        return iter.GetLemmas().GetLemmaLang(*iter); \
    } \
    } // NGzt
