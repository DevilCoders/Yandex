#pragma once

#include "ctx_lemmas.h"
#include "input_symbol_util.h"
#include "lemmas.h"
#include "properties.h"

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/remorph/core/core.h>

#include <kernel/lemmer/dictlib/gleiche.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <library/cpp/langmask/langmask.h>

#include <library/cpp/langs/langs.h>
#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/bitmap.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <utility>


namespace NSymbol {

typedef ui16 TExpressionId;
class TInputSymbol;
typedef TIntrusivePtr<TInputSymbol> TInputSymbolPtr;
typedef TVector<TInputSymbolPtr> TInputSymbols;

class TInputSymbol: public TAtomicRefCount<TInputSymbol> {
protected:
    class TLemmaSelector : public TCtxLemmas {
    public:
        using TCtxLemmas::Set;
        using TCtxLemmas::Reset;

        void Set(const ILemmas& l) {
            Lemmas = &l;
            LemmaMap.clear();
            FlexGramMap.clear();
        }

        const ILemmas& GetLemmas() const;
    };

    struct TLemmaQualityChecker {
        TLemmaQualityBitSet LemmaQualities;

        TLemmaQualityChecker(const TLemmaQualityBitSet& lemmaQualities)
            : LemmaQualities(lemmaQualities)
        {
        }

        inline bool operator() (const ILemmas& lemmas, size_t i) const {
            return LemmaQualities.Test(lemmas.GetQuality(i));
        }
    };

protected:
    std::pair<size_t, size_t> SourcePos;
    //original position, that can strictly include SourcePos
    //(not only prime-field, but the whole matched string)
    std::pair<size_t, size_t> WholeSourcePos;

    TUtf16String Text;
    TUtf16String NormalizedText;

    // For caching expression matching
    TDynBitMap ProcessedExpressions;
    TDynBitMap MatchResults;

    TLangMask LangMask;
    TLangMask QLangMask;
    TPropertyBitSet Properties;
    TVector<NGzt::TArticlePtr> GztArticles;
    THashMap<TUtf16String, TDynBitMap> GztNames;
    THashMap<TUtf16String, TDynBitMap> GeoGztNames;
    // Contains positions of articles, which always will appear in match context
    TDynBitMap StickyArticles;
    bool GeoNamesInitialized;
    bool LangInitialized;

    // Multi-symbol support
    TInputSymbols Children;
    TVector<TDynBitMap> Contexts;
    size_t MainOffset;
    NRemorph::TNamedSubmatches NamedSubRanges;
    TString RuleName;
    double Weight;

    TLemmaSelector LemmaSelector;

private:
    void UpdateCompoundCaseProps(const TPropertyBitSet& props, bool firstComponent);

protected:
    TInputSymbol(size_t begin, size_t end)
        : SourcePos(begin, end)
        , WholeSourcePos(begin, end)
        , GeoNamesInitialized(false)
        , LangInitialized(false)
        , MainOffset(0)
        , Weight(0.0)
    {
    }
    TInputSymbol(size_t pos)
        : SourcePos(pos, pos + 1)
        , WholeSourcePos(pos, pos + 1)
        , GeoNamesInitialized(false)
        , LangInitialized(false)
        , MainOffset(0)
        , Weight(0.0)
    {
    }
    TInputSymbol()
        : SourcePos(-1, -1)
        , WholeSourcePos(-1, -1)
        , GeoNamesInitialized(false)
        , LangInitialized(false)
        , MainOffset(0)
        , Weight(0.0)
    {
    }

    template <class TIter>
    TInputSymbol(const TIter start, const TIter end, const TVector<TDynBitMap>& ctxs, size_t mainOffset)
        : GeoNamesInitialized(false)
        , LangInitialized(false)
        , Contexts(ctxs)
        , MainOffset(mainOffset)
        , Weight(1.0)
    {
        for (TIter i = start; i != end; ++i) {
            Children.push_back(i->Get());
        }
        InitMultiSymbol();
        Y_ASSERT(Children.size() == Contexts.size());
    }

    template <class TIter>
    void AssignMultiSymbol(const TIter start, const TIter end, const TVector<TDynBitMap>& ctxs, size_t mainOffset) {
        Reset();

        Contexts = ctxs;
        MainOffset = mainOffset;
        Weight = 1.0; // Multi-symbol has explicit or precalculated 1.0 weight

        for (TIter i = start; i != end; ++i) {
            Children.push_back(i->Get());
        }
        InitMultiSymbol();
        Y_ASSERT(Children.size() == Contexts.size());
    }

