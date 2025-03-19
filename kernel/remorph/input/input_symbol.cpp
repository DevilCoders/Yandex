#include "input_symbol.h"

#include "lemma_quality.h"

#include <kernel/remorph/common/article_util.h>

#include <kernel/geograph/geograph.h>

#include <util/charset/unidata.h>
#include <util/generic/algorithm.h>

namespace NSymbol {

static const TPropertyBitSet CASE_FLAGS = TPropertyBitSet(PROP_CASE_MIXED, PROP_CASE_UPPER, PROP_CASE_CAMEL, PROP_CASE_TITLE)
    | TPropertyBitSet(PROP_CASE_LOWER);
static const TPropertyBitSet CAT_FLAGS = TPropertyBitSet(PROP_ALPHA, PROP_NUMBER, PROP_ASCII, PROP_NONASCII)
    | TPropertyBitSet(PROP_NMTOKEN, PROP_NUTOKEN);
static const TPropertyBitSet COMPOUND_FLAGS = TPropertyBitSet(PROP_COMPOUND, PROP_CALPHA, PROP_CNUMBER);

static const TPropertyBitSet CASE_UPPER_FLAGS = TPropertyBitSet(PROP_CASE_UPPER, PROP_CASE_TITLE);
static const TPropertyBitSet CAT_MW_FLAGS = TPropertyBitSet(PROP_ASCII);
static const TPropertyBitSet ALPHA_FLAGS = TPropertyBitSet(PROP_ALPHA, PROP_CALPHA);
static const TPropertyBitSet NUMBER_FLAGS = TPropertyBitSet(PROP_NUMBER, PROP_CNUMBER);

struct TDummyLemmas: public ILemmas {
    inline TDummyLemmas() {
    }

