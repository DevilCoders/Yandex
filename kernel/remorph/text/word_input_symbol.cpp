#include "word_input_symbol.h"

#include <kernel/remorph/input/lemma_quality.h>

#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/dictlib/grambitset.h>

namespace NText {

inline void ToLangLower(TUtf16String& text, const TLangMask& lang) {
    for (ELanguage l : lang) {
        NLemmer::GetAlphaRules(l)->ToLower(text);
        return;
    }
    text.to_lower();
}

class TWordYemmaIterator: public IYemmaIterator {
private:
    const TWLemmaArray& Yemmas;
    TWLemmaArray::const_iterator Yemma;

public:
    explicit TWordYemmaIterator(const TWLemmaArray& yemmas)
        : Yemmas(yemmas)
        , Yemma(Yemmas.begin())
    {
    }

    bool Ok() const override {
        return Yemma != Yemmas.end();
    }

    void operator++() override {
        ++Yemma;
    }

    const TYandexLemma& operator*() const override {
        Y_ASSERT(Ok());
        return *Yemma;
    }

    ~TWordYemmaIterator() override {
    }
};

TWordInputSymbol::TWordInputSymbol(size_t pos, const TWideToken& text, const TLangMask& lang, bool withMorphology, bool useDisamb)
    : TInputSymbol(pos)
{
    InitSymbol(text, lang, withMorphology, useDisamb);
    SetWideTokenFlags(text);
}

TWordInputSymbol::TWordInputSymbol(size_t pos, const TUtf16String& text)
    : TInputSymbol(pos)
{
    InitSymbol(text);
    Properties.Set(PROP_PUNCT);
}

TWordInputSymbol::TWordInputSymbol(const TWideToken& text, const TLangMask& lang, bool withMorphology, bool useDisamb)
    : TInputSymbol()
{
    InitSymbol(text, lang, withMorphology, useDisamb);
    SetWideTokenFlags(text);
}

TWordInputSymbol::TWordInputSymbol(const TUtf16String& text)
    : TInputSymbol()
{
    InitSymbol(text);
    Properties.Set(PROP_PUNCT);
}

TWordInputSymbol::TWordInputSymbol(TWordSymbols::const_iterator start, TWordSymbols::const_iterator end, const TVector<TDynBitMap>& ctxs, size_t mainOffset)
    : TInputSymbol(start, end, ctxs, mainOffset)
{
    InitMultiSymbol();
    InitWordMultiSymbol();
}

void TWordInputSymbol::InitSymbol(const TWideToken& text, const TLangMask& lang, bool withMorphology, bool useDisamb) {
    Text.AssignNoAlias(text.Token, text.Leng);

    if (lang.Test(LANG_TUR) && text.SubTokens.size() > 1
        && TOKDELIM_APOSTROPHE == text.SubTokens[text.SubTokens.size() - 2].TokenDelim) {

        TWideToken tmpTok(text);
        tmpTok.SubTokens.pop_back();
        tmpTok.Leng = tmpTok.SubTokens.back().EndPos() + tmpTok.SubTokens.back().SuffixLen;

        NormalizedText.AssignNoAlias(tmpTok.Token, tmpTok.SubTokens.back().EndPos());
        ToLangLower(NormalizedText, lang);
        if (withMorphology) {
            InitLemmas(tmpTok, lang, false, useDisamb);
        }
    } else if (withMorphology) {
        InitLemmas(text, lang, true, useDisamb);
    } else {
        NormalizedText = Text;
        ToLangLower(NormalizedText, lang);
    }
}

void TWordInputSymbol::InitSymbol(const TUtf16String& text) {
    NormalizedText = Text = text;
    ToLangLower(NormalizedText, LANG_UNK);
}

void TWordInputSymbol::InitLemmas(const TWideToken& text, const TLangMask& lang, bool normText, bool useDisamb) {
    if (text.SubTokens.size() > 1) {
        LemmatizeMultiToken(text, lang);
    } else {
        // AnalyzeWord() clears Lemmas before processing, so don't clear it here
        NLemmer::AnalyzeWord(text, Lemmas, lang);
    }
    if (normText) {
        if (!Lemmas.empty()) {
            NormalizedText.AssignNoAlias(Lemmas[0].GetNormalizedForm(), Lemmas[0].GetNormalizedFormLength());
        } else {
            NormalizedText = Text;
            ToLangLower(NormalizedText, lang);
        }
    }
    if (useDisamb && !Lemmas.empty())
        Lemmas.resize(1);  // TODO: use context-dependent disambiguator

    LemmaSelector.Set(*this);
}

//lemmatize only last part and each lemma to original prefix
void TWordInputSymbol::LemmatizeMultiToken(const TWideToken& text, const TLangMask& lang) {

    const TCharSpan& lastSubToken = text.SubTokens.back();
    const size_t lastSubTokenPos = text.SubTokens.size() - 1;

    TUtf16String prefix(text.Token, lastSubToken.Pos);
    ToLangLower(prefix, lang);

    TWLemmaArray lemmas;
    NLemmer::AnalyzeWord(text, lemmas, lang);
    bool hasFullLemma = false;
    for (size_t i = 0 ; i < lemmas.size(); ++i) {
        const bool full = lemmas[i].GetTokenPos() == 0 && lemmas[i].GetTokenSpan() == text.SubTokens.size();

        // Don't process partial lemma if we has full one.
        // For partial lemmas use only last sub-token.
        if (full) {
            if (!hasFullLemma) {
                // All previous lemmas are partial. Clear them.
                hasFullLemma = true;
                Lemmas.clear();
            }
            Lemmas.push_back(lemmas[i]);
        } else if (!hasFullLemma && lemmas[i].GetTokenPos() == lastSubTokenPos) {
            TYandexLemma newLemma = lemmas[i];
            NLemmerAux::TYandexLemmaSetter lemmaSetter(newLemma);
            prefix.append(newLemma.GetText(), newLemma.GetTextLength());
            lemmaSetter.SetLemma(prefix.data(), prefix.size(), 0, newLemma.GetPrefLen(), newLemma.GetFlexLen(), newLemma.GetStemGram());
            prefix.remove(lastSubToken.Pos).append(newLemma.GetNormalizedForm(), newLemma.GetNormalizedFormLength());
            lemmaSetter.SetNormalizedForm(prefix.data(), prefix.size());
            lemmaSetter.SetToken(0, text.SubTokens.size());
            Lemmas.push_back(newLemma);
        }
    }
}

void TWordInputSymbol::InitWordMultiSymbol() {
    SentencePos.first = static_cast<const TWordInputSymbol*>(Children.front().Get())->GetSentencePos().first;
    SentencePos.second = static_cast<const TWordInputSymbol*>(Children.back().Get())->GetSentencePos().second;
}

void TWordInputSymbol::AssignWord(size_t pos, const TWideToken& text, const TLangMask& lang, bool withMorphology, bool useDisamb) {
    Reset();

    TInputSymbol::SetSourcePos(pos);
    InitSymbol(text, lang, withMorphology, useDisamb);
    SetWideTokenFlags(text);
}

void TWordInputSymbol::AssignWord(const TWideToken& text, const TLangMask& lang, bool withMorphology, bool useDisamb) {
    Reset();

    TInputSymbol::SetSourcePos(std::pair<size_t, size_t>(-1, -1));
    InitSymbol(text, lang, withMorphology, useDisamb);
    SetWideTokenFlags(text);
}

void TWordInputSymbol::AssignPunct(size_t pos, const TUtf16String& text) {
    Reset();

    TInputSymbol::SetSourcePos(pos);
    InitSymbol(text);
    Properties.Set(PROP_PUNCT);
}

void TWordInputSymbol::AssignPunct(const TUtf16String& text) {
    Reset();

    TInputSymbol::SetSourcePos(std::pair<size_t, size_t>(-1, -1));
    InitSymbol(text);
    Properties.Set(PROP_PUNCT);
}

// Assigns multi-symbol
void TWordInputSymbol::AssignMultiSymbol(TWordSymbols::const_iterator start, TWordSymbols::const_iterator end, const TVector<TDynBitMap>& ctxs, size_t mainOffset) {
    // Reset() method is called inside of the TInputSymbol::AssignMultiSymbol()
    TInputSymbol::AssignMultiSymbol(start, end, ctxs, mainOffset);
    InitSymbol(TUtf16String());
    InitWordMultiSymbol();
}

void TWordInputSymbol::Reset() {
    TInputSymbol::Reset();
    SentencePos.first = SentencePos.second = 0;
}

size_t TWordInputSymbol::GetLemmaCount() const {
    return GetLemmaArray().size();
}

TWtringBuf TWordInputSymbol::GetLemmaText(size_t lemma) const {
    Y_ASSERT(lemma < GetLemmaArray().size());
    const TYandexLemma& l = GetLemmaArray()[lemma];
    return TWtringBuf(l.GetText(), l.GetTextLength());
}

ELanguage TWordInputSymbol::GetLemmaLang(size_t lemma) const {
    Y_ASSERT(lemma < GetLemmaArray().size());
    const TYandexLemma& l = GetLemmaArray()[lemma];
    return l.GetLanguage();
}

TGramBitSet TWordInputSymbol::GetStemGram(size_t lemma) const {
    Y_ASSERT(lemma < GetLemmaArray().size());
    const TYandexLemma& l = GetLemmaArray()[lemma];
    return TGramBitSet::FromBytes(l.GetStemGram());
}

size_t TWordInputSymbol::GetFlexGramCount(size_t lemma) const {
    Y_ASSERT(lemma < GetLemmaArray().size());
    const TYandexLemma& l = GetLemmaArray()[lemma];
    return l.FlexGramNum();
}

TGramBitSet TWordInputSymbol::GetFlexGram(size_t lemma, size_t gram) const {
    Y_ASSERT(lemma < GetLemmaArray().size());
    const TYandexLemma& l = GetLemmaArray()[lemma];
    Y_ASSERT(gram < l.FlexGramNum());
    return TGramBitSet::FromBytes(l.GetFlexGram()[gram]);
}

ELemmaQuality TWordInputSymbol::GetQuality(size_t lemma) const {
    Y_ASSERT(lemma < GetLemmaArray().size());
    return NSymbol::LemmaQualityFromLemmerQualityMask(GetLemmaArray()[lemma].GetQuality());
}

NSymbol::TYemmaIteratorPtr TWordInputSymbol::GetYemmas() const {
    return NSymbol::TYemmaIteratorPtr(new TWordYemmaIterator(GetLemmaArray()));
}

void TWordInputSymbol::SetWideTokenFlags(const TWideToken& text) {
    // Treat multitoken as a usual token for properties calculation.
    SetProperties(NLemmer::ClassifyCase(text.Token, text.Leng));

    // ...But not for case properties. They are calculated on subtoken basis.
    if (text.SubTokens.size() > 1) {
        for (size_t i = 0; i < text.SubTokens.size(); ++i) {
            TCharCategory cc = NLemmer::ClassifyCase(text.Token + text.SubTokens[i].Pos, text.SubTokens[i].Len);
            UpdateCompoundProps(cc, 0 == i);
        }
    }
}

} // NText