    Y_FORCE_INLINE static TPropertyBitSet CharCategory2Properties(TCharCategory cc) {
        return TPropertyBitSet()
            .Set(PROP_CASE_UPPER, (cc & CC_UPPERCASE) && !(cc & CC_NUMBER))
            .Set(PROP_CASE_TITLE, (cc & CC_TITLECASE) && !(cc & CC_NUMBER))
            .Set(PROP_CASE_MIXED, (cc & CC_MIXEDCASE) && !(cc & CC_NUMBER))
            .Set(PROP_CASE_LOWER, (cc & CC_LOWERCASE) && !(cc & CC_NUMBER))
            .Set(PROP_ALPHA, cc & CC_ALPHA)
            .Set(PROP_NUMBER, cc & CC_NUMBER)
            .Set(PROP_ASCII, cc & CC_ASCII)
            .Set(PROP_NONASCII, cc & CC_NONASCII)
            .Set(PROP_NMTOKEN, cc & CC_NMTOKEN)
            .Set(PROP_NUTOKEN, cc & CC_NUTOKEN)
            .Set(PROP_CALPHA, cc & CC_ALPHA)
            .Set(PROP_CNUMBER, cc & CC_NUMBER)
            ;
    }

    // This function has Gleiche signature and is used together with other gleiche operations
    static TGramBitSet IncludeGrammem(const TGramBitSet& gram, const TGramBitSet& subGram) {
        return (gram & subGram) == subGram ? gram : TGramBitSet();
    }

    Y_FORCE_INLINE static bool TestLang(const TLangMask& controllerMask, ELanguage lang) {
        return controllerMask.Test(lang) || (LANG_UNK == lang && controllerMask.Empty());
    }

    Y_FORCE_INLINE static bool TestLangMask(const TLangMask& controllerMask, const TLangMask& langMask) {
        return controllerMask.HasAny(langMask) || (controllerMask.Empty() && langMask.Empty());
    }

    template <class Functor>
    bool IterGrammems(Functor& func, bool& fullMatch) const {
        bool res = false;
        fullMatch = true;
        size_t pos = 0;
        const ILemmas& lemmas = GetLemmas();
        const size_t lemmaCount = lemmas.GetLemmaCount();
        for (size_t iLemma = 0; iLemma < lemmaCount; ++iLemma) {
            const TGramBitSet stemGram = lemmas.GetStemGram(iLemma);
            const size_t gramCount = lemmas.GetFlexGramCount(iLemma);
            if (gramCount > 0) {
                for (size_t iGram = 0; iGram < gramCount; ++iGram) {
                    if (func(stemGram | lemmas.GetFlexGram(iLemma, iGram), pos))
                        res = true;
                    else
                        fullMatch = false;
                    ++pos;
                }
            } else if (!stemGram.Empty()) {
                if (func(stemGram, pos))
                    res = true;
                else
                    fullMatch = false;
                ++pos;
            }
        }
        return res;
    }

    void LoadGeoNames();
    void LoadLangMask() {
        if (!LangInitialized) {
            LangMask = DoLoadLangMask();
            LangInitialized = true;
        }
    }
    virtual TLangMask DoLoadLangMask();

    void InitMultiSymbol();

    bool AgreeGztNames(const THashMap<TUtf16String, TDynBitMap>& gztNames, TDynBitMap& myCtx,
        const TInputSymbol& target, TDynBitMap& targetCtx) const;

    bool JoinGztContextIfIntersect(TDynBitMap& ctx, TDynBitMap& gatheredCtx) const;
    void JoinGztContext(bool emptyGztCtx, TDynBitMap& ctx, TDynBitMap& gatheredCtx) const;

    void SetProperties(const TPropertyBitSet& props);
    void UpdatePropCaseFirstUpper();
    void UpdateCompoundProps(const TPropertyBitSet& props, bool firstComponent);

public:
    virtual ~TInputSymbol() {
    }

    // Resets the symbol to initial state and make it ready to reuse
    virtual void Reset();

    ////////////////////// Match cache /////////////////////////////

    Y_FORCE_INLINE bool HasMatchResult(TExpressionId id, bool& res) const {
        Y_ASSERT(Max<TExpressionId>() != id);
        return ProcessedExpressions.Get(id) ? (res = MatchResults.Get(id), true) : false;
    }

    inline bool SetMatchResult(TExpressionId id, bool res) {
        Y_ASSERT(Max<TExpressionId>() != id);
        ProcessedExpressions.Set(id);
        if (res)
            MatchResults.Set(id);
        return res;
    }