    size_t GetLemmaCount() const override {
        return 0;
    }
    TWtringBuf GetLemmaText(size_t /*lemma*/) const override {
        Y_ASSERT(false);
        return TWtringBuf();
    }
    ELanguage GetLemmaLang(size_t /*lemma*/) const override {
        Y_ASSERT(false);
        return LANG_UNK;
    }
    TGramBitSet GetStemGram(size_t /*lemma*/) const override {
        Y_ASSERT(false);
        return TGramBitSet();
    }
    size_t GetFlexGramCount(size_t /*lemma*/) const override {
        Y_ASSERT(false);
        return 0;
    }
    TGramBitSet GetFlexGram(size_t /*lemma*/, size_t /*gram*/) const override {
        Y_ASSERT(false);
        return TGramBitSet();
    }
    ELemmaQuality GetQuality(size_t /*lemma*/) const override {
        Y_ASSERT(false);
        return LQ_GOOD;
    }
    TYemmaIteratorPtr GetYemmas() const override {
        return TYemmaIteratorPtr();
    }
};

static const TDummyLemmas NO_LEMMAS;

void TInputSymbol::Reset() {
    SourcePos.first = SourcePos.second = -1;
    WholeSourcePos.first = WholeSourcePos.second = 0;

    Text.clear();
    NormalizedText.clear();

    ClearMatchResults();

    LangMask.Reset();
    QLangMask.Reset();
    Properties.Reset();

    GztArticles.clear();
    GztNames.clear();
    GeoGztNames.clear();
    StickyArticles.Clear();

    GeoNamesInitialized = false;
    LangInitialized = false;

    Children.clear();
    Contexts.clear();
    MainOffset = 0;
    NamedSubRanges.clear();
    RuleName.clear();
    Weight = 0.0;

    LemmaSelector.Reset();
}

const ILemmas& TInputSymbol::TLemmaSelector::GetLemmas() const {
    return nullptr == Lemmas ? NO_LEMMAS : (LemmaMap.empty() ? *Lemmas : *this);
}

void TInputSymbol::SetProperties(const TPropertyBitSet& props) {
    Properties = props;
}

void TInputSymbol::UpdatePropCaseFirstUpper() {
    if (!Text.empty() && ::IsUpper(Text[0])) {
        Properties.Set(PROP_CASE_FIRST_UPPER);
    }
}

// This method should be called only for compound symbols.
void TInputSymbol::UpdateCompoundProps(const TPropertyBitSet& props, bool firstComponent) {
    // Handle case flags.
    UpdateCompoundCaseProps(props, firstComponent);

    if (firstComponent) {
        // Firstly set all compound flags.
        // This will be refuted by traversing components.
        Properties |= COMPOUND_FLAGS;

        // AND-logic for punctuation flag.
        Properties.Set(PROP_PUNCT, true);
    }

    // Merge category compound flags from component.
    Properties.Set(PROP_CALPHA, Properties.Test(PROP_CALPHA) && props.HasAny(ALPHA_FLAGS));
    Properties.Set(PROP_CNUMBER, Properties.Test(PROP_CNUMBER) && props.HasAny(NUMBER_FLAGS));

    // Merge punctuation flag from component.
    Properties.Set(PROP_PUNCT, Properties.Test(PROP_PUNCT) && props.Test(PROP_PUNCT));
}

void TInputSymbol::UpdateCompoundCaseProps(const TPropertyBitSet& props, bool firstComponent) {
    if (firstComponent) {
        // For first component just copy case flags.
        Properties = (Properties & ~CASE_FLAGS) | (props & CASE_FLAGS);
    } else if (!Properties.Test(PROP_CASE_MIXED)) { // If we already have MIXED flag then ignore all other
        // Check the special case when a word is a upper single-letter.
        // In this case it has both UPPER and TITLE flags.
        // Properly merge it with other flags:
        // [W] [Word] -> Title,Camel
        // [W] [WORD] -> Upper
        // [W] [word] -> Title
        if (Properties.HasAll(CASE_UPPER_FLAGS) || props.HasAll(CASE_UPPER_FLAGS)) {
            if ((Properties & props).HasAny(CASE_UPPER_FLAGS)) {
                Properties &= ~CASE_FLAGS | TPropertyBitSet(PROP_CASE_CAMEL) | props;
                Properties.Set(PROP_CASE_CAMEL, props.Test(PROP_CASE_TITLE));
                return;
            } else {
                Properties.Reset(PROP_CASE_UPPER);
            }
        }

        // Set MIXED flag
        if (props.Test(PROP_CASE_MIXED) // Explicit MIXED flag
            || (props.Test(PROP_CASE_UPPER) != Properties.Test(PROP_CASE_UPPER)) // UPPER word in the middle or non-UPPER word after UPPER sequence
            || (props.Test(PROP_CASE_TITLE) && !Properties.Test(PROP_CASE_TITLE)) // TITLE word in the middle
            ) {
            Properties &= ~CASE_FLAGS;
            Properties.Set(PROP_CASE_MIXED);
            return;
        }
        // Set CAMEL flag if we have at least one another word with the TITLE case
        Properties.Set(PROP_CASE_CAMEL, Properties.Test(PROP_CASE_CAMEL) || (Properties.Test(PROP_CASE_TITLE) && props.Test(PROP_CASE_TITLE)));
    }
}

struct TWeightCalculator {
    double& Weight;

    TWeightCalculator(double& w)
        : Weight(w)
    {
    }

