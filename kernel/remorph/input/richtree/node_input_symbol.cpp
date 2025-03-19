#include "node_input_symbol.h"

#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/input/lemma_quality.h>
#include <kernel/remorph/input/properties.h>

#include <kernel/qtree/richrequest/printrichnode.h>

#include <kernel/lemmer/core/wordinstance.h>

#include <util/charset/wide.h>
#include <util/string/vector.h>
#include <util/generic/algorithm.h>

static const TUtf16String SPACE = u" ";

namespace NReMorph {

namespace NRichNode {

class TNodeYemmaIterator: public NSymbol::IYemmaIterator {
private:
    const TWordInstance::TLemmasVector& Words;
    TWordInstance::TLemmasVector::const_iterator Word;

public:
    explicit TNodeYemmaIterator(const TWordInstance::TLemmasVector& words)
        : Words(words)
        , Word(Words.begin())
    {}

    bool Ok() const override {
        return Word != Words.end();
    }

    void operator++() override {
        ++Word;
    }

    const TYandexLemma& operator*() const override {
        Y_ASSERT(Ok() && Word->GetOriginalLemma());
        return *(Word->GetOriginalLemma());
    }

    ~TNodeYemmaIterator() override
    {};
};

TNodeInputSymbol::TNodeInputSymbol()
    : TInputSymbol()
    , Node(nullptr)
{
}

TNodeInputSymbol::TNodeInputSymbol(size_t pos, const TRichRequestNode* n)
    : TInputSymbol(pos)
    , Node(n)
{
    InitSymbol();
}

TNodeInputSymbol::TNodeInputSymbol(size_t pos, const wchar16 punct)
    : TInputSymbol(pos)
    , Node(nullptr)
{
    Text.AssignNoAlias(&punct, 1);
    NormalizedText = Text;
    Properties.Set(NSymbol::PROP_PUNCT);
}

TNodeInputSymbol::TNodeInputSymbol(TNodeInputSymbols::const_iterator start, TNodeInputSymbols::const_iterator end, const TVector<TDynBitMap>& contexts, ui32 mainWordIndex)
    : TInputSymbol(start, end, contexts, mainWordIndex)
    , Node(nullptr)
{
}

void TNodeInputSymbol::InitSymbol() {
    Text = NormalizedText = Node->GetText();
    Y_ASSERT(Node->WordInfo.Get());
    if (Node->WordInfo->IsLemmerWord()) {
        SetProperties(Node->WordInfo->GetCaseFlags());
    } else {
        // Special properties handling for non-lemmer nodes - they don't have caseflags.
        SetProperties(NLemmer::ClassifyCase(Node->GetText().data(), Node->GetText().size()));
    }
    if (Node->IsOrdinaryWord())
        NormalizedText = Node->WordInfo->GetNormalizedForm();
    Text = GetRichRequestNodeText(Node);

    if (Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord()) {
        LemmaSelector.Set(*this);
    }
}

void TNodeInputSymbol::AssignNode(size_t pos, const TRichRequestNode* n) {
    Reset();

    NSymbol::TInputSymbol::SetSourcePos(pos);
    Node = n;
    InitSymbol();
}

void TNodeInputSymbol::AssignPunct(size_t pos, const wchar16 punct) {
    Reset();
    TInputSymbol::SetSourcePos(pos);
    Text.AssignNoAlias(&punct, 1);
    NormalizedText = Text;
    Properties.Set(NSymbol::PROP_PUNCT);
}

void TNodeInputSymbol::AssignMultiSymbol(TNodeInputSymbols::const_iterator start, TNodeInputSymbols::const_iterator end, const TVector<TDynBitMap>& contexts, ui32 mainWordIndex) {
    // Reset() method is called inside of the TInputSymbol::AssignMultiSymbol()
    NSymbol::TInputSymbol::AssignMultiSymbol(start, end, contexts, mainWordIndex);
    // Node is set to null by the Reset() method
}

void TNodeInputSymbol::Reset() {
    NSymbol::TInputSymbol::Reset();
    Node = nullptr;
    GztItems.clear();
}

void TNodeInputSymbol::AddGztItems(const TVector<const TGztResultItem*>& gztItems) {
    for (TVector<const TGztResultItem*>::const_iterator i = gztItems.begin(); i != gztItems.end(); ++i) {
        GztItems[(*i)->GetTitle()] = *i;
        AddGztArticle(**i);
    }
}

void TNodeInputSymbol::AddGztItem(const TGztResultItem* gztItem) {
    GztItems[gztItem->GetTitle()] = gztItem;
    AddGztArticle(*gztItem);
}

TLangMask TNodeInputSymbol::DoLoadLangMask() {
    TLangMask langMask;
    if (Node) {
        if (Node->WordInfo.Get()) {
            langMask = Node->WordInfo->GetLangMask();
        } else if (Node->GetPhraseType() == PHRASE_MULTIWORD) {
            for (size_t i = 0 ; i < Node->Children.size(); ++i) {
                if (Node->Children[i]->WordInfo.Get())
                    langMask |= Node->Children[i]->WordInfo->GetLangMask();
            }
        }
    }
    return langMask;
}

size_t TNodeInputSymbol::GetLemmaCount() const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    return Node->WordInfo->GetLemmas().size();
}

TWtringBuf TNodeInputSymbol::GetLemmaText(size_t lemma) const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    Y_ASSERT(lemma < Node->WordInfo->GetLemmas().size());
    const TLemmaForms& l = Node->WordInfo->GetLemmas()[lemma];
    return l.GetLemma();
}