    Y_FORCE_INLINE void ClearMatchResults() {
        ProcessedExpressions.Clear();
        MatchResults.Clear();
    }

    ////////////////////// Properties /////////////////////////////

    // Original text from the text/request
    Y_FORCE_INLINE const TUtf16String& GetText() const {
        return Text;
    }

    Y_FORCE_INLINE void SetText(const TUtf16String& str) {
        Text = str;
    }

    // Normalized text from the text/request (case, diacritics, etc)
    Y_FORCE_INLINE const TUtf16String& GetNormalizedText() const {
        return NormalizedText;
    }

    Y_FORCE_INLINE void SetNormalizedText(const TUtf16String& str) {
        NormalizedText = str;
    }

    Y_FORCE_INLINE const std::pair<size_t, size_t>& GetSourcePos() const {
        return SourcePos;
    }

    Y_FORCE_INLINE void SetSourcePos(const std::pair<size_t, size_t>& pos) {
        SourcePos = pos;
        // Set WholeSourcePos only if it is not set explicitly
        if (WholeSourcePos.second == WholeSourcePos.first)
            WholeSourcePos = pos;
    }

    inline void SetSourcePos(size_t pos) {
        SourcePos.first = pos;
        SourcePos.second = pos + 1;
        // Set WholeSourcePos only if it is not set explicitly
        if (WholeSourcePos.second == WholeSourcePos.first)
            WholeSourcePos = SourcePos;
    }

    Y_FORCE_INLINE const std::pair<size_t, size_t>& GetWholeSourcePos() const {
        return WholeSourcePos;
    }

    Y_FORCE_INLINE void SetWholeSourcePos(const std::pair<size_t, size_t>& pos) {
        WholeSourcePos = pos;
    }


    Y_FORCE_INLINE const TPropertyBitSet& GetProperties() const {
        return Properties;
    }

    Y_FORCE_INLINE TPropertyBitSet& GetProperties() {
        return Properties;
    }

    inline void SetProperties(TCharCategory cc) {
        SetProperties(CharCategory2Properties(cc));
        UpdatePropCaseFirstUpper();
    }

    inline void UpdateCompoundProps(TCharCategory cc, bool firstComponent) {
        UpdateCompoundProps(CharCategory2Properties(cc), firstComponent);
    }

    Y_FORCE_INLINE const NRemorph::TNamedSubmatches& GetNamedSubRanges() const {
        return NamedSubRanges;
    }

    Y_FORCE_INLINE NRemorph::TNamedSubmatches& GetNamedSubRanges() {
        return NamedSubRanges;
    }

    Y_FORCE_INLINE const TString& GetRuleName() const {
        return RuleName;
    }

    Y_FORCE_INLINE void SetRuleName(const TString& s) {
        RuleName = s;
    }

    double CalcWeight(const TDynBitMap& ctx) const;

    inline double GetWeight() const {
        // Zero value means uninitialized weight. Use 1.0 by default
        return 0.0 == Weight ? 1.0 : Weight;
    }

    inline void SetWeight(double w) {
        Weight = w;
    }

    // Non-const because we use delayed lang-mask initialization
    Y_FORCE_INLINE const TLangMask& GetLangMask() {
        LoadLangMask();
        return LangMask;
    }

    Y_FORCE_INLINE void SetQLangMask(const TLangMask& langMask) {
        QLangMask = langMask;
    }

    Y_FORCE_INLINE const TLangMask& GetQLangMask() const {
        return QLangMask;
    }

    ////////////////////// Gazetteer articles /////////////////////////////

    void AddGztArticle(const NGzt::TArticlePtr& article, bool sticky = false);
    void AddGztArticles(const TVector<NGzt::TArticlePtr>& articles, bool sticky = false);
    void AddGztArticles(const TInputSymbol& src, const TDynBitMap& ctx, bool sticky = false);

    Y_FORCE_INLINE const TVector<NGzt::TArticlePtr>& GetGztArticles() const {
        return GztArticles;
    }

    // Applies supplied operator to each article from the context while operator returns false
    // If context is empty then applies operator to all articles
    template <class TOperator>
    bool TraverseArticles(const TDynBitMap& ctx, TOperator op) const {
        size_t i = ctx.FirstNonZeroBit();
        if (i < GztArticles.size() && i < ctx.Size()) {
            for (; i < ctx.Size() && i < GztArticles.size(); i = ctx.NextNonZeroBit(i)) {
                if (op(GztArticles[i]))
                    return true;
            }
        } else {
            // Gazetteer context is empty. Iterate all articles
            for (TVector<NGzt::TArticlePtr>::const_iterator iArt = GztArticles.begin(); iArt != GztArticles.end(); ++iArt) {
                if (op(*iArt))
                    return true;
            }
        }
        return false;
    }