    Y_FORCE_INLINE bool operator() (const TArticlePtr& a) const {
        Weight = Max(Weight, NGztSupport::GetGztWeight(a));
        return false;
    }
};


double TInputSymbol::CalcWeight(const TDynBitMap& ctx) const {
    // Zero value means uninitialized weight. Calc it using gzt articles
    if (0.0 == Weight) {
        double gztWeight = 0.0;
        TraverseOnlyCtxArticles(ctx, TWeightCalculator(gztWeight));
        return 0.0 == gztWeight ? 1.0 : gztWeight;
    }
    return Weight;
}


void TInputSymbol::AddGztArticle(const TArticlePtr& article, bool sticky) {
    const size_t ndx = GztArticles.size();
    TUtf16String title = article.GetTitle();
    TDynBitMap& titleBits = GztNames[title];
    TDynBitMap& typeBits = GztNames[article.GetTypeName()];
    // Add only unique articles
    if (titleBits.Empty() || typeBits.Empty()) {
        GztArticles.push_back(article);
        titleBits.Set(ndx);
        typeBits.Set(ndx);
        if (NGztSupport::GetLanguageIndependentTitle(title))
            GztNames[title].Set(ndx);
        if (sticky)
            StickyArticles.Set(ndx);
    }
}

void TInputSymbol::AddGztArticles(const TVector<TArticlePtr>& articles, bool sticky) {
    const size_t oldSize = GztArticles.size();
    for (TVector<TArticlePtr>::const_iterator i = articles.begin(); i != articles.end(); ++i) {
        AddGztArticle(*i, false);
    }
    // A little optimization - set all bits at once.
    // Use actual count of added articles, because some of them could be skipped
    if (sticky && GztArticles.size() > oldSize)
        StickyArticles.Set(oldSize, GztArticles.size());
}

bool TInputSymbol::HasGztArticle(const TArticlePtr& a, TDynBitMap& gztCtx) const {
    // Find bits of articles with the equal title
    THashMap<TUtf16String, TDynBitMap>::const_iterator i = GztNames.find(a.GetTitle());
    if (i != GztNames.end()) {
        const TDynBitMap& titleIds = i->second;
        // Find bits of articles with the equal type
        i = GztNames.find(a.GetTypeName());
        if (i != GztNames.end()) {
            const TDynBitMap bits = titleIds & i->second;
            Y_ASSERT(!bits.Empty()); // Must have at least on common bit
            // Finally compare article ids
            bool hasMatch = false;
            Y_FOR_EACH_BIT(pos, bits) {
                if (GztArticles[pos].GetId() == a.GetId()) {
                    gztCtx.Set(pos);
                    hasMatch = true;
                }
            }
            // Add sticky articles to all matches
            if (hasMatch) {
                gztCtx.Or(StickyArticles);
                return true;
            }
        }
    }
    return false;
}


struct TArticleCollector {
    TInputSymbol& Symbol;
    bool Sticky;

    TArticleCollector(TInputSymbol& s, bool sticky)
        : Symbol(s)
        , Sticky(sticky)
    {
    }

