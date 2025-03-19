#pragma once

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/gazetteer_input.h>
#include <kernel/remorph/input/input_symbol.h>
#include <kernel/remorph/input/lemmas.h>

#include <library/cpp/token/token_structure.h>
#include <kernel/lemmer/core/lemmer.h>

#include <util/generic/ptr.h>
#include <utility>

namespace NText {

using namespace NSymbol;

class TWordInputSymbol;
typedef TIntrusivePtr<TWordInputSymbol> TWordInputSymbolPtr;
typedef TVector<TWordInputSymbolPtr> TWordSymbols;
typedef NRemorph::TInput<TWordInputSymbolPtr> TWordInput;

class TWordInputSymbol: public TInputSymbol, private ILemmas {
protected:
    TWLemmaArray Lemmas;
    std::pair<size_t, size_t> SentencePos;

private:
    size_t GetLemmaCount() const override;

    TWtringBuf GetLemmaText(size_t lemma) const override;

    ELanguage GetLemmaLang(size_t lemma) const override;

    TGramBitSet GetStemGram(size_t lemma) const override;
    size_t GetFlexGramCount(size_t lemma) const override;
    TGramBitSet GetFlexGram(size_t lemma, size_t gram) const override;
    ELemmaQuality GetQuality(size_t lemma) const override;
    NSymbol::TYemmaIteratorPtr GetYemmas() const override;

    void SetWideTokenFlags(const TWideToken& text);

    void InitSymbol(const TWideToken& text, const TLangMask& lang, bool withMorphology, bool useDisamb);
    void InitSymbol(const TUtf16String& text);
    void InitWordMultiSymbol();

    void InitLemmas(const TWideToken& text, const TLangMask& lang, bool normText, bool useDisamb);
    void LemmatizeMultiToken(const TWideToken& text, const TLangMask& lang);

public:
    // Constructor for normal token in the specified position. Uses lemmer to get the grammar and case properties
    TWordInputSymbol(size_t pos, const TWideToken& text, const TLangMask& lang = LANG_RUS, bool withMorphology = true, bool useDisamb = false);
    // Constructor for non-text token (punctuation). The constructed symbol have no grammar and case properties
    TWordInputSymbol(size_t pos, const TUtf16String& text);
    // Constructor for normal token. Uses lemmer to get the grammar and case properties
    TWordInputSymbol(const TWideToken& text, const TLangMask& lang = LANG_RUS, bool withMorphology = true, bool useDisamb = false);
    // Constructor for non-text token (punctuation). The constructed symbol have no grammar and case properties
    TWordInputSymbol(const TUtf16String& text);

    // Multi-symbol constructor
    TWordInputSymbol(TWordSymbols::const_iterator start, TWordSymbols::const_iterator end, const TVector<TDynBitMap>& ctxs, size_t mainOffset);


    // Assigners. All assigners call Reset(), so you don't need to call it before assigning
    // Assigns normal token in the specified position. Uses lemmer to get the grammar and case properties
    void AssignWord(size_t pos, const TWideToken& text, const TLangMask& lang = LANG_RUS, bool withMorphology = true, bool useDisamb = false);
    // Assigns normal token. Uses lemmer to get the grammar and case properties
    void AssignWord(const TWideToken& text, const TLangMask& lang = LANG_RUS, bool withMorphology = true, bool useDisamb = false);
    // Assigns non-text token (punctuation). The constructed symbol have no grammar and case properties
    void AssignPunct(size_t pos, const TUtf16String& text);
    // Assigns non-text token (punctuation). The constructed symbol have no grammar and case properties
    void AssignPunct(const TUtf16String& text);
    // Assigns multi-symbol
    void AssignMultiSymbol(TWordSymbols::const_iterator start, TWordSymbols::const_iterator end, const TVector<TDynBitMap>& ctxs, size_t mainOffset);

    void Reset() override;

    // Returns symbol positions in a sentence
    Y_FORCE_INLINE const std::pair<size_t, size_t>& GetSentencePos() const {
        return SentencePos;
    }

    Y_FORCE_INLINE std::pair<size_t, size_t>& GetSentencePos() {
        return SentencePos;
    }

    inline const TWLemmaArray& GetLemmaArray() const {
        return Lemmas;
    }
};

} // NText

inline TString ToString(const NText::TWordInputSymbolPtr& s) {
    return s ? WideToUTF8(s->GetText()) : TString();
}

inline TUtf16String ToWtroku(const NText::TWordInputSymbolPtr& s) {
    return s ? s->GetText() : TUtf16String();
}

DECLARE_GAZETTEER_SUPPORT(NText::TWordSymbols, TWordSymbolsIterator)