    // Applies supplied operator to each article from the context while operator returns false
    // If context is empty then doesn't apply at all
    template <class TOperator>
    bool TraverseOnlyCtxArticles(const TDynBitMap& ctx, TOperator op) const {
        size_t i = ctx.FirstNonZeroBit();
        if (i < GztArticles.size() && i < ctx.Size()) {
            for (; i < ctx.Size() && i < GztArticles.size(); i = ctx.NextNonZeroBit(i)) {
                if (op(GztArticles[i]))
                    return true;
            }
        }
        return false;
    }

    // The function updates gztCtx bitmap and set bits with corresponding positions of matched articles
    // Resulting context contains gazetteer offsets only
    inline bool HasGztArticle(const TUtf16String& gztArticle, TDynBitMap& gztCtx) const {
        THashMap<TUtf16String, TDynBitMap>::const_iterator i = GztNames.find(gztArticle);
        if (i != GztNames.end()) {
            // Add sticky articles to all matches
            gztCtx.Or(i->second).Or(StickyArticles);
            return true;
        }
        return false;
    }

    // The function updates gztCtx bitmap and set bits with corresponding positions of equal articles.
    // Resulting context contains gazetteer offsets only
    bool HasGztArticle(const TArticlePtr& a, TDynBitMap& gztCtx) const;

    inline bool AgreeGztArticle(TDynBitMap& myCtx, const TInputSymbol& target, TDynBitMap& targetCtx) const {
        return AgreeGztNames(GztNames, myCtx, target, targetCtx);
    }
    // Non-const because we use delayed geo-names initialization
    inline bool AgreeGeoGztArticle(TDynBitMap& myCtx, const TInputSymbol& target, TDynBitMap& targetCtx) {
        LoadGeoNames();
        return AgreeGztNames(GeoGztNames, myCtx, target, targetCtx);
    }

    bool AgreeGztIds(TDynBitMap& myCtx, const TInputSymbol& target, TDynBitMap& targetCtx) const;

    // Returns normal text of the symbol, which is specified in the gzt article as the "lemma" attribute
    inline TGztLemma GetGazetteerForm() const {
        return GetGazetteerForm(TDynBitMap());
    }

    // The same as above, but uses context to choose gzt articles
    TGztLemma GetGazetteerForm(const TDynBitMap& ctx) const;

    // Returns all possible texts of the symbol, which are specified in the gzt articles as the "lemma" attribute
    inline TGztLemmas GetAllGazetteerForms() const {
        return GetAllGazetteerForms(TDynBitMap());
    }
    // The same as above, but uses context to choose gzt articles
    TGztLemmas GetAllGazetteerForms(const TDynBitMap& ctx) const;

    ////////////////////// Grammar /////////////////////////////

    Y_FORCE_INLINE const ILemmas& GetLemmas() const {
        return LemmaSelector.GetLemmas();
    }

    // Try to avoid using GetLemmas(ctx) frequently with the same context, better cache its result.
    inline TCtxLemmas GetLemmas(const TDynBitMap& ctx) const {
        TCtxLemmas res;
        res.Set(*this, ctx);
        return res;
    }

    bool CheckGrammems(NGleiche::TGleicheFunc f, const TGramBitSet& grammar, TDynBitMap& ctx, bool& fullMatch) const;
    bool AgreeGrammems(NGleiche::TGleicheFunc f, TDynBitMap& myCtx, const TInputSymbol& target, TDynBitMap& targetCtx) const;

    // The function sets fullMatch flag if all symbol grammems has the specified grammar
    inline bool HasGrammems(const TGramBitSet& grammar, TDynBitMap& ctx, bool& fullMatch) const {
        return CheckGrammems(IncludeGrammem, grammar, ctx, fullMatch);
    }

    Y_FORCE_INLINE size_t GetFirstGrammarCtxBit(const TDynBitMap& ctx) const {
        return GztArticles.empty() ? ctx.FirstNonZeroBit() : ctx.NextNonZeroBit(GztArticles.size() - 1);
    }

    Y_FORCE_INLINE bool IsGztCtxEmpty(const TDynBitMap& ctx) const {
        const size_t nonZeroBit = ctx.FirstNonZeroBit();
        return nonZeroBit >= GztArticles.size() || nonZeroBit >= ctx.Size();
    }