    Y_FORCE_INLINE bool operator() (const TArticlePtr& a) const {
        Symbol.AddGztArticle(a, Sticky);
        return false;
    }
};

void TInputSymbol::AddGztArticles(const TInputSymbol& src, const TDynBitMap& ctx, bool sticky) {
    src.TraverseArticles(ctx, TArticleCollector(*this, sticky));
}

// Return false if existing context and new one do not have common matches
bool TInputSymbol::JoinGztContextIfIntersect(TDynBitMap& ctx, TDynBitMap& gatheredCtx) const {
    if (IsGztCtxEmpty(ctx)) {
        // If gazetteer context is empty then fill it by the match context
        ctx.Or(gatheredCtx);
    } else if (ctx.HasAny(gatheredCtx)) {
        // Fill higher bits in order to keep grammar sub-context unchanged
        if (GetGztArticles().size() < ctx.Size())
            gatheredCtx.Set(GetGztArticles().size(), ctx.Size());
        ctx.And(gatheredCtx);
    } else {
        // No match within the target context
        return false;
    }
    return true;
}

void TInputSymbol::JoinGztContext(bool emptyGztCtx, TDynBitMap& ctx, TDynBitMap& gatheredCtx) const {
    if (emptyGztCtx) {
        ctx.Or(gatheredCtx);
    } else {
        // Fill higher bits in order to keep grammar sub-context unchanged
        if (GetGztArticles().size() < ctx.Size())
            gatheredCtx.Set(GetGztArticles().size(), ctx.Size());
        ctx.And(gatheredCtx);
    }
}

// Contexts contain full info: gazetteer and grammar sub-contexts
// Empty sub-context is equal to full one in order to cover the case when gazetteer articles were not checked
bool TInputSymbol::AgreeGztNames(const THashMap<TUtf16String, TDynBitMap>& gztNames, TDynBitMap& myCtx,
    const TInputSymbol& target, TDynBitMap& targetCtx) const {

    bool res = false;
    const bool myCtxEmpty = IsGztCtxEmpty(myCtx);
    TDynBitMap comulativeMyCtx;
    TDynBitMap comulativeTargetCtx;
    for (THashMap<TUtf16String, TDynBitMap>::const_iterator i = gztNames.begin(); i != gztNames.end(); ++i) {
        if (myCtxEmpty || myCtx.HasAny(i->second)) {
            if (target.HasGztArticle(i->first, comulativeTargetCtx)) {
                comulativeMyCtx |= i->second;
                res = true;
            }
        }
    }

    if (!res) {
        return false;
    }

    // Update contexts with gathered info
    res = target.JoinGztContextIfIntersect(targetCtx, comulativeTargetCtx);

    if (!res) {
        return false;
    }

    JoinGztContext(myCtxEmpty, myCtx, comulativeMyCtx);
    myCtx.Or(StickyArticles);

    return true;
}

// Contexts contain full info: gazetteer and grammar sub-contexts
// Empty sub-context is equal to full one in order to cover the case when gazetteer articles were not checked
bool TInputSymbol::AgreeGztIds(TDynBitMap& myCtx, const TInputSymbol& target, TDynBitMap& targetCtx) const {
    bool res = false;
    const bool myCtxEmpty = IsGztCtxEmpty(myCtx);
    TDynBitMap comulativeMyCtx;
    TDynBitMap comulativeTargetCtx;
    for (size_t i = 0; i < GztArticles.size(); ++i) {
        if (myCtxEmpty || myCtx.Test(i)) {
            if (target.HasGztArticle(GztArticles[i], comulativeTargetCtx)) {
                comulativeMyCtx.Set(i);
                res = true;
            }
        }
    }
    if (!res) {
        return false;
    }
    // Update contexts with gathered info
    res = target.JoinGztContextIfIntersect(targetCtx, comulativeTargetCtx);

    if (!res) {
        return false;
    }

    JoinGztContext(myCtxEmpty, myCtx, comulativeMyCtx);
    myCtx.Or(StickyArticles);

    return true;
}

struct TCollectNamesFunctor {
    THashMap<TUtf16String, TDynBitMap>& Names;
    const size_t& Ndx;

    TCollectNamesFunctor(THashMap<TUtf16String, TDynBitMap>& names, const size_t& ndx)
        : Names(names)
        , Ndx(ndx)
    {
    }

