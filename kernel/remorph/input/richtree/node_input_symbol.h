#pragma once

#include <kernel/remorph/input/input_symbol.h>
#include <kernel/remorph/input/lemmas.h>
#include <kernel/remorph/input/gazetteer_input.h>
#include <kernel/remorph/common/gztoccurrence.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <kernel/gazetteer/richtree/gztres.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/charset/wide.h>

namespace NReMorph {

namespace NRichNode {

using namespace NGztSupport;

class TNodeInputSymbol;

typedef TIntrusivePtr<TNodeInputSymbol> TNodeInputSymbolPtr;
typedef TVector<TNodeInputSymbolPtr> TNodeInputSymbols;
typedef NRemorph::TInput<TNodeInputSymbolPtr> TNodeInput;

typedef THashMap<TUtf16String, const TGztResultItem*> TArticleTitle2Body;

class TNodeYemmaIterator;

class TNodeInputSymbol: public NSymbol::TInputSymbol, private NSymbol::ILemmas {
protected:
    TArticleTitle2Body GztItems;
    const TRichRequestNode* Node;

private:
    size_t GetLemmaCount() const override;

    TWtringBuf GetLemmaText(size_t lemma) const override;

    ELanguage GetLemmaLang(size_t lemma) const override;

    TGramBitSet GetStemGram(size_t lemma) const override;
    size_t GetFlexGramCount(size_t lemma) const override;
    TGramBitSet GetFlexGram(size_t lemma, size_t gram) const override;
    NSymbol::ELemmaQuality GetQuality(size_t lemma) const override;
    NSymbol::TYemmaIteratorPtr GetYemmas() const override;

    void InitSymbol();

protected:
    TLangMask DoLoadLangMask() override;

public:
    TNodeInputSymbol();
    TNodeInputSymbol(size_t pos, const TRichRequestNode* n);
    TNodeInputSymbol(size_t pos, const wchar16 punct);
    TNodeInputSymbol(TNodeInputSymbols::const_iterator start, TNodeInputSymbols::const_iterator end, const TVector<TDynBitMap>& contexts, ui32 mainWordIndex);

    // Assigners. All assigners call Reset(), so you don't need to call it before assigning
    void AssignNode(size_t pos, const TRichRequestNode* n);
    void AssignPunct(size_t pos, const wchar16 punct);
    void AssignMultiSymbol(TNodeInputSymbols::const_iterator start, TNodeInputSymbols::const_iterator end, const TVector<TDynBitMap>& contexts, ui32 mainWordIndex);

    void Reset() override;

    void AddGztItems(const TVector<const TGztResultItem*>& equalGztItems);
    void AddGztItem(const TGztResultItem* gztItem);

    inline bool IsAssigned() const {
        return Node != nullptr || !Text.empty();
    }

    Y_FORCE_INLINE const TArticleTitle2Body& GetGztItems() const {
        return GztItems;
    }

    Y_FORCE_INLINE const TRichRequestNode* GetRichNode() const {
        return Node;
    }

    friend class TNodeYemmaIterator;
};

void DbgPrint(IOutputStream& s, const TNodeInputSymbols& inputSymbols);

} // NRichNode

} // NReMorph

namespace NGztSupport {

template <>
struct TGztItemTraits<const TGztResultItem*> {
    Y_FORCE_INLINE static void AddGztItem(NReMorph::NRichNode::TNodeInputSymbol& s, const TGztResultItem* item) {
        s.AddGztItem(item);
    }

    Y_FORCE_INLINE static void AddGztItems(NReMorph::NRichNode::TNodeInputSymbol& s, const TVector<const TGztResultItem*>& items) {
        s.AddGztItems(items);
    }

    Y_FORCE_INLINE static const TArticlePtr& ToArticle(const TGztResultItem* item) {
        return *item;
    }
};

} // NGztSupport

DECLARE_GAZETTEER_SUPPORT(NReMorph::NRichNode::TNodeInputSymbols, TRichNodeSymbolsIterator)
