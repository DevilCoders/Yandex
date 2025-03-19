#pragma once

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol.h>
#include <kernel/remorph/input/gazetteer_input.h>
#include <kernel/remorph/input/lemmas.h>

#include <kernel/indexer/direct_text/dt.h>

#include <library/cpp/wordpos/wordpos.h>

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <utility>

namespace NDT {

class TDTInputSymbol;
typedef TIntrusivePtr<TDTInputSymbol> TDTInputSymbolPtr;
typedef TVector<TDTInputSymbolPtr> TDTInputSymbols;
typedef NRemorph::TInput<TDTInputSymbolPtr> TDTInput;

class TDTYemmaIterator;

class TDTInputSymbol: public NSymbol::TInputSymbol, private NSymbol::ILemmas {
private:
    const NIndexerCore::TDirectTextEntry2* TextEntry;
    struct TLemmaInfo {
        TLemmaInfo(size_t offset, size_t len, size_t index)
            : LemmaTxtOffset(offset)
            , LemmaTxtLen(len)
            , LemmaIndex(index)
        {
        }
        size_t LemmaTxtOffset;
        size_t LemmaTxtLen;
        size_t LemmaIndex; // lemma index in TextEntry lemma array
    };
    TVector<TLemmaInfo> Lemmas;
    TUtf16String LemmaBuffer; // Contains text of all lemmas
    TPosting Posting; // Contains index position of symbol

private:
    // ILemmas implementation
    size_t GetLemmaCount() const override;
    TWtringBuf GetLemmaText(size_t lemma) const override;
    ELanguage GetLemmaLang(size_t lemma) const override;
    TGramBitSet GetStemGram(size_t lemma) const override;
    size_t GetFlexGramCount(size_t lemma) const override;
    TGramBitSet GetFlexGram(size_t lemma, size_t gram) const override;
    NSymbol::ELemmaQuality GetQuality(size_t lemma) const override;
    NSymbol::TYemmaIteratorPtr GetYemmas() const override;

    void InitSymbol(const TLangMask& lang, const wchar16* text);
    void InitLemmas(const TLangMask& lang);
    void InitProps();

public:
    TDTInputSymbol();
    TDTInputSymbol(size_t pos, const NIndexerCore::TDirectTextEntry2& entry, const TLangMask& lang = TLangMask(), const wchar16* text = nullptr);
    TDTInputSymbol(size_t pos, const wchar16 punct, TPosting dtPosting);
    TDTInputSymbol(TDTInputSymbols::const_iterator start, TDTInputSymbols::const_iterator end,
        const TVector<TDynBitMap>& ctxs, size_t mainOffset);

    void AssignWord(size_t pos, const NIndexerCore::TDirectTextEntry2& entry, const TLangMask& lang = TLangMask(), const wchar16* text = nullptr);
    void AssignPunct(size_t pos, const wchar16 punct, TPosting dtPosting);
    void AssignMultiSymbol(TDTInputSymbols::const_iterator start, TDTInputSymbols::const_iterator end,
        const TVector<TDynBitMap>& ctxs, size_t mainOffset);

    inline TPosting GetPosting() const {
        return Posting;
    }

    inline const NIndexerCore::TDirectTextEntry2* GetOriginalToken() const {
        const TInputSymbol* const head = GetHead();
        return nullptr != head ? static_cast<const TDTInputSymbol*>(head)->TextEntry : TextEntry;
    }

    inline bool IsAssigned() const {
        return nullptr != TextEntry || !Text.empty();
    }

    inline bool IsPunct() const {
        return nullptr == TextEntry;
    }

    void Reset() override;

    friend class TDTYemmaIterator;
};

} // NDT

inline TString ToString(const NDT::TDTInputSymbol& s) {
    return WideToUTF8(s.GetText());
}

inline TUtf16String ToWtroku(const NDT::TDTInputSymbol& s) {
    return s.GetText();
}

DECLARE_GAZETTEER_SUPPORT(NDT::TDTInputSymbols, TDTInputSymbolsIterator)