    bool operator () (const TWtringBuf& title) const {
        auto it = Names.find(title);
        if (it != Names.end() && it->second.Get(Ndx))
             return true;

        Names[title].Set(Ndx);
        return false;
    }
};

void TInputSymbol::LoadGeoNames() {
    if (!GeoNamesInitialized) {
        size_t i = 0;
        TCollectNamesFunctor func(GeoGztNames, i);
        for (; i < GztArticles.size(); ++i) {
            NGeoGraph::TraverseGeoPartsAll(GztArticles[i], func);
        }
        GeoNamesInitialized = true;
    }
}

TLangMask TInputSymbol::DoLoadLangMask() {
    TLangMask langMask;
    const ILemmas& lemmas = GetLemmas();
    const size_t count = lemmas.GetLemmaCount();
    for (size_t i = 0; i < count; ++i) {
        langMask.SafeSet(lemmas.GetLemmaLang(i));
    }
    return langMask;
}

void TInputSymbol::InitMultiSymbol() {
    Y_ASSERT(!Children.empty());
    Y_ASSERT(MainOffset < Children.size());

    LangInitialized = true;

    if (Contexts.empty()) {
        Contexts.resize(Children.size());
    }

    SetSourcePos(std::pair<size_t, size_t>(Children.front()->GetSourcePos().first, Children.back()->GetSourcePos().second));

    Text = JoinInputSymbolText(Children.begin(), Children.end(), TTextExtractor());
    NormalizedText = JoinInputSymbolText(Children.begin(), Children.end(), TNormalizedTextExtractor());
    LemmaSelector.Set(*Children[MainOffset], Contexts[MainOffset]);

    // Handling properties.
    if (Children.size() == 1) {
        // For one-word multisymbol just copy properties.
        SetProperties(Children.front()->GetProperties());

        LangMask = Children.front()->GetLangMask();
        QLangMask = Children.front()->GetQLangMask();
    } else {
        // Multiword.
        // The following properties are always unset for multiwords:
        //  alpha (use calpha for word phrases)
        //  num (use cnum for number sequences)
        //  nmtoken (makes sence only for tokens)
        //  nutoken (makes sense only for tokens)
        //  nascii (multiwords contain ASCII-separators)
        // The only possible category flag except compounds for multiwords is ascii.

        // Before traversing multiword components let's unset all category flags.
        Properties &= ~CAT_FLAGS;
        // Then set all appropriate multiword category flags (ascii).
        // This will be refuted by traversing multiword components.
        Properties |= CAT_MW_FLAGS;

        // Update special property for uppercase first letter.
        UpdatePropCaseFirstUpper();

        for (TInputSymbols::const_iterator i = Children.begin(); i != Children.end(); ++i) {
            const TPropertyBitSet& props = i->Get()->GetProperties();
            const bool firstWord = i == Children.begin();

            // Merge component properties.
            Properties &= ~CAT_MW_FLAGS | props;

            // Update compound properties (same algo for multitokens and multiwords).
            UpdateCompoundProps(props, firstWord);

            LangMask |= i->Get()->GetLangMask();
            QLangMask |= i->Get()->GetQLangMask();
        }

        Properties.Set(PROP_MULTIWORD);
        Properties.Set(PROP_SPACE_BEFORE, Children.front()->GetProperties().Test(PROP_SPACE_BEFORE));
        Properties.Set(PROP_FIRST, Children.front()->GetProperties().Test(PROP_FIRST));
        Properties.Set(PROP_LAST, Children.back()->GetProperties().Test(PROP_LAST));
    }
}

struct TDebugArticleCollector {
    TString& Res;
    bool First;

    TDebugArticleCollector(TString& s)
        : Res(s)
        , First(true)
    {
    }

    bool operator() (const NGzt::TArticlePtr& a) {
        if (!First)
            Res.append(',');
        Res.append(a->GetTypeName()).append('[').append(WideToUTF8(a.GetTitle())).append(']');
        First = false;
        return false;
    }
};

TString TInputSymbol::ToDebugString(const TDynBitMap& ctx) const {
    TString res;
    res.append("[text: '").append(WideToUTF8(GetText())).append("'; ")
        .append("pos: [").append(::ToString(SourcePos.first)).append(',')
        .append(::ToString(SourcePos.second)).append(')');
    if (!Properties.Empty()) {
        res.append("; prop: ");
        for (EProperty p : Properties) {
            if (p != *Properties.begin()) {
                res.append(',');
            }
            res.append(NameByProperty(p));
        }
    }
    if (!GztArticles.empty()) {
        res.append("; gzt: ");
        TraverseArticles(ctx, TDebugArticleCollector(res));
    }
    res.append(']');
    if (!QLangMask.Empty())
        res.insert(1, TString("qlang: ").append(NLanguageMasks::ToString(QLangMask)).append("; "));
    return res;
}

struct TGetGztLemmas {
    TGztLemmas& Lemmas;
    TVector<NGzt::TArticlePtr>& CommonArticles;
    bool SingleResult;

    TGetGztLemmas(TGztLemmas& res, TVector<NGzt::TArticlePtr>& articles, bool single)
        : Lemmas(res)
        , CommonArticles(articles)
        , SingleResult(single)
    {
    }

