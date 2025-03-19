#include "richtreebuilderaux.h"
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/language.h>

namespace {
    class TLemmasSorter: private TNonCopyable{
    public:
        struct LCmp {
            bool operator () (const TYandexLemma* a, const TYandexLemma* b) {
                if (a->GetTokenPos() != b->GetTokenPos())
                    return a->GetTokenPos() < b->GetTokenPos();
                return a->GetTokenSpan() > b->GetTokenSpan();
            }
        };
        typedef TVector<const TYandexLemma*> TVLms;
    private:
        struct TLemmaInfo {
            size_t Begin;
            size_t End;
            size_t Num;
            bool Killed;

            TLemmaInfo(size_t beg, size_t end, size_t i, bool flKilled)
                : Begin(beg)
                , End(end)
                , Num(i)
                , Killed(flKilled)
            {}
            TLemmaInfo()
                : Begin(0)
                , End(0)
                , Num(0)
                , Killed(false)
            {}

        };
        typedef std::pair<size_t, size_t> TSzPair;
        typedef TMap<TSzPair, TVLms> TLMap;
    private:
        TLMap Map;
    private:
        static const TVLms& Empty() {
            static const TVLms empty;
            return empty;
        }
        static size_t GetBegin(const TYandexLemma* li) {
            return li->GetTokenPos();
        }
        static size_t GetEnd(const TYandexLemma* li) {
            return li->GetTokenPos() + li->GetTokenSpan();
        }
        TSzPair GetBounds(const TYandexLemma* li, const TWideToken& wt) {
            size_t beg = wt.SubTokens[GetBegin(li)].Pos;
            size_t end = wt.SubTokens[GetEnd(li) - 1].EndPos() + wt.SubTokens[GetEnd(li) - 1].SuffixLen;
            return TSzPair(beg, end);
        }

        static void SortLemmas(TVLms& lms) {
            std::sort(lms.begin(), lms.end(), LCmp());
            TStack<TLemmaInfo> st;
            for (size_t i = 0; i < lms.size(); ++i) {
                if (lms[i]->GetTokenSpan() == 1) // just a little optimization
                    continue;
                size_t beg = GetBegin(lms[i]);
                size_t end = GetEnd(lms[i]);
                bool flKill = !st.empty() && st.top().Killed
                    && beg == st.top().Begin && end == st.top().End;
                while (!st.empty() && end > st.top().End) {
                    if (beg < st.top().End)
                        flKill = true;
                    st.pop();
                }

                st.push(TLemmaInfo(beg, end, i, flKill));
                if (flKill)
                    lms.erase(lms.begin() + i--);
            }
        }
    public:
        TLemmasSorter(const TWideToken& wt, const TWLemmaArray& lemmas) {
            TVLms lms;
            for (TWLemmaArray::const_iterator i = lemmas.begin(); i != lemmas.end(); ++i)
                lms.push_back(&*i);

            SortLemmas(lms);

            for (TVLms::const_iterator i = lms.begin(); i != lms.end(); ++i)
                Map[GetBounds(*i, wt)].push_back(*i);
        }

        const TVLms& operator () (size_t begin, size_t end) const {
            TLMap::const_iterator i = Map.find(TSzPair(begin, end));
            if (i != Map.end())
                return i->second;
            return Empty();
        }
        bool exist(size_t begin, size_t end) const {
            return Map.find(TSzPair(begin, end)) != Map.end();
        }
    };

// @todo it would be better to use AnalyzeWord() called by TWordInstance, it can be implemented without collecting TWideToken
//       because it already exists when multitoken node is created, now there is a problem with this function - it does not copy
//       suffixes and left/right joins are not assigned

    class TMultitokenLemmatizer: private TNonCopyable {
        const TWideToken& Multitoken;
        TWLemmaArray Lemmas;
    public:
        TMultitokenLemmatizer(const TWideToken& multitoken, const TLanguageContext& lang);
        const TWLemmaArray& GetLemmas() const {
            return Lemmas;
        }
    private:
        void CoverUncovered(bool covered[]);
        void AddDefaultLemmas(const TWideToken& wt, size_t tokenPos);
        TWideToken GetSubToken(size_t i) const;
        void RepairSuffixes();
    };

    TMultitokenLemmatizer::TMultitokenLemmatizer(const TWideToken& multitoken, const TLanguageContext& lang)
        : Multitoken(multitoken)
    {
        NLemmer::TAnalyzeWordOpt opt("", NLemmer::SfxOnly, NLemmer::MtnSplitAllPossible, NLemmer::AccFoundling);
        opt.MaxTokensInCompound = 1;
        NLemmer::AnalyzeWord(Multitoken, Lemmas, lang.GetLangMask(), lang.GetLangOrder(), opt);

        //copypaste (from kernel/lemmer/core/wordinstance.cpp) rulezzz
        for (auto& lemma : Lemmas) {
            if (lang.GetDisabledLanguages().SafeTest(lemma.GetLanguage())) {
                NLemmerAux::TYandexLemmaSetter setter(lemma);
                setter.SetQuality(TYandexLemma::QFoundling | TYandexLemma::QDisabled | (lemma.GetQuality() & TYandexLemma::QFix));
            }
        }

        bool covered[MAX_SUBTOKENS] = {0};
        for (size_t i = 0; i < Lemmas.size(); ++i) {
            if (Lemmas[i].GetTokenSpan() == 1)
                covered[Lemmas[i].GetTokenPos()] = true;
        }

        CoverUncovered(covered);
        RepairSuffixes();
    }