ELanguage TNodeInputSymbol::GetLemmaLang(size_t lemma) const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    Y_ASSERT(lemma < Node->WordInfo->GetLemmas().size());
    const TLemmaForms& l = Node->WordInfo->GetLemmas()[lemma];
    return l.GetLanguage();
}

TGramBitSet TNodeInputSymbol::GetStemGram(size_t lemma) const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    Y_ASSERT(lemma < Node->WordInfo->GetLemmas().size());
    const TLemmaForms& l = Node->WordInfo->GetLemmas()[lemma];
    return l.GetStemGrammar();
}

size_t TNodeInputSymbol::GetFlexGramCount(size_t lemma) const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    Y_ASSERT(lemma < Node->WordInfo->GetLemmas().size());
    const TLemmaForms& l = Node->WordInfo->GetLemmas()[lemma];
    return l.GetFlexGrammars().size();
}

TGramBitSet TNodeInputSymbol::GetFlexGram(size_t lemma, size_t gram) const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    Y_ASSERT(lemma < Node->WordInfo->GetLemmas().size());
    const TLemmaForms& l = Node->WordInfo->GetLemmas()[lemma];
    Y_ASSERT(gram < l.GetFlexGrammars().size());
    return l.GetFlexGrammars()[gram];
}

NSymbol::ELemmaQuality TNodeInputSymbol::GetQuality(size_t lemma) const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    Y_ASSERT(lemma < Node->WordInfo->GetLemmas().size());
    const TLemmaForms& l = Node->WordInfo->GetLemmas()[lemma];
    return NSymbol::LemmaQualityFromLemmerQualityMask(l.GetQuality());
}

NSymbol::TYemmaIteratorPtr TNodeInputSymbol::GetYemmas() const {
    Y_ASSERT(Node && Node->WordInfo.Get() && Node->WordInfo->IsLemmerWord());
    return NSymbol::TYemmaIteratorPtr(new TNodeYemmaIterator(Node->WordInfo->GetLemmas()));
}

void DbgPrint(IOutputStream& s, const TNodeInputSymbols& inputSymbols) {
    TVector<TUtf16String> dbgStr(inputSymbols.size(), TUtf16String());
    std::transform(inputSymbols.begin(), inputSymbols.end(), dbgStr.begin(), NSymbol::TDebugTextExtractor());
    s << WideToUTF8(JoinStrings(dbgStr, SPACE));
}

} // NRichNode

} // NReMorph