    inline bool operator ()(const TArticlePtr& a) const {
        TUtf16String text = NGztSupport::GetLemma(a);
        if (!text.empty()) {
            if (!SingleResult || Lemmas.empty() || Lemmas.front().Text == text) {
                Lemmas[text].Articles.push_back(a);
            }
        } else {
            CommonArticles.push_back(a);
        }
        return false;
    }
};

TGztLemma TInputSymbol::GetGazetteerForm(const TDynBitMap& ctx) const {
    TVector<NGzt::TArticlePtr> articlesWithoutLemma;
    if (!GztArticles.empty()) {
        TGztLemmas lemmas;
        TraverseArticles(ctx, TGetGztLemmas(lemmas, articlesWithoutLemma, true));
        if (!lemmas.empty()) {
            return lemmas.front().Add(articlesWithoutLemma);
        }
    }

    if (!Children.empty())
        return NSymbol::GetGazetteerForm(Children, Contexts);

    return TGztLemma(GetNormalizedText()).Add(articlesWithoutLemma);
}

TGztLemmas TInputSymbol::GetAllGazetteerForms(const TDynBitMap& ctx) const {
    TGztLemmas lemmas;
    TVector<NGzt::TArticlePtr> articlesWithoutLemma;
    if (!GztArticles.empty())
        TraverseArticles(ctx, TGetGztLemmas(lemmas, articlesWithoutLemma, false));

    if (lemmas.empty()) {
        if (!Children.empty()) {
            lemmas = NSymbol::GetAllGazetteerForms(Children, Contexts);
        } else {
            lemmas.push_back(TGztLemma(GetNormalizedText()));
            lemmas.back().Add(articlesWithoutLemma);
        }
    } else if (!articlesWithoutLemma.empty()) {
        for (TGztLemmas::iterator i = lemmas.begin(); i != lemmas.end(); ++i) {
            i->Add(articlesWithoutLemma);
        }
    }
    return lemmas;
}

template <class TResHolder>
struct TGetLemmas {
    TResHolder& Res;
    TGetLemmas(TResHolder& r)
        : Res(r)
    {
    }

    inline bool operator ()(const TWtringBuf& lemma) const {
        if (!lemma.empty()) {
            Res.Put(ToWtring(lemma));
            return !Res.AcceptMore();
        }
        return false;
    }
};

TUtf16String TInputSymbol::GetNominativeForm(const TDynBitMap& ctx) const {
    if (!Children.empty()) {
        return NSymbol::GetNominativeForm(Children, Contexts);
    } else {
        NRemorph::TSingleResultHolder<TUtf16String> res;
        TraverseLemmas(ctx, TGetLemmas<NRemorph::TSingleResultHolder<TUtf16String>>(res));
        return res.Result.empty() ? GetNormalizedText() : res.Result;
    }
}

struct TCheckGrammemFunctor {
    size_t Offset;
    NGleiche::TGleicheFunc Gleiche;
    const TGramBitSet& Grammar;
    TDynBitMap& MyCtx;

    TCheckGrammemFunctor(size_t offset, NGleiche::TGleicheFunc f, const TGramBitSet& g, TDynBitMap& myCtx)
        : Offset(offset)
        , Gleiche(f)
        , Grammar(g)
        , MyCtx(myCtx)
    {
    }

    Y_FORCE_INLINE bool operator () (const TGramBitSet& g, size_t pos) const {
        return Gleiche(g, Grammar).any() ? MyCtx.Set(Offset + pos), true : false;
    }
};

bool TInputSymbol::CheckGrammems(NGleiche::TGleicheFunc f, const TGramBitSet& grammar, TDynBitMap& ctx, bool& fullMatch) const {
    TCheckGrammemFunctor checker(GetGztArticles().size(), f, grammar, ctx);
    return IterGrammems(checker, fullMatch);
}

struct TAgreeGrammemFunctor {
    size_t Offset;
    NGleiche::TGleicheFunc Gleiche;
    TDynBitMap& MyCtx;
    const TInputSymbol& Target;
    TDynBitMap& TargetCtx;
    const bool EmptyMyCtx;
    TDynBitMap ComulativeTargetCtx;
    const bool EmptyTargetCtx;