    void TMultitokenLemmatizer::CoverUncovered(bool covered[])  {
        for (size_t i = 0; i < Multitoken.SubTokens.size(); ++i) {
            if (!covered[i]) {
                TWideToken wt = GetSubToken(i);
                AddDefaultLemmas(wt, i);
            }
        }
    }

    TWideToken TMultitokenLemmatizer::GetSubToken(size_t i) const {
        Y_ASSERT(i < Multitoken.SubTokens.size());

        size_t len = Multitoken.SubTokens[i].Len;
        if (i == Multitoken.SubTokens.size() - 1)
            len = Multitoken.Leng - Multitoken.SubTokens[i].Pos;

        TTokenStructure mtt;
        mtt.push_back(Multitoken.SubTokens[i]);
        mtt.back().Pos = 0;

        return TWideToken(Multitoken.Token + Multitoken.SubTokens[i].Pos, len, mtt);
    }

    void TMultitokenLemmatizer::AddDefaultLemmas(const TWideToken& wt, size_t tokenPos)  {
        NLemmer::TAnalyzeWordOpt opt("", NLemmer::SfxOnly, NLemmer::MtnSplitAll, NLemmer::AccFoundling);
        TWLemmaArray lemmasInt;
        NLemmer::AnalyzeWord(wt, lemmasInt, TLangMask(), nullptr, opt);
        Lemmas.reserve(Lemmas.size() + lemmasInt.size());
        for (size_t j = 0; j < lemmasInt.size(); ++j) {
            NLemmerAux::TYandexLemmaSetter(lemmasInt[j]).SetToken(tokenPos, 1);
            Lemmas.push_back(lemmasInt[j]);
        }
    }

    void TMultitokenLemmatizer::RepairSuffixes()  {
        for (size_t i = 0; i < Lemmas.size(); ++i) {
            size_t n = Lemmas[i].GetTokenPos() + Lemmas[i].GetTokenSpan() - 1;
            if (!Lemmas[i].GetSuffixLength() && Multitoken.SubTokens[n].SuffixLen)
                NLemmerAux::TYandexLemmaSetter(Lemmas[i]).AddSuffix(Multitoken.Token + Multitoken.SubTokens[n].EndPos(), Multitoken.SubTokens[n].SuffixLen);
        }
    }


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
    class TSpanVector: private TVector<TCharSpan>, TNonCopyable {
    public:
        explicit TSpanVector(const TRichRequestNode& node) {
            size_t totalLength = CollectSpans(node.Children);
            Y_ASSERT(node.GetText().empty() || totalLength == node.GetText().length()
                || totalLength == node.GetText().length() + node.Children.back()->GetTextAfter().length());
        }

        TTokenStructure GetAndReplaceMultispan(size_t first, size_t num) {
            Y_ASSERT(first + num <= size());
            TTokenStructure tst;
            const size_t beg = GetMultispanBegin(first);
            for (size_t i = 0; i < num; ++i) {
                tst.push_back((*this)[first + i]);
                tst.back().Pos -= beg;
            }
            (*this)[first] = TCharSpan(GetMultispanBegin(first), GetMultispanEnd(first, num) - GetMultispanBegin(first));
            erase(begin() + first + 1, begin() + first + num);
            return tst;
        }

        size_t GetMultispanBegin(size_t first) const {
            Y_ASSERT(first < size());
            return (*this)[first].Pos;
        }

        size_t GetMultispanEnd(size_t first, size_t num) const {
            Y_ASSERT(first + num <= size());
            return (*this)[first + num - 1].EndPos();
        }

    private:
        size_t CollectSpans(const TVector<TRichNodePtr>& nodes) {
            reserve(nodes.size());
            size_t totalLength = 0;
            for (size_t i = 0; i < nodes.size(); ++i) {
                const TRichRequestNode& childNode = *nodes[i];
                push_back(childNode.GetTokSpan());
                back().Pos += totalLength;
                totalLength += (childNode.GetText().length() + childNode.GetTextAfter().length());
            }
            return totalLength;
        }
    };
} // namespace

namespace NRichTreeBuilder {
    class TLemmerMultitokenDeployer: private TNonCopyable{
    private:
        const TLemmasSorter& Sorter;
        const TCreateTreeOptions& Options;
        const TFormType FormType;
    public:
        TLemmerMultitokenDeployer(TRichNodePtr& richNode, TRichNodePtr& parent, const TLemmasSorter& sorter, const TCreateTreeOptions& options)
            : Sorter(sorter)
            , Options(options)
            , FormType(richNode->GetFormType())
        {
            DeployMtTree(richNode, parent, 0);
        }

