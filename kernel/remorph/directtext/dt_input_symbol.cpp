#include <kernel/search_types/search_types.h>
#include "dt_input_symbol.h"

#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/core/lemmeraux.h>

#include <library/cpp/langs/langs.h>
#include <util/charset/unidata.h>

namespace NDT {

using namespace NSymbol;
using namespace NIndexerCore;

namespace {

const wchar16 SUBTOKEN_DELIMS[] = {'-', '\'', 0x60, 0x2019};
const TWtringBuf APOS = TWtringBuf(SUBTOKEN_DELIMS + 1, Y_ARRAY_SIZE(SUBTOKEN_DELIMS) - 1);

// LemmatizedToken array is ordered by the FormOffset field.
// Returns last LemmatizedToken item offset + 1
inline size_t SubTokenCount(const TDirectTextEntry2& entry) {
    return entry.LemmatizedTokenCount > 0
        ? entry.LemmatizedToken[entry.LemmatizedTokenCount - 1].FormOffset + 1
        : 0;
}

} // unnamed namespace

class TDTYemmaIterator: public IYemmaIterator {
private:
    const TDTInputSymbol& Symbol;
    size_t LemmaIndex;
    TYandexLemma Yemma;

private:
    void ConstructYemma() {
        const TLemmatizedToken& token = Symbol.TextEntry->LemmatizedToken[Symbol.Lemmas[LemmaIndex].LemmaIndex];
        NLemmerAux::TYandexLemmaSetter lemmaSetter(Yemma);
        lemmaSetter.SetLanguage(static_cast<ELanguage>(token.Lang));
        lemmaSetter.SetLemma(Symbol.LemmaBuffer.data() + Symbol.Lemmas[LemmaIndex].LemmaTxtOffset,
            Symbol.Lemmas[LemmaIndex].LemmaTxtLen, 0, 0, 0, token.StemGram);
        lemmaSetter.SetFlexGrs(token.FlexGrams, token.GramCount);
        lemmaSetter.SetQuality(token.IsBastard ? TYandexLemma::QBastard : TYandexLemma::QDictionary);
        lemmaSetter.SetInitialForm(Symbol.Text.data(), Symbol.Text.size());
        lemmaSetter.SetNormalizedForm(Symbol.NormalizedText.data(), Symbol.NormalizedText.size());
    }

public:
    explicit TDTYemmaIterator(const TDTInputSymbol& symbol)
        : Symbol(symbol)
        , LemmaIndex(0)
    {
        if (Ok()) {
            ConstructYemma();
        }
    }

    bool Ok() const override {
        return LemmaIndex < Symbol.Lemmas.size();
    }

    void operator++() override {
        ++LemmaIndex;
        if (Ok()) {
            ConstructYemma();
        }
    }

    const TYandexLemma& operator*() const override {
        Y_ASSERT(Ok());
        return Yemma;
    }