    // Contexts contain full info: gazetteer and grammar sub-contexts
    // Empty sub-context is equal to full one in order to cover the case when grammars were not checked
    TAgreeGrammemFunctor(size_t offset, NGleiche::TGleicheFunc f, TDynBitMap& myCtx, const TInputSymbol& t, TDynBitMap& targetCtx)
        : Offset(offset)
        , Gleiche(f)
        , MyCtx(myCtx)
        , Target(t)
        , TargetCtx(targetCtx)
        , EmptyMyCtx(myCtx.Size() == (offset ? myCtx.NextNonZeroBit(offset - 1) : myCtx.FirstNonZeroBit()))
        , EmptyTargetCtx(t.GetFirstGrammarCtxBit(targetCtx) >= targetCtx.Size())
    {
    }

    ~TAgreeGrammemFunctor() {
        if (EmptyTargetCtx) {
            // If grammar context is empty then fill it by the match context
            TargetCtx |= ComulativeTargetCtx;
        } else {
            // If grammar context is not empty then apply intersection of both contexts
            // Fill lower bits in order to keep gazetteer sub-context unchanged
            ComulativeTargetCtx.Set(0, Target.GetGztArticles().size());
            TargetCtx &= ComulativeTargetCtx;
        }
    }

    bool operator () (const TGramBitSet& g, size_t pos) {
        if (EmptyMyCtx || MyCtx.Test(Offset + pos)) {
            TDynBitMap matchCtx;
            bool fullMatch;
            // Additionally verify that matched grammems are within the target context
            const bool res = Target.CheckGrammems(Gleiche, g, matchCtx, fullMatch) && (EmptyTargetCtx || TargetCtx.HasAny(matchCtx));
            if (res) {
                ComulativeTargetCtx |= matchCtx;
                if (EmptyMyCtx) {
                    MyCtx.Set(Offset + pos);
                }
            } else if (!EmptyMyCtx) {
                // Reset bit for grammem, which has no match in the target symbol
                MyCtx.Reset(Offset + pos);
            }
            return res;
        }
        return false;
    }
};

bool TInputSymbol::AgreeGrammems(NGleiche::TGleicheFunc f, TDynBitMap& myCtx, const TInputSymbol& target, TDynBitMap& targetCtx) const {
    bool fullMatch;
    TAgreeGrammemFunctor checker(GetGztArticles().size(), f, myCtx, target, targetCtx);
    return IterGrammems(checker, fullMatch);
}

NSpike::TGrammarBunch TInputSymbol::GetAllGram(size_t lemmaIndex) const {
    NSpike::TGrammarBunch gb;
    const ILemmas& lemmas = GetLemmas();
    if (lemmaIndex < lemmas.GetLemmaCount()) {
        TVector<TGramBitSet> flexGrams;
        for (size_t flexGramIndex = 0; flexGramIndex < lemmas.GetFlexGramCount(lemmaIndex); ++flexGramIndex) {
            flexGrams.push_back(lemmas.GetFlexGram(lemmaIndex, flexGramIndex));
        }
        NSpike::ToGrammarBunch(lemmas.GetStemGram(lemmaIndex), flexGrams, gb);
    }
    return gb;
}

void TInputSymbol::ExpungeChildrenGztArticles(const TDynBitMap& ctx) {
    if (!Children.size()) {
        return;
    }

    Y_ASSERT(MainOffset < Children.size());
    Y_ASSERT(MainOffset < Contexts.size());
    TInputSymbol& headChild = *Children[MainOffset];
    TDynBitMap& headContext = Contexts[MainOffset];

    size_t bit = 0;
    Y_FOR_EACH_BIT(headBit, headContext) {
        if (!ctx.Test(bit)) {
            headContext.Reset(headBit);
        }
        ++bit;
    }

    headChild.ExpungeChildrenGztArticles(headContext);
}

} // NSymbol