    private:
        bool DeployMtTree(TRichNodePtr& node, TRichNodePtr& parent, size_t base);
        TRichNodePtr CreateMultitokenNode(const TRichNodePtr &node, size_t first, size_t num, TSpanVector &tkStr);
        bool CreateWordInfo(TRichNodePtr &node, size_t base);
        void ToSynonyms(TRichNodePtr& parent, TRichNodePtr& node);
    };

    bool TLemmerMultitokenDeployer::DeployMtTree(TRichNodePtr& node, TRichNodePtr& parent, size_t base) {
        if (CreateWordInfo(node, base)) {
            node->SetPhraseType(PHRASE_NONE);
            node->OpInfo.Op = oUnused;
            if (!node->Children.empty()) {
                node->SetTextBefore(node->Children[0]->GetTextBefore());
                node->SetTextAfter(node->Children.back()->GetTextAfter());
                ToSynonyms(parent, node);
            }
            return true;
        }

        //lemmer generated no lemma for this token
        if (node->Children.empty()) {
            return false;
        }
        node->SetTextBefore(node->Children[0]->GetTextBefore());
        node->SetTextAfter(node->Children.back()->GetTextAfter());

        TSpanVector tkStr(*node);

        for (size_t i = 0; i < node->Children.size();) {
            for (size_t num = node->Children.size() - (i ? i : 1); num > 1; --num) {
                if (Sorter.exist(tkStr.GetMultispanBegin(i) + base, tkStr.GetMultispanEnd(i, num) + base)) {
                    TRichNodePtr nd = CreateMultitokenNode(node, i, num, tkStr);
                    node->ReplaceChildren(i, num, nd);
                    break;
                }
            }
            if (!DeployMtTree(node->Children.MutableNode(i), node, tkStr.GetMultispanBegin(i) + base)) {
                node->Children.Remove(i, i + 1);
            } else {
                ++i;
            }
        }
        node->SetPhrase(PHRASE_PHRASE);
        node->OpInfo = DefaultQuoteOpInfo;
        return true;
    }

    bool TLemmerMultitokenDeployer::CreateWordInfo(TRichNodePtr &node, size_t base) {
        size_t beg = node->GetTokSpan().Pos;
        size_t end = node->GetText().length() ? (node->GetText().length() - node->GetTokSpan().Pos) : node->GetTokSpan().EndPos();
        const TLemmasSorter::TVLms& lms = Sorter(beg + base, end + base);
        if (lms.empty())
            return false;
        node->WordInfo.Reset(TWordNode::CreateLemmerNode(lms, FormType, Options.Lang, true).Release());
        node->SetStopWordHiliteMode();
        return true;
    }

    TRichNodePtr TLemmerMultitokenDeployer::CreateMultitokenNode(const TRichNodePtr &node, size_t first, size_t num, TSpanVector &tkStr) {
        TRichNodePtr nd (TTreeCreator::NewNode(*node, node->Children[first]->Necessity, node->Children[first]->GetFormType()));
        if (num > 0 && first < node->Children.size()) {
            nd->Children.Append(node->Children[first]);
            for (size_t i = 1; i < num; ++i)
                nd->Children.Append(node->Children[first + i], node->Children.ProxBefore(first + i));
        }
        TTokenStructure tst = tkStr.GetAndReplaceMultispan(first, num);
        nd->SetMultitoken(TUtf16String(node->GetText(), tkStr.GetMultispanBegin(first), tst.back().EndPos()), tst, node->GetPhraseType());
        return nd;
    }

    void TLemmerMultitokenDeployer::ToSynonyms(TRichNodePtr& parent, TRichNodePtr& node) {
        TRichNodePtr ndWhl (TTreeCreator::NewNode(*node, node->Necessity, node->GetFormType()));
        if (!!node->GetText())
            ndWhl->SetLeafWord(node->GetText(), node->GetTokSpan());
        else
            ndWhl->SetLeafWord(node->WordInfo->GetNormalizedForm(), node->GetTokSpan());
        ndWhl->OpInfo = node->OpInfo;
        ndWhl->WordInfo.Reset(node->WordInfo.Release());
        node->MiscOps.swap(ndWhl->MiscOps);

        TRichNodePtr syn;
        if (Options.MakeSynonymsForMultitokens) {
            node->SetPhrase(PHRASE_PHRASE);
            node->OpInfo = DefaultQuoteOpInfo;
            syn = node;
        }

        node = ndWhl;
        if (!!syn)
            parent->AddMarkup(*node, new TSynonym(syn, TE_NONE, 0, EQUAL_BY_STRING));
    }

    void ReplaceMultitokenSubtree(TRichNodePtr& node, TRichNodePtr& parent, const TCreateTreeOptions& options) {
        const TWideToken& tok = node->GetMultitoken();
        Y_ASSERT(!tok.SubTokens.empty());
        TMultitokenLemmatizer lemmatizer(tok, options.Lang);
        TLemmasSorter sorter(tok, lemmatizer.GetLemmas());

        TLemmerMultitokenDeployer(node, parent, sorter, options);
    }
}