    ~TDTYemmaIterator() override {
    }
};

TDTInputSymbol::TDTInputSymbol()
    : TInputSymbol()
    , TextEntry(nullptr)
    , Posting(0)
{
}

TDTInputSymbol::TDTInputSymbol(size_t pos, const TDirectTextEntry2& entry, const TLangMask& lang, const wchar16* text)
    : TInputSymbol(pos)
    , TextEntry(&entry)
    , Posting(entry.Posting)
{
    InitSymbol(lang, text);
}

TDTInputSymbol::TDTInputSymbol(size_t pos, const wchar16 punct, TPosting dtPosting)
    : TInputSymbol(pos)
    , TextEntry(nullptr)
    , Posting(dtPosting)
{
    Text.AssignNoAlias(&punct, 1);
    NormalizedText = Text;
    Properties.Set(PROP_PUNCT);
}


TDTInputSymbol::TDTInputSymbol(TDTInputSymbols::const_iterator start, TDTInputSymbols::const_iterator end,
    const TVector<TDynBitMap>& ctxs, size_t mainOffset)
    : TInputSymbol(start, end, ctxs, mainOffset)
    , TextEntry(nullptr)
    , Posting(Y_LIKELY(start != end) ? start->Get()->GetPosting() : 0)
{
}

size_t TDTInputSymbol::GetLemmaCount() const {
    return Lemmas.size();
}

TWtringBuf TDTInputSymbol::GetLemmaText(size_t lemma) const {
    Y_ASSERT(lemma < Lemmas.size());
    return TWtringBuf(LemmaBuffer, Lemmas[lemma].LemmaTxtOffset, Lemmas[lemma].LemmaTxtLen);
}

ELanguage TDTInputSymbol::GetLemmaLang(size_t lemma) const {
    Y_ASSERT(lemma < Lemmas.size());
    Y_ASSERT(Lemmas[lemma].LemmaIndex < TextEntry->LemmatizedTokenCount);
    return static_cast<ELanguage>(TextEntry->LemmatizedToken[Lemmas[lemma].LemmaIndex].Lang);
}

TGramBitSet TDTInputSymbol::GetStemGram(size_t lemma) const {
    Y_ASSERT(lemma < Lemmas.size());
    Y_ASSERT(Lemmas[lemma].LemmaIndex < TextEntry->LemmatizedTokenCount);
    return TGramBitSet::FromBytes(TextEntry->LemmatizedToken[Lemmas[lemma].LemmaIndex].StemGram);
}

size_t TDTInputSymbol::GetFlexGramCount(size_t lemma) const {
    Y_ASSERT(lemma < Lemmas.size());
    Y_ASSERT(Lemmas[lemma].LemmaIndex < TextEntry->LemmatizedTokenCount);
    return TextEntry->LemmatizedToken[Lemmas[lemma].LemmaIndex].GramCount;
}

TGramBitSet TDTInputSymbol::GetFlexGram(size_t lemma, size_t gram) const {
    Y_ASSERT(lemma < Lemmas.size());
    Y_ASSERT(Lemmas[lemma].LemmaIndex < TextEntry->LemmatizedTokenCount);
    Y_ASSERT(gram < TextEntry->LemmatizedToken[Lemmas[lemma].LemmaIndex].GramCount);
    return TGramBitSet::FromBytes(TextEntry->LemmatizedToken[Lemmas[lemma].LemmaIndex].FlexGrams[gram]);
}

ELemmaQuality TDTInputSymbol::GetQuality(size_t lemma) const {
    Y_ASSERT(lemma < Lemmas.size());
    Y_ASSERT(Lemmas[lemma].LemmaIndex < TextEntry->LemmatizedTokenCount);
    return TextEntry->LemmatizedToken[Lemmas[lemma].LemmaIndex].IsBastard ? LQ_BASTARD : LQ_GOOD;
}

TYemmaIteratorPtr TDTInputSymbol::GetYemmas() const {
    return TYemmaIteratorPtr(new TDTYemmaIterator(*this));
}

void TDTInputSymbol::Reset() {
    TInputSymbol::Reset();
    TextEntry = nullptr;
    Lemmas.clear();
    LemmaBuffer.clear();
    Posting = 0;
}

void TDTInputSymbol::InitSymbol(const TLangMask& lang, const wchar16* text) {
    LemmaSelector.Set(*this);

    Text.AssignNoAlias(text ? text : TextEntry->Token);
    NormalizedText = Text;
    NormalizedText.to_lower();

    InitLemmas(lang);
    InitProps();
}

void TDTInputSymbol::InitLemmas(const TLangMask& lang) {
    // We explicitly init LangMask during lemmas traversing
    LangInitialized = true;

    bool hasAffix = false;
    if (lang.Test(LANG_TUR)) {
        const size_t aposPos = NormalizedText.find_first_of(APOS);
        if (TUtf16String::npos != aposPos && aposPos > 0) {
            NormalizedText.remove(aposPos);
            hasAffix = true;
        }
    }

    if (0 == TextEntry->LemmatizedTokenCount)
        return;

    const size_t subTokenCount = SubTokenCount(*TextEntry);
    ui8 lastSubToken = TextEntry->LemmatizedToken[TextEntry->LemmatizedTokenCount - 1].FormOffset;
    if (hasAffix && lastSubToken > 0) {
        --lastSubToken;
    }

    wchar16 buff[MAXKEY_BUF];
    bool hasFullLemma = false;
    // Don't reserve too much. Limit it for 5 lemmas.
    LemmaBuffer.reserve(MAXKEY_BUF * Min(TextEntry->LemmatizedTokenCount, ui32(5)));

    TUtf16String prefix;
    const size_t pos = NormalizedText.find_last_of(SUBTOKEN_DELIMS, TUtf16String::npos, Y_ARRAY_SIZE(SUBTOKEN_DELIMS));
    if (TUtf16String::npos != pos) {
        prefix = NormalizedText.substr(0, pos + 1); // Include last delimiter
    }

    for (ui32 i = 0; i < TextEntry->LemmatizedTokenCount; ++i) {
        const TLemmatizedToken& token = TextEntry->LemmatizedToken[i];
        const ELanguage lemmaLang = static_cast<ELanguage>(token.Lang);

        // Filter lemmas by language
        if (!lang.Empty() && !lang.SafeTest(lemmaLang))
            continue;

        // Detect the full lemma. It starts from the zero position and has no joins from the right
        const bool full = (1 == subTokenCount
            || (0 == token.FormOffset
                && (0 == (FORM_HAS_JOINS & token.Flags) || 0 == (FORM_RIGHT_JOIN & token.Joins))));

        // Don't process partial lemma if we has full one.
        // For partial lemmas use only last sub-token.
        if (!full && (hasFullLemma || lastSubToken != token.FormOffset))
            continue;

        if (full && !hasFullLemma) {
            // All previous lemmas are partial. Clear them.
            hasFullLemma = true;
            Lemmas.clear();
            LemmaBuffer.clear();
            LangMask.Reset();
            prefix.clear();
        }

        // Use normalized form from either first full lemma or from first partial lemma of the last sub-token
        // In case of partial lemmas, the prefix will be appended later
        if (Lemmas.empty() && !hasAffix) {
            NormalizedText.assign(prefix).AppendNoAlias(buff, NIndexerCore::ConvertKeyText(token.FormaText, buff));
        }

        const size_t buffOffset = LemmaBuffer.length();
        const size_t lemmaLength = NIndexerCore::ConvertKeyText(token.LemmaText, buff);
        LemmaBuffer.AppendNoAlias(prefix).AppendNoAlias(buff, lemmaLength);
        Lemmas.push_back(TLemmaInfo(buffOffset, prefix.length() + lemmaLength, i));

        LangMask.SafeSet(lemmaLang);
    }
}

void TDTInputSymbol::InitProps() {
    TInputSymbol::SetProperties(NLemmer::ClassifyCase(Text.data(), Text.size()));
    if (SubTokenCount(*TextEntry) > 1 ||
        Text.find_first_of(TWtringBuf(SUBTOKEN_DELIMS, Y_ARRAY_SIZE(SUBTOKEN_DELIMS))) != TWtringBuf::npos)
    {
        size_t chunkCount = 0;
        const wchar16* chunkStart = Text.data();
        const wchar16* const end = Text.data() + Text.size();
        while (chunkStart != end) {
            size_t chunkLength = 0;
            // Find continues alpha-chunk
            while (chunkStart + chunkLength != end && ::IsAlnum(chunkStart[chunkLength])) {
                ++chunkLength;
            }
            if (chunkLength != 0) {
                const TCharCategory cc = NLemmer::ClassifyCase(chunkStart, chunkLength);
                TInputSymbol::UpdateCompoundProps(cc, 0 == chunkCount);
                ++chunkCount;
            }
            chunkStart += chunkLength;
            // Skip non-alpha symbols
            while (chunkStart != end && !::IsAlnum(*chunkStart)) {
                ++chunkStart;
            }
        }
    }
}

void TDTInputSymbol::AssignWord(size_t pos, const TDirectTextEntry2& entry, const TLangMask& lang, const wchar16* text) {
    Reset();
    TInputSymbol::SetSourcePos(pos);
    TextEntry = &entry;
    Posting = entry.Posting;
    InitSymbol(lang, text);
}

void TDTInputSymbol::AssignPunct(size_t pos, const wchar16 punct, TPosting dtPosting) {
    Reset();
    TInputSymbol::SetSourcePos(pos);
    Text.AssignNoAlias(&punct, 1);
    NormalizedText = Text;
    Posting = dtPosting;
    Properties.Set(PROP_PUNCT);
}

void TDTInputSymbol::AssignMultiSymbol(TDTInputSymbols::const_iterator start, TDTInputSymbols::const_iterator end,
    const TVector<TDynBitMap>& ctxs, size_t mainOffset) {
    // Reset() method is called inside of the TInputSymbol::AssignMultiSymbol()
    TInputSymbol::AssignMultiSymbol(start, end, ctxs, mainOffset);
    Posting = Y_LIKELY(start != end) ? start->Get()->GetPosting() : 0;
}

} // NDT



