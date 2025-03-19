#pragma once

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/wordinstance.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <library/cpp/deprecated/iter/iter.h>

#include <kernel/gazetteer/worditerator.h>

#include "hashcache.h"

namespace NGzt
{

// Declared in this file:
class TSimpleWord;
class TSimpleText;


// Simplified version of TWordInstance: only stores set of lemmas and original word text
// This is all gazetteer currently needs to make a search.
class TSimpleWord
{
public:
    TSimpleWord(const TWideToken& text, const TLangMask& lang = LANG_RUS);
    TSimpleWord(const TUtf16String& text);    // word without lemmas

    // Re-initilize methods
    void Set(const TWideToken& text, const TLangMask& lang = LANG_RUS);
    void Set(const TUtf16String& text);

    inline TWtringBuf GetOriginalText() const {
        return OriginalText;
    }

    inline TWtringBuf GetNormalizedText() const {
        return NormalizedText;
    }

    inline const TWLemmaArray& GetLemmas() const {
        return Lemmas;
    }

    inline void SetPosition(size_t pos) {
        StartPosInText = pos;
    }

    // Returns start position of word in wide symbols in original text.
    inline size_t GetPosition() const {
        return StartPosInText;
    }

private:
    void SimpleNormalize();
    bool LemmatizeMultiToken(const TWideToken& text, TLangMask lang);

private:
    size_t StartPosInText;

    TUtf16String OriginalText;
    TUtf16String NormalizedTextHolder;
    TWtringBuf NormalizedText;      // points either to NormalizedTextHolder buffer or to Lemmas[0].GetNormalizedForm()

    TWLemmaArray Lemmas;
};




// A simple class for tokenizing and lemmatizing of raw text (TUtf16String)
// which could be used to run gazetteer search on it.
// You should use it only if you do not have already some of
// arcadia standard word-collection classes (TRichTree, TVector<TWordInstance>, etc.)
// where pre-processing is already done (as it can take quite a lot of time).

class TSimpleText: public ITokenHandler
{
public:
    enum EMultiTokenSplit {
        None,   // Don't split multi-tokens
        Wizard, // Split multi-tokens in the same way as wizard does
        Index   // Split multi-tokens in the same way as indexer does
    };

    // lang - language(s) for lemmatization
    // split - multi-token split method
    // usePunct - use punctuation symbols as separate tokens
    TSimpleText(TLangMask lang = LANG_RUS, EMultiTokenSplit split = Wizard, bool usePunct = false);

    // text - text to parse
    // lang - language(s) for lemmatization
    // split - multi-token split method
    // usePunct - use punctuation symbols as separate tokens
    TSimpleText(const TUtf16String& text, TLangMask lang = LANG_RUS, EMultiTokenSplit split = Wizard, bool usePunct = false);

    // TSimpleText instance could be re-used for many gzt searches.
    // This is more efficient than constructing it in a loop repeatedly.
    void Reset(const TUtf16String& text);

    // enables local morphology cache
    void InitCache(size_t cacheSizeLimit = 1000);

    inline size_t size() const {
        return Words.size();
    }

    const TSimpleWord& operator [] (size_t index) const {
        return *Words[index];
    }

    TWtringBuf GetNormalizedWord(size_t index) const {
        return Words[index]->GetNormalizedText();
    }

    TWtringBuf GetOriginalWord(size_t index) const {
        return Words[index]->GetOriginalText();
    }

    inline const TWLemmaArray& GetWordLemmas(size_t index) const {
        return Words[index]->GetLemmas();
    }

    const TUtf16String& GetOriginalText() const {
        return OriginalText;
    }

    TUtf16String DebugString() const {
        TUtf16String ret;
        for (size_t i = 0; i < Words.size(); ++i)
            ret.append('[').append(Words[i]->GetOriginalText()).append(']');
        return ret;
    }

private:
    friend class ::TNlpTokenizer;

    // implements ITokenHandler
    void OnToken(const TWideToken& tok, size_t, NLP_TYPE type) override;

    void PutToken(const TWideToken& tok);
    void PutDelims(const TWtringBuf& punct, bool isBreak, size_t offset = 0);
    void AddWord(const TWideToken& word, size_t offset);
    void AddWord(const TUtf16String& word, size_t offset);
    bool CheckSplit(const TWideToken& word, size_t pos) const;
    void DoAddWord(const TWideToken& tok, size_t offset);


private:
    TUtf16String OriginalText;
    size_t TokenStartPos;

    TNlpTokenizer Tokenizer;
    TVector< TSimpleSharedPtr<TSimpleWord> > Words;
    TLangMask Languages;
    EMultiTokenSplit MultiTokenSplit; // Multi-token split method
    bool UsePunct; // Use punctuation symbols as separate tokens

    THashCache<TWtringBuf, size_t> Cache; // string to the index of already added word
};


// TWordIterator specialization for NGzt::TSimpleText
typedef TWordIterator<NGzt::TSimpleText> TSimpleTextIterator;
template <> class TWordIteratorTraits<NGzt::TSimpleText> {
public:
    typedef wchar16 TChar;
    typedef TWtringBuf TWordString;
    typedef NIter::TValueIterator<const TWordString> TExactFormIter;
    typedef NIter::TVectorIterator<const TYandexLemma> TLemmaIter;
    typedef TYandexLemmaGramIter TGrammemIter;
    typedef TWideCharTester TCharTester;
};

// TWordIterator specialization for NGzt::TSimpleText
template <> inline TSimpleTextIterator::TWordString TSimpleTextIterator::GetWordString(size_t word_index) const {
    return Input->GetNormalizedWord(word_index);
}

template <> inline TSimpleTextIterator::TWordString TSimpleTextIterator::GetOriginalWordString(size_t word_index) const {
    return Input->GetOriginalWord(word_index);
}

template <> inline TSimpleTextIterator::TLemmaIter TSimpleTextIterator::IterLemmas(size_t word_index) const {
    return TLemmaIter(Input->GetWordLemmas(word_index));
}

template <> inline TSimpleTextIterator::TWordString TSimpleTextIterator::GetLemmaString(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return TWtringBuf(lemma->GetText(), lemma->GetTextLength());
}

template <> inline TSimpleTextIterator::TGrammemIter TSimpleTextIterator::IterGrammems(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return TYandexLemmaGramIter(*lemma);
}

template <> inline ELanguage TSimpleTextIterator::GetLanguage(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return lemma->GetLanguage();
}

} // namespace NGzt