    template <class TOperator>
    bool TraverseLemmas(const TDynBitMap& ctx, TOperator op) const {
        const size_t offset = GztArticles.size();
        const bool ctxEmpty = (offset ? ctx.NextNonZeroBit(offset - 1) : ctx.FirstNonZeroBit()) >= ctx.Size();

        size_t pos = 0;
        const ILemmas& lemmas = GetLemmas();
        const size_t lemmaCount = lemmas.GetLemmaCount();
        for (size_t iLemma = 0; iLemma < lemmaCount; ++iLemma) {
            const size_t gramCount = lemmas.GetFlexGramCount(iLemma);
            if (gramCount > 0) {
                bool inCtx = ctxEmpty;
                if (!inCtx) {
                    for (size_t iGram = 0; !inCtx && iGram < gramCount; ++iGram) {
                        if (ctx.Test(offset + pos + iGram))
                            inCtx = true;
                    }
                }
                pos += gramCount;
                if (inCtx && op(lemmas.GetLemmaText(iLemma)))
                    return true;
            } else if (!lemmas.GetStemGram(iLemma).Empty()) {
                if ((ctxEmpty || ctx.Test(offset + pos)) && op(lemmas.GetLemmaText(iLemma)))
                    return true;
                ++pos;
            }
        }
        return false;
    }

    template <class TLemmaChecker>
    bool CheckLemmas(const TLemmaChecker& check, TDynBitMap& ctx, bool& fullMatch) const {
        bool res = false;
        fullMatch = true;
        size_t pos = GztArticles.size(); // Initial offset of grammar context
        const ILemmas& lemmas = GetLemmas();
        const size_t lemmaCount = lemmas.GetLemmaCount();
        for (size_t iLemma = 0; iLemma < lemmaCount; ++iLemma) {
            const size_t gramCount = Max(lemmas.GetFlexGramCount(iLemma), size_t(!lemmas.GetStemGram(iLemma).Empty()));
            if (gramCount > 0) {
                if (check(lemmas, iLemma)) {
                    res = true;
                    ctx.Set(pos, pos + gramCount);
                } else {
                    fullMatch = false;
                }
                pos += gramCount;
            }
        }
        return res;
    }

    NSpike::TGrammarBunch GetAllGram(size_t lemmaIndex) const;

    // Returns text of the symbol, which is converted to nominative form
    inline TUtf16String GetNominativeForm() const {
        return GetNominativeForm(TDynBitMap());
    }
    // The same as above, but uses context to choose lemmas
    TUtf16String GetNominativeForm(const TDynBitMap& ctx) const;

    ////////////////////// Multi-symbol /////////////////////////////

    Y_FORCE_INLINE const TInputSymbol* GetHead() const {
        Y_ASSERT(Children.empty() || MainOffset < Children.size());
        return Children.empty() ? nullptr : Children[MainOffset].Get();
    }

    Y_FORCE_INLINE TInputSymbol* GetHead() {
        Y_ASSERT(Children.empty() || MainOffset < Children.size());
        return Children.empty() ? nullptr : Children[MainOffset].Get();
    }

    Y_FORCE_INLINE const TInputSymbols& GetChildren() const {
        return Children;
    }

    Y_FORCE_INLINE const TVector<TDynBitMap>& GetContexts() const {
        return Contexts;
    }

    Y_FORCE_INLINE void UpdateContexts(const TVector<TDynBitMap>& ctxs) {
        Y_ASSERT(Contexts.size() == ctxs.size());
        Contexts = ctxs;
    }

    TString ToDebugString(const TDynBitMap& ctx) const;
    inline TString ToDebugString() const {
        return ToDebugString(TDynBitMap());
    }

    inline bool HasLemmaQualities(const TLemmaQualityBitSet& lemmaQualities, TDynBitMap& ctx) const {
        // @todo Избавиться от dummyFullMatch.
        bool dummyFullMatch;
        return CheckLemmas(TLemmaQualityChecker(lemmaQualities), ctx, dummyFullMatch);
    }

    void ExpungeChildrenGztArticles(const TDynBitMap& ctx);
};

}  // NSymbol

inline TString ToString(const NSymbol::TInputSymbolPtr& s) {
    return s ? WideToUTF8(s->GetText()) : TString();
}

inline TString ToString(const NSymbol::TInputSymbol& s) {
    return WideToUTF8(s.GetText());
}

inline TUtf16String ToWtroku(const NSymbol::TInputSymbolPtr& s) {
    return s ? s->GetText() : TUtf16String();
}

inline TUtf16String ToWtroku(const NSymbol::TInputSymbol& s) {
    return s.GetText();
}
