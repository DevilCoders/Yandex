#include "word_symbol_factory.h"

#include <kernel/remorph/tokenizer/tokenizer.h>

#include <util/generic/ylimits.h>
#include <util/generic/utility.h>

namespace NText {

TWordSymbols CreateWordSymbols(const NToken::TSentenceInfo& sentInfo, const TLangMask& lang, bool withMorphology, bool useDisamb) {
    TWordSymbols res;
    for (size_t i = 0; i < sentInfo.Tokens.size(); ++i) {
        const NToken::TTokenInfo& tokInfo = sentInfo.Tokens[i];
        TWordInputSymbolPtr symbol;
        if (tokInfo.IsNormalToken()) {
            symbol = new TWordInputSymbol(i, tokInfo.ToWideToken(sentInfo.Text.data()), lang, withMorphology, useDisamb);
            symbol->GetSentencePos() = std::make_pair(tokInfo.OriginalOffset, tokInfo.OriginalOffset + tokInfo.Length);
        } else {
            symbol = new TWordInputSymbol(i, tokInfo.Punctuation);
            symbol->GetSentencePos() = std::make_pair(tokInfo.OriginalOffset, tokInfo.OriginalOffset + tokInfo.Punctuation.size());
        }
        symbol->SetQLangMask(lang);
        if (tokInfo.SpaceBefore)
            symbol->GetProperties().Set(PROP_SPACE_BEFORE);
        res.push_back(symbol);
    }
    return res;
}

namespace {

struct TSymbolCollector: public NToken::ITokenizerCallback {
    TWordSymbols& Symbols;
    const TLangMask& Lang;
    const bool WithMorphology;
    const bool UseDisamb;

    TSymbolCollector(TWordSymbols& symbols, const TLangMask& l, bool withMorphology, bool useDisamb)
        : Symbols(symbols)
        , Lang(l)
        , WithMorphology(withMorphology)
        , UseDisamb(useDisamb)
    {
    }

    void OnSentence(const NToken::TSentenceInfo& sentInfo) override {
        TWordSymbols s = CreateWordSymbols(sentInfo, Lang, WithMorphology, UseDisamb);
        if (Y_LIKELY(Symbols.empty())) {
            DoSwap(Symbols, s);
        } else {
            Symbols.insert(Symbols.end(), s.begin(), s.end());
        }
    }
};

} // unnamed namespace

TWordSymbols CreateWordSymbols(const TUtf16String& text, const TLangMask& lang, NToken::EMultitokenSplit ms, bool withMorphology, bool useDisamb) {
    TWordSymbols symbols;
    TSymbolCollector sc(symbols, lang, withMorphology, useDisamb);
    NToken::TokenizeText(sc, text, NToken::TTokenizeOptions(NToken::TF_NO_SENTENCE_SPLIT, NToken::BD_DEFAULT, ms, Max<size_t>()));
    return symbols;
}

} // NText
