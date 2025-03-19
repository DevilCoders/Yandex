#pragma once

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/indexer/direct_text/dt.h>

#include <kernel/lemmer/dictlib/grambitset.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/deprecated/iter/vector.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <kernel/search_types/search_types.h>
#include <util/system/yassert.h>
#include <utility>

using namespace NIndexerCore;

namespace NGzt {

namespace NPrivate {

class TLemmatizedTokenIter {
private:
    const TLemmatizedToken* Beg;
    const TLemmatizedToken* End;
    wchar16 LemmaBuffer[MAXKEY_BUF];
    size_t LemmaLen;

private:
    void SetLemma() {
        if (Ok()) {
            LemmaLen = ConvertKeyText(GetToken().LemmaText, LemmaBuffer);
        }
    }

public:
    TLemmatizedTokenIter()
        : Beg(nullptr)
        , End(nullptr)
        , LemmaLen(0)
    {
    }

    TLemmatizedTokenIter(const TLemmatizedToken* tokens, size_t count)
        : Beg(tokens)
        , End(tokens + count)
        , LemmaLen(0)
    {
        SetLemma();
    }

    inline bool Ok() const {
        return Beg < End;
    }

    inline void operator++() {
        ++Beg;
        SetLemma();
    }

    inline const TLemmatizedToken& GetToken() const {
        Y_ASSERT(Ok());
        return *Beg;
    }

    inline TWtringBuf GetLemma() const {
        return TWtringBuf(LemmaBuffer, LemmaLen);
    }
};

class TLemmatizedTokenGramIter {
public:
    TLemmatizedTokenGramIter()
        : StemGram(nullptr)
    {
    }

    TLemmatizedTokenGramIter(const TLemmatizedToken& token)
        : StemGram(token.StemGram)
        , StemGramBitSet(TGramBitSet::FromBytes(StemGram))
        , FlexGram(token.FlexGrams, token.GramCount)
    {
        // when there is no @FlexGram, the iterator still will do a single iteration over @StemGram
        if (!FlexGram.Ok() && StemGramBitSet.any())
            FlexGram.Reset(&StemGram, &StemGram + 1);
    }

    inline bool Ok() const {
        return FlexGram.Ok();
    }

    inline void operator++() {
        ++FlexGram;
    }

    // return a COPY instead of ref
    typedef TGramBitSet TValueRef;

    inline TGramBitSet operator*() const {
        return StemGramBitSet | TGramBitSet::FromBytes(*FlexGram);
    }

private:
    typedef const char* TGramStr;

    TGramStr StemGram;
    TGramBitSet StemGramBitSet;
    NIter::TVectorIterator<const TGramStr> FlexGram;
};

} // NPrivate

class TDTGztInput {
private:
    const TDirectTextEntry2* Entries;
    size_t Count;
    TUtf16String TextBuffer;
    TVector< std::pair<size_t, size_t> > NormPos;
    TVector< std::pair<size_t, size_t> > ExtraPos;

private:
    std::pair<size_t, size_t> CalcNormalizedText(const TDirectTextEntry2& entry);
    std::pair<size_t, size_t> CalcExtraForm(const TDirectTextEntry2& entry);
    void CalcTexts(const TLangMask& langs);

public:
    TDTGztInput(const TDirectTextEntry2* entries, size_t count, const TLangMask& langs = LANG_UNK)
        : Entries(entries)
        , Count(count)
    {
        CalcTexts(langs);
        Y_ASSERT(Count == NormPos.size());
    }

    inline const TDirectTextEntry2& operator[] (size_t i) const {
        Y_ASSERT(i < Count);
        return Entries[i];
    }

    inline TWtringBuf GetNormalizedText(size_t i) const {
        Y_ASSERT(i < Count);
        return TWtringBuf(TextBuffer, NormPos[i].first, NormPos[i].second);
    }

    inline bool HasExtraForm(size_t i) const {
        Y_ASSERT(i < Count);
        return i < ExtraPos.size() && ExtraPos[i].second > 0;
    }

    inline TWtringBuf GetExtraForm(size_t i) const {
        Y_ASSERT(i < Count);
        Y_ASSERT(i < ExtraPos.size());
        return TWtringBuf(TextBuffer, ExtraPos[i].first, ExtraPos[i].second);
    }

    inline size_t size() const {
        return Count;
    }
};

typedef TWordIterator<TDTGztInput> TDTGztInputIterator;

template<> class TWordIteratorTraits<TDTGztInput> {
public:
    typedef wchar16 TChar;
    typedef TWtringBuf TWordString;
    typedef NIter::TValueIterator<const TWordString> TExactFormIter;
    typedef NGzt::NPrivate::TLemmatizedTokenIter TLemmaIter;
    typedef NGzt::NPrivate::TLemmatizedTokenGramIter TGrammemIter;
    typedef TWideCharTester TCharTester;
};

template<>
inline TDTGztInputIterator::TWordString TDTGztInputIterator::GetWordString(size_t word_index) const {
    return Input->GetNormalizedText(word_index);
}

template<>
inline TDTGztInputIterator::TWordString TDTGztInputIterator::GetOriginalWordString(size_t word_index) const {
    return (*Input)[word_index].Token;
}

template<>
inline TDTGztInputIterator::TLemmaIter TDTGztInputIterator::IterLemmas(size_t word_index) const {
    return TDTGztInputIterator::TLemmaIter((*Input)[word_index].LemmatizedToken, (*Input)[word_index].LemmatizedTokenCount);
}

template<>
inline TDTGztInputIterator::TWordString TDTGztInputIterator::GetLemmaString(const TLemmaIter& iter) {
    Y_ASSERT(iter.Ok());
    return iter.GetLemma();
}

template<>
inline TDTGztInputIterator::TGrammemIter TDTGztInputIterator::IterGrammems(const TLemmaIter& iter) {
    Y_ASSERT(iter.Ok());
    return TDTGztInputIterator::TGrammemIter(iter.GetToken());
}

template<>
inline ELanguage TDTGztInputIterator::GetLanguage(const TLemmaIter& iter) {
    Y_ASSERT(iter.Ok());
    return static_cast<ELanguage>(iter.GetToken().Lang);
}

// Iterate over other possible exact forms
template<>
inline TDTGztInputIterator::TExactFormIter TDTGztInputIterator::IterExtraForms(size_t word_index) const {
    return Input->HasExtraForm(word_index)
        ? TDTGztInputIterator::TExactFormIter(Input->GetExtraForm(word_index))
        : TDTGztInputIterator::TExactFormIter();
}

} // NGzt
