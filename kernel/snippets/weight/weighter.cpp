#include "weighter.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/dynamic_data/host_stats.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/plm/plm.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/read_helper/read_helper.h>
#include <kernel/snippets/schemaorg/factor_calcer/schemaorg_factor_calcer.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/smartcut/snip_length.h>
#include <kernel/snippets/title_trigram/title_trigram.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <kernel/snippets/wordstat/wordstat.h>
#include <kernel/snippets/wordstat/wordstat_data.h>

#include <kernel/web_factors_info/factor_names.h>

#include <library/cpp/string_utils/url/url.h>

namespace NSnippets
{
    namespace {
        template <class T>
        double SafeDiv(const T& a, const T& b) {
            return b ? double(a) / double(b) : 0.0;
        }

        template <class T>
        void SetMin(T& a, const T& b, const bool first = false) {
            if (first)
                a = b;
            else
                a = Min(b, a);
        }

        template <class T>
        void SetMax(T& a, const T& b) {
            a = Max(b, a);
        }

        template <class T>
        void FillAllPosAndPercent(T& allPos, T& percent, int allCount, int seenCount) {
            allPos = (allCount == seenCount);
            percent = SafeDiv(seenCount, allCount);
        }

        class TDependentFactorsResolver {
            TVector<ui32> WebFactors;
        public:
            template<typename TFactorInfo>
            explicit TDependentFactorsResolver(const TFactorInfo* factors, size_t count) {
                WebFactors.reserve(count);
                for (size_t i = 0; i < count; i++) {
                    const auto& dependentSlices = factors[i].DependsOn.DependentSlices;
                    Y_VERIFY(dependentSlices.size() == 1);
                    auto dependentSlice0 = dependentSlices.begin();
                    Y_VERIFY(dependentSlice0->first == TStringBuf("web_production"));
                    Y_VERIFY(dependentSlice0->second.size() == 1);
                    const TString& featureName = dependentSlice0->second[0];
                    size_t index;
                    Y_VERIFY(GetWebFactorsInfo()->GetFactorIndex(featureName.data(), &index));
                    WebFactors.push_back(index);
                }
            }

            void Fill(const TConfig& cfg, TFactorView& factors) const {
                Y_ASSERT(factors.Size() == WebFactors.size());
                for (unsigned i = 0; i < WebFactors.size(); i++)
                    cfg.GetRankingFactor(WebFactors[i], factors[i]);
            }
        };

        // for use in Singleton<>
        class TWebFactorsResolver : public TDependentFactorsResolver {
        public:
            TWebFactorsResolver() : TDependentFactorsResolver(NSnippetsWeb::GetFactorsInfo(), NSnippetsWeb::FI_FACTOR_COUNT) {}
        };

        class TWebV1FactorsResolver : public TDependentFactorsResolver {
        public:
            TWebV1FactorsResolver() : TDependentFactorsResolver(NSnippetsWebV1::GetFactorsInfo(), NSnippetsWebV1::FI_FACTOR_COUNT) {}
        };

        class TWebNoclickFactorsResolver : public TDependentFactorsResolver {
        public:
            TWebNoclickFactorsResolver() : TDependentFactorsResolver(NSnippetsWebNoclick::GetFactorsInfo(), NSnippetsWebNoclick::FI_FACTOR_COUNT) {}
        };
    }

    struct TFactorsCalcer::TImpl {

        class TStaticFactorsCalcer {
            private:
                using TFactorValue = std::pair<unsigned, float>;
                TVector<TFactorValue> Values;

            public:
                TStaticFactorsCalcer(const TString& url, const TConfig& cfg) {
                    TStringBuf host;
                    try {
                        host = GetHost(url);
                    } catch (...) {
                    }
                    if (const THostStats* hostStats = cfg.GetHostStats()) {
                        Values.emplace_back(A2_FQ_HOST_WEIGHT, hostStats->GetHostWeight(host));
                    }
                }
                void FillStatic(const TConfig& cfg, TFactorStorage& factorStorage) const;
        };

        const TSentsMatchInfo& Info;
        TTitleMatchInfo TitleInfo;
        const TConfig& Cfg;
        const TWordSpanLen& WordSpanLen;
        TWordStat WordStat;
        TPLMStatData PLMStatData;
        TVector<TSingleSnip> SnipParts;
        const float MaxLenForLPFactors;
        TStaticFactorsCalcer StaticFactorsCalcer;
        const TSchemaOrgQuestionInfo SchemaOrgQuestionInfo;
        bool IgnoreWidthOfScreen;

    public:
        TImpl(const TSentsMatchInfo& info, const TConfig& cfg, const TString& url, const TWordSpanLen& wordSpanLen, const TSnipTitle* title, float maxLenForLPFactors, const TSchemaOrgArchiveViewer* schemaOrgViewer, bool ignoreWidthOfScreen)
          : Info(info)
          , Cfg(cfg)
          , WordSpanLen(wordSpanLen)
          , WordStat(info.Query, info, title ? &title->GetTitleSnip() : nullptr)
          , PLMStatData(info)
          , SnipParts()
          , MaxLenForLPFactors(maxLenForLPFactors)
          , StaticFactorsCalcer(url, cfg)
          , SchemaOrgQuestionInfo(schemaOrgViewer)
          , IgnoreWidthOfScreen(ignoreWidthOfScreen)
        {
            TitleInfo.Fill(Info, title);
        }

        bool AllWordsSeenInTitle(int first, int last) const;
        void CalcWordSpan(TFactorView& x, const TSpan& w);
        void CalcWordSpans(TFactorView& x, const TSpans& spans);
        void CalcWordStat(TFactorView& x, const TSpans& spans);
        void CalcSentSpan(TFactorView& x, const TSpan& s);
        void CalcSentSpans(TFactorView& x, const TSpans& sents);
        void CalcSymSpans(TFactorView& x, const TSpans& wSpans);
        void CalcSchemaOrgFactors(TFactorView& x, const TSpans& ws);
    };

    void TFactorsCalcer::TImpl::TStaticFactorsCalcer::FillStatic(const TConfig& cfg, TFactorStorage& factorStorage) const {
        TFactorView x = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::SNIPPETS_MAIN);
        for (const TFactorValue& factorValue : Values) {
            x[factorValue.first] = factorValue.second;
        }
        TFactorView webV1 = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::SNIPPETS_WEBRANKING_V1);
        if (webV1.Size()) // beware: "if (webV1)" would compile but would be always true
            Singleton<TWebV1FactorsResolver>()->Fill(cfg, webV1);
        TFactorView web = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::SNIPPETS_WEBRANKING);
        if (web.Size())
            Singleton<TWebFactorsResolver>()->Fill(cfg, web);
        TFactorView webNoClick = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::SNIPPETS_WEBRANKING_NOCLICK);
        if (webNoClick.Size())
            Singleton<TWebNoclickFactorsResolver>()->Fill(cfg, webNoClick);
    }

    TFactorsCalcer::TFactorsCalcer(const TSentsMatchInfo& info, const TConfig& cfg, const TString& url, const TWordSpanLen& wordSpanLen, const TSnipTitle* title, float maxLenForLPFactors, const TSchemaOrgArchiveViewer* schemaOrgViewer, bool ignoreWidthOfScreen)
      : Impl(new TImpl(info, cfg, url, wordSpanLen, title, maxLenForLPFactors, schemaOrgViewer, ignoreWidthOfScreen))
    {
    }

    TFactorsCalcer::~TFactorsCalcer() {
    }

    const TWordStat& TFactorsCalcer::GetWordStat() const {
        return Impl->WordStat;
    }
    inline double lenX(double mid, double rLow, double rHigh, double x, bool leftmost, bool rightmost) {
        x -= mid;
        if (leftmost && x < 0 || rightmost && x > 0)
            return 1;
        if (x < 0)
            x = -x;
        if (x < rLow)
            return 1;
        if (x < rHigh)
            return (rHigh - x) / (rHigh - rLow);
        return 0;
    }

    void TFactorsCalcer::TImpl::CalcSentSpan(TFactorView& x, const TSpan& s) {
        const int& si = s.First;
        const int& sj = s.Last;

        const int navlikeSents = Info.NavlikeInSentRange(si, sj);
        const double navlikePenalty =
            navlikeSents > 2
            ? navlikeSents * 3.75
            : 0.0;
        x[A2_NAV] -= navlikePenalty;
        x[A2_NAV2] -= navlikeSents * 3.0;

        x[A2_QUAL2] += (Info.SumQualityInSentRange(si, sj)) * 0.1;

        const double perc = Info.SumAnchorPercentInSentRange(si, sj);
        const double lowAnchorsPercentPenalty = (perc > 0.6 ? 0.0 : perc) * 3.75;
        const double highAnchorsPercentPenalty = (perc > 0.6 ? perc : 0.0) * 3.75;
        x[A2_LANCHP] -= lowAnchorsPercentPenalty;
        x[A2_HANCHP] -= highAnchorsPercentPenalty;

        SetMax(x[A2_INFO], float(Info.GetInfoBonusSentsInRange(si, sj) ? 0.1 : 0.0));
        x[A2_INFO2] += Info.GetInfoBonusSentsInRange(si, sj) * 0.1;

        SetMax(x[A2_MTCH], float(Info.GetInfoBonusMatchSentsInRange(si, sj) ? 0.07 : 0.0));
        x[A2_MTCH2] += Info.GetInfoBonusMatchSentsInRange(si, sj) * 0.07;

        //2х фрагментых НПС не бывает - ну дa ладно
        const int uporn = Info.GetUnmatchedByLinkPornCountInRange(Info.SentsInfo.FirstWordIdInSent(si), Info.SentsInfo.LastWordIdInSent(sj));
        x[A2_LUPORN] -= (uporn > 0 ? 1 : 0);

        SetMin(x[A2_HAS_ADS], -float(!!Info.AdsSentsInRange(si, sj)));
        SetMax(x[A2_HAS_HEADER], float(!!Info.HeaderSentsInRange(si, sj)));
        SetMin(x[A2_HAS_POLL], -float(!!Info.PollSentsInRange(si, sj)));
        SetMin(x[A2_HAS_MENU], -float(!!Info.MenuSentsInRange(si, sj)));

        if (Info.SentsInfo.GetArchiveSent(si).SourceArc != ARC_LINK) {
            const int paraW = Info.SentsInfo.SentVal[si].ParaLenInWords;
            const double quality = 0.1 * Info.SumQualityInSentRange(si, sj) / s.Len();
            const double paraBeginBonus =
                (Info.SentsInfo.SentVal[si].ParaLenInSents + 1) / 2 + (paraW >= 13) + (paraW >= 20 && (paraW < 42 || quality > 1E-3)) >= 3
                ? 0.1 / (Info.SentsInfo.SentVal[si].OffsetInPara + 1.0)
                : 0.0;
            x[A2_PBEG] += paraBeginBonus;
        }

        x[A2_MF_FOOTER] += Info.FooterSentsInRange(si, sj);
        x[A2_MF_TEXT_AREA] += Info.TextAreaSentsInRange(si, sj);
    }

    bool TFactorsCalcer::TImpl::AllWordsSeenInTitle(int first, int last) const {
        const TWordStatData& wordStatTit = WordStat.ZeroData(true);
        for (int word = first; word <= last; ++word) {
            for (int lemmaId : Info.GetNotExactMatchedLemmaIds(word)) {
                for (int pos : Info.Query.Id2Poss[lemmaId]) {
                    if (!wordStatTit.SeenLikePos[pos]) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void TFactorsCalcer::TImpl::CalcWordSpan(TFactorView& x, const TSpan& w) {
        const int& i = w.First;
        const int& j = w.Last;

        const int strangeGaps = Info.StrangeGapsInRange(i, j);
        const double sGapsPenalty =
            strangeGaps > 3
            ? strangeGaps * 0.015
            : 0.0;
        x[A2_SGAPS] -= sGapsPenalty;

        const int trashInGaps = Info.TrashInGapsInRange(i, j);
        const double tGapsPenalty =
            trashInGaps > 2
            ? trashInGaps * 0.025
            : 0.0;
        x[A2_TGAPS] -= tGapsPenalty;

        x[A2_SHORTS] -= Info.ShortsInRange(i, j) * 0.06;
        x[A2_EXCT] += Info.ExactMatchesInRange(i, j);
        SetMax(x[A2_SEQ], float(Info.GetLongestMatchChainInRange(i, j)));

        // SNIPPETS-1479
        if (j - i + 1 <= 4) {
            if (AllWordsSeenInTitle(i, j)) {
                x[A2_USELESS_PESS] = 1.0;
            }
        }
    }

    void TFactorsCalcer::TImpl::CalcSentSpans(TFactorView& x, const TSpans& sents) {
        if (sents.empty()) {
            return; //x should be zero-filled before - so fine
        }

        const TSpan& f = sents.front();
        const TSpan& l = sents.back();

        size_t len = 0;

        const NSegments::TSegmentsInfo* segmInfo = Info.SentsInfo.GetSegments();
        using namespace NSegments;
        NSegm::TSegmentSpan super;
        float l_localLinkPct(0);
        float u_localLinkPct(0);
        float l_linkWordsPct(0);
        float u_linkWordsPct(0);
        bool first = true;
        bool hasSegData = segmInfo && segmInfo->HasData();
        TSegmentCIt prevSegment = nullptr;
        ui32 wordssofar = 0;
        float maxWeight = -std::numeric_limits<float>::max();

        for (TSpanCIt s = sents.begin(); s != sents.end(); ++s) {
            CalcSentSpan(x, *s);
            len += s->Len();
            x[A2_SENT] -= 0.2 * (1 + !!(s->Len() > 3));
            x[A2_QUAL] += Info.SumQualityInSentRange(s->First, s->Last);

            if (hasSegData) {
                if (first)
                    prevSegment = segmInfo->SegmentsEnd();

                for (int k = s->First; k <= s->Last; ++k) {
                    x[A2_FQ_IS_DEFINITION] += Info.SentLooksLikeDefinition(k) ? 1.0 : 0.0;

                    TSegmentCIt curSegment = segmInfo->GetArchiveSegment(Info.SentsInfo.GetArchiveSent(k));
                    if (segmInfo->IsValid(curSegment)) {
                        if (curSegment != prevSegment) {
                            SetMin(l_localLinkPct, curSegment->AvLocalLinksPerLink() * curSegment->AvLinkWordsPerWord(), first);
                            SetMin(l_linkWordsPct, curSegment->AvLinkWordsPerWord(), first);
                            SetMax(u_localLinkPct, curSegment->AvLocalLinksPerLink() * curSegment->AvLinkWordsPerWord());
                            SetMax(u_linkWordsPct, curSegment->AvLinkWordsPerWord());
                            super.MergeNext(*curSegment);
                            prevSegment = curSegment;
                            first = false;

                            if (curSegment->Weight > maxWeight) {
                                x[A2_SEG_WORDS_PER_SYMBOL] = SafeDiv(curSegment->Words, curSegment->SymbolsInText);
                                x[A2_SEG_TITLE_WORDS] = curSegment->TitleWords;
                                x[A2_SEG_BLOCKS] = curSegment->Blocks;
                                x[A2_SEG_BLOCKS_PER_WORD] = SafeDiv((ui16)curSegment->Blocks, curSegment->Words);
                                x[A2_SEG_COMMENTS_CSS] = curSegment->CommentsCSS;
                                x[A2_SEG_MIDDLE_WORD] = wordssofar + curSegment->Words / 2;
                                x[A2_SEG_COMMAS_PER_SYMBOL] = SafeDiv(curSegment->CommasInText, curSegment->SymbolsInText);
                                x[A2_SEG_SPACES_PER_SYMBOL] = SafeDiv(curSegment->SpacesInText, curSegment->SymbolsInText);
                                x[A2_SEG_ALPHAS_PER_SYMBOL] = SafeDiv(curSegment->AlphasInText, curSegment->SymbolsInText);
                                x[A2_SEG_HEADERS_PER_SENT] = SafeDiv((ui32)curSegment->HeadersCount, curSegment->Sentences());
                                x[A2_SEG_WORDS] = curSegment->Words;
                                x[A2_SEG_LOCAL_LINKS_PER_LINK] = SafeDiv(curSegment->LocalLinks, curSegment->Links);
                                maxWeight = curSegment->Weight;
                            }
                            wordssofar += curSegment->Words;
                        }
                    }
                }
            }
        }
        x[A2_QUAL] *= 0.1 / (len ? len : 1);

        x[A2_LOCAL_LINKS_LPCT] -= l_localLinkPct;
        x[A2_LOCAL_LINKS_UPCT] -= u_localLinkPct;
        x[A2_LOCAL_LINKS_AVPCT] -= super.AvLocalLinksPerLink() * super.AvLinkWordsPerWord();
        x[A2_DOMAINS] -= super.Domains;
        x[A2_INPUTS] -= super.Inputs;
        x[A2_BLOCKS] -= super.Blocks;
        x[A2_LINK_WORDS_PCT_L] -= l_linkWordsPct;
        x[A2_LINK_WORDS_PCT_U] -= u_linkWordsPct;
        x[A2_LINK_WORDS_PCT_AV] -= super.AvLinkWordsPerWord();


        if (Info.SentsInfo.GetArchiveSent(f.First).SourceArc != ARC_LINK) {
            x[A2_END] = -l.Last * 1E-5;
            x[A2_LEDGE] = f.First <= Info.GetFirstMatchSentId() && f.Last >= Info.GetFirstMatchSentId() ? -0.01 : 0.0;
            x[A2_REDGE] = l.First <= Info.GetLastMatchSentId() && l.Last >= Info.GetLastMatchSentId() ? -0.017 : 0.0;
        }

        x[A2_SENT2] = -0.15 * len;

        ESentsSourceType sourceType = Info.SentsInfo.SentVal[f.First].SourceType;

        x[A2_IS_META_DESCR] = (sourceType == SST_META_DESCR) ? 1.0 : 0.0;
        x[A2_IS_DMOZ] = (sourceType == SST_DMOZ || sourceType == SST_DMOZ_WITH_TITLE) ? 1.0 : 0.0;
        x[A2_IS_YACA] = (sourceType == SST_YACA || sourceType == SST_YACA_WITH_TITLE) ? 1.0 : 0.0;
        x[A2_HAS_CUSTOM_TITLE] = (sourceType == SST_YACA_WITH_TITLE || sourceType == SST_DMOZ_WITH_TITLE) ? 1.0 : 0.0;
    }

    void TFactorsCalcer::TImpl::CalcSchemaOrgFactors(TFactorView& x, const TSpans& wSpans) {
        if (!SchemaOrgQuestionInfo.SchemaOrgViewer || !SchemaOrgQuestionInfo.Question ||
            !SchemaOrgQuestionInfo.BestAnswer)
        {
            return;
        }

        x[A2_FQ_SCHEMA_IS_QUESTION] = 1.0;
        x[A2_FQ_SCHEMA_HAS_APPROVED_ANSWER] = SchemaOrgQuestionInfo.Question->HasApprovedAnswer() ? 1.0 : 0.0;

        const TAnswerInfo* bestAns = SchemaOrgQuestionInfo.BestAnswer.Get();
        TAnswerMatchFactors bestAnsFactors = bestAns->CalcMatchFactors(Info, wSpans);

        x[A2_FQ_SCHEMA_BEST_ANS_WORD_COUNT] = bestAnsFactors.WordCount;
        x[A2_FQ_SCHEMA_BEST_ANS_UPVOTE_COUNT] = bestAnsFactors.UpvoteCount;
        x[A2_FQ_SCHEMA_BEST_ANS_MAX_SPAN_LCSWC_DIV_SPAN_WC] = bestAnsFactors.MaxSpanMatchRatioInSnip;
        x[A2_FQ_SCHEMA_BEST_ANS_MAX_SPAN_LCSWC_DIV_ANS_WC] = bestAnsFactors.MaxSpanMatchRatioInAnswer;
        x[A2_FQ_SCHEMA_BEST_ANS_LCSW_POS_RATIO_IN_SNIP] = bestAnsFactors.FirstMatchPosRatioInSnip;
        x[A2_FQ_SCHEMA_BEST_ANS_LCSW_POS_RATIO_IN_ANSWER] = bestAnsFactors.FirstMatchPosRatioInAnswer;

        for (const TAnswerInfo& ans : SchemaOrgQuestionInfo.Answers) {
            TAnswerMatchFactors ansFactors = ans.CalcMatchFactors(Info, wSpans);
            if (ansFactors.MaxSpanMatchRatioInSnip > bestAnsFactors.MaxSpanMatchRatioInSnip + 1e-6) {
                bestAnsFactors = ansFactors;
            }
        }
        x[A2_FQ_SCHEMA_MATCHED_ANS_WORD_COUNT] = bestAnsFactors.WordCount;
        x[A2_FQ_SCHEMA_MATCHED_ANS_UPVOTE_COUNT] = bestAnsFactors.UpvoteCount;
        x[A2_FQ_SCHEMA_MATCHED_ANS_MAX_SPAN_LCSWC_DIV_SPAN_WC] = bestAnsFactors.MaxSpanMatchRatioInSnip;
        x[A2_FQ_SCHEMA_MATCHED_ANS_MAX_SPAN_LCSWC_DIV_ANS_WC] = bestAnsFactors.MaxSpanMatchRatioInAnswer;
        x[A2_FQ_SCHEMA_MATCHED_ANS_LCSW_POS_RATIO_IN_SNIP] = bestAnsFactors.FirstMatchPosRatioInSnip;
        x[A2_FQ_SCHEMA_MATCHED_ANS_LCSW_POS_RATIO_IN_ANSWER] = bestAnsFactors.FirstMatchPosRatioInAnswer;

    }

    static void FillBagFactors(TFactorView& x, const TQueryy& query, const TVector<int>& seenLikePos, int leastShift, int avgLeastShift) {
        TQueryy::TBag lp;
        for (const int& i : seenLikePos) {
            lp.Set(i);
        }
        TVector<double> v;
        v.reserve(Min(query.UserPosCount(), 1));
        for (int i = 0; i < query.PosCount(); ++i) {
            if (!query.Positions[i].IsUserWord) {
                continue;
            }
            double w = lp.Get(i) ? 2000.0 : -2000.0;
            for (const TQueryy::TBagWeight& j : query.Positions[i].Bags) {
                if (lp.HasAll(j.first)) {
                    const double y = j.second ? j.second : 1001.0;
                    if (y > w) {
                        w = y;
                    }
                }
            }
            v.push_back(w);
        }
        if (v.empty()) {
            v.push_back(-2000.0);
        }
        Sort(v.begin(), v.end());
        for (size_t i = 0; i < FC_ALGO2_BAG; ++i) {
            x[leastShift + i] = v[Min(v.size() - 1, i)];
            x[avgLeastShift + i] = v[(v.size() - 1) * (i + 1) / FC_ALGO2_BAG];
        }
    }

    void TFactorsCalcer::TImpl::CalcWordStat(TFactorView& x, const TSpans& spans) {
        for (TSpanCIt it = spans.begin(); it != spans.end(); ++it) {
            if (it == spans.begin()) {
                WordStat.SetSpan(it->First, it->Last);
            } else {
                WordStat.AddSpan(it->First, it->Last);
            }
        }

        const TWordStatData& wordStat = WordStat.Data();
        const TWordStatData& wordStatWTit = WordStat.Data(true);
        const TWordStatData& wordStatTit = WordStat.ZeroData(true);
        const TQueryy& q = wordStat.Query;

        FillBagFactors(x, q, wordStat.SeenLikePos, A2_BAG_LEAST_1, A2_BAG_AVGLEAST_1);
        FillBagFactors(x, q, wordStatWTit.SeenLikePos, A2_WTIT_BAG_LEAST_1, A2_WTIT_BAG_AVGLEAST_1);
        FillBagFactors(x, q, wordStatTit.SeenLikePos, A2_TIT_BAG_LEAST_1, A2_TIT_BAG_AVGLEAST_1);

        x[A2_IDF] = wordStat.SumPosIdfNorm;
        x[A2_UIDF] = wordStat.SumUserPosIdfNorm;
        x[A2_WIDF] = wordStat.SumWizardPosIdfNorm;
        x[A2_WTIT_IDF] = wordStatWTit.SumPosIdfNorm;
        x[A2_WTIT_UIDF] = wordStatWTit.SumUserPosIdfNorm;
        x[A2_WTIT_WIDF] = wordStatWTit.SumWizardPosIdfNorm;
        x[A2_TIT_IDF] = wordStatTit.SumPosIdfNorm;
        x[A2_TIT_UIDF] = wordStatTit.SumUserPosIdfNorm;
        x[A2_TIT_WIDF] = wordStatTit.SumWizardPosIdfNorm;

        x[A2_DOM2] = wordStat.GetForcedPosSeenPercent();
        x[A2_WTIT_DOM2] = wordStatWTit.GetForcedPosSeenPercent();
        x[A2_TIT_DOM2] = wordStatTit.GetForcedPosSeenPercent();

        x[A2_USERWS] = wordStat.WordSeenCount.StopUser;
        x[A2_USERWNS] = wordStat.WordSeenCount.NonstopUser;
        x[A2_USERLS] = wordStat.LemmaSeenCount.StopUser;
        x[A2_USERLNS] = wordStat.LemmaSeenCount.NonstopUser;
        x[A2_USERLWS] = wordStat.LikeWordSeenCount.StopUser;
        x[A2_USERLWNS] = wordStat.LikeWordSeenCount.NonstopUser;

        x[A2_WIZWS] = wordStat.WordSeenCount.StopWizard;
        x[A2_WIZWNS] = wordStat.WordSeenCount.NonstopWizard;
        x[A2_WIZLS] = wordStat.LemmaSeenCount.StopWizard;
        x[A2_WIZLNS] = wordStat.LemmaSeenCount.NonstopWizard;
        x[A2_WIZLWS] = wordStat.LikeWordSeenCount.StopWizard;
        x[A2_WIZLWNS] = wordStat.LikeWordSeenCount.NonstopWizard;

        x[A2_WTIT_USERWS] = wordStatWTit.WordSeenCount.StopUser;
        x[A2_WTIT_USERWNS] = wordStatWTit.WordSeenCount.NonstopUser;
        x[A2_WTIT_USERLS] = wordStatWTit.LemmaSeenCount.StopUser;
        x[A2_WTIT_USERLNS] = wordStatWTit.LemmaSeenCount.NonstopUser;
        x[A2_WTIT_USERLWS] = wordStatWTit.LikeWordSeenCount.StopUser;
        x[A2_WTIT_USERLWNS] = wordStatWTit.LikeWordSeenCount.NonstopUser;

        x[A2_WTIT_WIZWS] = wordStatWTit.WordSeenCount.StopWizard;
        x[A2_WTIT_WIZWNS] = wordStatWTit.WordSeenCount.NonstopWizard;
        x[A2_WTIT_WIZLS] = wordStatWTit.LemmaSeenCount.StopWizard;
        x[A2_WTIT_WIZLNS] = wordStatWTit.LemmaSeenCount.NonstopWizard;
        x[A2_WTIT_WIZLWS] = wordStatWTit.LikeWordSeenCount.StopWizard;
        x[A2_WTIT_WIZLWNS] = wordStatWTit.LikeWordSeenCount.NonstopWizard;

        x[A2_TIT_USERWS] = wordStatTit.WordSeenCount.StopUser;
        x[A2_TIT_USERWNS] = wordStatTit.WordSeenCount.NonstopUser;
        x[A2_TIT_USERLS] = wordStatTit.LemmaSeenCount.StopUser;
        x[A2_TIT_USERLNS] = wordStatTit.LemmaSeenCount.NonstopUser;
        x[A2_TIT_USERLWS] = wordStatTit.LikeWordSeenCount.StopUser;
        x[A2_TIT_USERLWNS] = wordStatTit.LikeWordSeenCount.NonstopUser;

        x[A2_TIT_WIZWS] = wordStatTit.WordSeenCount.StopWizard;
        x[A2_TIT_WIZWNS] = wordStatTit.WordSeenCount.NonstopWizard;
        x[A2_TIT_WIZLS] = wordStatTit.LemmaSeenCount.StopWizard;
        x[A2_TIT_WIZLNS] = wordStatTit.LemmaSeenCount.NonstopWizard;
        x[A2_TIT_WIZLWS] = wordStatTit.LikeWordSeenCount.StopWizard;
        x[A2_TIT_WIZLWNS] = wordStatTit.LikeWordSeenCount.NonstopWizard;

        x[A2_REP2] = wordStat.RepeatedWordSeen >= 3 ? wordStat.RepeatedWordSeen * -0.04 : 0.0;
        x[A2_WTIT_REP2] = wordStatWTit.RepeatedWordSeen >= 3 ? wordStatWTit.RepeatedWordSeen * -0.04 : 0.0;
        x[A2_TIT_REP2] = wordStatTit.RepeatedWordSeen >= 3 ? wordStatTit.RepeatedWordSeen * -0.04 : 0.0;

        x[A2_GOOD] = -wordStat.RepeatedLemmaSeen * 0.13;
        x[A2_UBM25] = wordStat.GetUserBM25();
        x[A2_UNIQUE_WORDS] = wordStat.UniqueWords;
        x[A2_SAME_WORDS] = wordStat.Words - wordStat.UniqueWords;
        x[A2_CNT] = WordStat.GetSpansCount();
        x[A2_DIFF_LEMM_POS] = wordStat.LemmaSeenCount.GetTotal() + wordStat.WordSeenCount.GetTotal();
        x[A2_INTERS_LEMM_POS] = wordStat.FreqLemmaSeen + wordStat.FreqPosSeen;
        x[A2_REPEAT_PESS] = wordStat.RepeatedWordsScore > 0 ? wordStat.RepeatedWordsScore : 0;

        x[A2_WTIT_GOOD] = -wordStatWTit.RepeatedLemmaSeen * 0.13;
        x[A2_WTIT_UBM25] = wordStatWTit.GetUserBM25();
        x[A2_WTIT_UNIQUE_WORDS] = wordStatWTit.UniqueWords;
        x[A2_WTIT_SAME_WORDS] = wordStatWTit.Words - wordStatWTit.UniqueWords;
        x[A2_WTIT_DIFF_LEMM_POS] = wordStatWTit.LemmaSeenCount.GetTotal() + wordStatWTit.WordSeenCount.GetTotal();
        x[A2_WTIT_INTERS_LEMM_POS] = wordStatWTit.FreqLemmaSeen + wordStatWTit.FreqPosSeen;

        x[A2_TIT_GOOD] = -wordStatTit.RepeatedLemmaSeen * 0.13;
        x[A2_TIT_UBM25] = wordStatTit.GetUserBM25();
        x[A2_TIT_UNIQUE_WORDS] = wordStatTit.UniqueWords;
        x[A2_TIT_SAME_WORDS] = wordStatTit.Words - wordStatTit.UniqueWords;
        x[A2_TIT_DIFF_LEMM_POS] = wordStatTit.LemmaSeenCount.GetTotal() + wordStatTit.WordSeenCount.GetTotal();
        x[A2_TIT_INTERS_LEMM_POS] = wordStatTit.FreqLemmaSeen + wordStatTit.FreqPosSeen;

        {
            const TSeenCount& seenCount = wordStat.LikeWordSeenCount;
            FillAllPosAndPercent(x[A2_HAS_ALL_NS_UPOS], x[A2_SEEN_NS_UPOS_PCT], q.NonstopUserPosCount(), seenCount.NonstopUser);
            FillAllPosAndPercent(x[A2_HAS_ALL_UPOS], x[A2_SEEN_UPOS_PCT], q.UserPosCount(), seenCount.GetUserCount());
            FillAllPosAndPercent(x[A2_HAS_ALL_NS_WPOS], x[A2_SEEN_NS_WPOS_PCT], q.NonstopWizardPosCount(), seenCount.NonstopWizard);
            FillAllPosAndPercent(x[A2_HAS_ALL_WPOS], x[A2_SEEN_WPOS_PCT], q.WizardPosCount(), seenCount.GetWizardCount());
            FillAllPosAndPercent(x[A2_HAS_ALL_NS_POS], x[A2_SEEN_NS_POS_PCT], q.NonstopPosCount(), seenCount.GetNonstopCount());
            FillAllPosAndPercent(x[A2_HAS_ALL_POS], x[A2_SEEN_POS_PCT], q.PosCount(), seenCount.GetTotal());
        }

        {
            const TSeenCount& seenCount = wordStatWTit.LikeWordSeenCount;
            FillAllPosAndPercent(x[A2_WTIT_HAS_ALL_NS_UPOS], x[A2_WTIT_SEEN_NS_UPOS_PCT], q.NonstopUserPosCount(), seenCount.NonstopUser);
            FillAllPosAndPercent(x[A2_WTIT_HAS_ALL_UPOS], x[A2_WTIT_SEEN_UPOS_PCT], q.UserPosCount(), seenCount.GetUserCount());
            FillAllPosAndPercent(x[A2_WTIT_HAS_ALL_NS_WPOS], x[A2_WTIT_SEEN_NS_WPOS_PCT], q.NonstopWizardPosCount(), seenCount.NonstopWizard);
            FillAllPosAndPercent(x[A2_WTIT_HAS_ALL_WPOS], x[A2_WTIT_SEEN_WPOS_PCT], q.WizardPosCount(), seenCount.GetWizardCount());
            FillAllPosAndPercent(x[A2_WTIT_HAS_ALL_NS_POS], x[A2_WTIT_SEEN_NS_POS_PCT], q.NonstopPosCount(), seenCount.GetNonstopCount());
            FillAllPosAndPercent(x[A2_WTIT_HAS_ALL_POS], x[A2_WTIT_SEEN_POS_PCT], q.PosCount(), seenCount.GetTotal());
        }

        {
            const TSeenCount& seenCount = wordStatTit.LikeWordSeenCount;
            FillAllPosAndPercent(x[A2_TIT_HAS_ALL_NS_UPOS], x[A2_TIT_SEEN_NS_UPOS_PCT], q.NonstopUserPosCount(), seenCount.NonstopUser);
            FillAllPosAndPercent(x[A2_TIT_HAS_ALL_UPOS], x[A2_TIT_SEEN_UPOS_PCT], q.UserPosCount(), seenCount.GetUserCount());
            FillAllPosAndPercent(x[A2_TIT_HAS_ALL_NS_WPOS], x[A2_TIT_SEEN_NS_WPOS_PCT], q.NonstopWizardPosCount(), seenCount.NonstopWizard);
            FillAllPosAndPercent(x[A2_TIT_HAS_ALL_WPOS], x[A2_TIT_SEEN_WPOS_PCT], q.WizardPosCount(), seenCount.GetWizardCount());
            FillAllPosAndPercent(x[A2_TIT_HAS_ALL_NS_POS], x[A2_TIT_SEEN_NS_POS_PCT], q.NonstopPosCount(), seenCount.GetNonstopCount());
            FillAllPosAndPercent(x[A2_TIT_HAS_ALL_POS], x[A2_TIT_SEEN_POS_PCT], q.PosCount(), seenCount.GetTotal());
        }
    }

    void TFactorsCalcer::TImpl::CalcWordSpans(TFactorView& x, const TSpans& spans) {
        size_t len = 0;
        size_t charLen = 0;
        size_t shortestFragmentCharLen = 0;
        bool first = true;
        int goodWords = 0;
        int langMatchs = 0;

        SnipParts.clear();
        SnipParts.reserve(spans.size());
        bool hasByLink = false;
        for (TSpanCIt it = spans.begin(); it != spans.end(); ++it) {
            CalcWordSpan(x, *it);
            len += it->Len();
            const int& i = it->First;
            const int& j =  it->Last;
            if (Info.SentsInfo.GetArchiveSent(Info.SentsInfo.WordId2SentId(i)).SourceArc == ARC_LINK) {
                hasByLink = true;
            }
            SnipParts.push_back(TSingleSnip(i, j, Info));
            size_t charLenAdd = Info.SentsInfo.GetWordSpanBuf(i, j).size();
            charLen += charLenAdd;
            SetMin(shortestFragmentCharLen, charLenAdd, first);
            first = false;
            x[A2_FOOTER_PCT] -= Info.GetFooterWords(i, j);
            x[A2_CONTENT_PCT] += Info.GetContentWords(i, j);
            x[A2_MCONTENT_PCT] += Info.GetMainContentWords(i, j);
            x[A2_SEG_WEIGHT_SUM] += Info.GetSegmentWeightSums(i, j);
            x[A2_HEADER_PCT] += Info.GetHeaderWords(i, j);
            x[A2_MHEADER_PCT] += Info.GetMainHeaderWords(i, j);
            x[A2_MENU_PCT] -= Info.GetMenuWords(i, j);
            x[A2_REFERAT_PCT] += Info.GetReferatWords(i, j);
            x[A2_AUX_PCT] -= Info.GetAuxWords(i, j);
            x[A2_LINKS_PCT] -= Info.GetLinksWords(i, j);
            x[A2_MF_KANDZI] += Info.GetKandziWords(i, j);
            x[A2_FQ_SNIP_WEIGHT] += Info.GetAnswerWeight(i, j);
            const float telOrg = Cfg.BoostTelephoneCandidates() ? Info.TelephonesInRange(i, j) > 0 : 0;
            SetMax(x[A2_MF_TELORG], telOrg);
            goodWords += Info.MatchesInRange(i, j);
            langMatchs += Info.LangMatchsInRange(i, j);
            double doPessimizeTitleLike = TitleInfo.GetTitleLikeness(i, j);
            SetMax(x[A2_MF_TITLELIKE], static_cast<float>(doPessimizeTitleLike));
        }
        int badWords = len - goodWords;
        if (len > 0) {
            x[A2_FOOTER_PCT] /= len;
            x[A2_CONTENT_PCT] /= len;
            x[A2_MCONTENT_PCT] /= len;
            x[A2_SEG_WEIGHT_SUM] /= len;
            x[A2_HEADER_PCT] /= len;
            x[A2_MHEADER_PCT] /= len;
            x[A2_MENU_PCT] /= len;
            x[A2_REFERAT_PCT] /= len;
            x[A2_AUX_PCT] /= len;
            x[A2_LINKS_PCT] /= len;
            x[A2_MF_KANDZI] /= len;
        }

        if (x[A2_MF_KANDZI] < 0.65f)
            x[A2_MF_KANDZI] = 0;

        x[A2_LANGM] = badWords && Info.DocLangId != LANG_UNK
            ? -0.1 * (1.0 - double(langMatchs) / double(badWords))
            : 0.0;
        x[A2_LLANGM] = x[A2_LANGM] * hasByLink;

        x[A2_GOOD_RATIO] = true &&
                            badWords <= goodWords * 1.7 &&
                            goodWords <= badWords * 1.7 &&
                            len >= 8;
        x[A2_BAD_RATIO] = badWords < goodWords / 1.75;

        x[A2_SHORTEST_FRAGMENT] = SafeDiv(shortestFragmentCharLen, charLen);

        if (!hasByLink) {
            float maxSize = MaxLenForLPFactors;
            double lp;
            if (!IgnoreWidthOfScreen && Cfg.GetSnipCutMethod() == TCM_PIXEL) {
                const double realLen = WordSpanLen.CalcLength(SnipParts);
                x[A2_REAL_LEN] = realLen;
                x[A2_LINES_COUNT] = ceil(x[A2_REAL_LEN]);
                x[A2_LAST_STRING_LEN] = (x[A2_REAL_LEN] > 0 ? 1.0 + x[A2_REAL_LEN] - x[A2_LINES_COUNT] : 0);
                lp = realLen / double(maxSize);
            } else {
                lp = double(charLen) / double(maxSize);
            }
            x[A2_LEN_0] = lenX(0, 0.1, 0.1, lp, 1, 0);
            x[A2_LEN_50] = lenX(0.2, 0.1, 0.1, lp, 0, 0);
            x[A2_LEN_100] = lenX(0.4, 0.1, 0.1, lp, 0, 0);
            x[A2_LEN_150] = lenX(0.6, 0.1, 0.1, lp, 0, 0);
            x[A2_LEN_200] = lenX(0.8, 0.1, 0.1, lp, 0, 0);
            x[A2_LEN_250] = lenX(1, 0.1, 0.2, lp, 0, 1);
            x[A2_LEN] = ((-2.9333 * lp + 3.4) * lp + 0.0333) * lp - 0.5;
            x[A2_LENP] = lp;
        }
        x[A2_QUERYLEN] = Info.Query.UserPosCount();
        if (Cfg.CalculatePLM())
            x[A2_PLM_LIKE] = PLMStatData.CalculateWeightSum(spans);
    }

    void TFactorsCalcer::TImpl::CalcSymSpans(TFactorView& x, const TSpans& wSpans) {
        TReadabilityHelper readHelper(Cfg.UseTurkishReadabilityThresholds());
        TSpanCIt wIt = wSpans.begin();
        float shareOfCyrAlpha = 0;
        for (/**/; wIt != wSpans.end(); ++wIt) {
            const int i = wIt->First;
            const int j = wIt->Last;
            readHelper.AddRange(Info, i, j);
            shareOfCyrAlpha += Info.CyrAlphasInRange(i, j);
        }
        if (Cfg.PessimizeCyrForTr() && !Info.Query.CyrillicQuery) {
            shareOfCyrAlpha = (readHelper.SymLen > 0 ? shareOfCyrAlpha / readHelper.SymLen : 0);
            x[A2_PESSIMIZE_CYR] = (shareOfCyrAlpha < 0.1 ? 0 : 1);
        } else {
            x[A2_PESSIMIZE_CYR] = 0;
        }

        x[A2_SHARE_OF_ALPHA] = (readHelper.SymLen > 0 ? (double)readHelper.Alpha / readHelper.SymLen : 0);
        x[A2_SHARE_OF_DIGIT] = (readHelper.SymLen > 0 ? (double)readHelper.Digit / readHelper.SymLen : 0);
        x[A2_SHARE_OF_PUNCT] = (readHelper.Words > 0 ? (double)readHelper.Punct / readHelper.Words : 0);
        x[A2_PUNCT_BAL] = readHelper.PunctBal;
        x[A2_SHARE_ASCII_TRASH] = (readHelper.SymLen > 0 ? (double)readHelper.TrashAscii / readHelper.SymLen : 0);
        x[A2_SHARE_UTF_TRASH] = (readHelper.SymLen > 0 ? (double)readHelper.TrashUTF / readHelper.SymLen : 0);
        x[A2_SLASH] = readHelper.Slash;
        x[A2_VERT] = readHelper.Vert;
        x[A2_TELEPHONES] = readHelper.Phones;
        x[A2_DATES] = readHelper.Dates;

        if (Cfg.UseTurkey() && !Cfg.TrashPessTr()) {
            x[A2_MF_READABLE] = 0;
            return;
        }

        double val = readHelper.CalcReadability();
        x[A2_MF_READABLE] = (val < 1e-5 ? 0 : val);
    }

    void TFactorsCalcer::CalcAll(TFactorStorage& factorStorage, const TSpans& ws, const TSpans& ss) {
        Y_ASSERT(ws.size() == ss.size());
        Y_ASSERT(!ss.empty());
        Impl->StaticFactorsCalcer.FillStatic(Impl->Cfg, factorStorage);
        TFactorView x = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::SNIPPETS_MAIN);
        TSpans sents;
        double tsimPenalty = 0.0;
        for (TSpanCIt w = ws.begin(), s = ss.begin(); w != ws.end(); ++w, ++s) {
            const int& i = w->First;
            const int& j = w->Last;
            x[A2_DOTS] -= double(!Impl->Info.SentsInfo.IsWordIdFirstInSent(i)) * 1.5;
            x[A2_RDOTS] -= double(!Impl->Info.SentsInfo.IsWordIdLastInSent(j)) * 1.5;
            for (int k = s->First; k <= s->Last; ++k) {
                const int sentLen = Impl->Info.SentsInfo.GetSentLengthInWords(k);
                int sstart = Impl->Info.SentsInfo.FirstWordIdInSent(k);
                if (sstart < i)
                   sstart = i;
                int send = sstart + sentLen - 1;
                if (j < send)
                    send = j;

                const double t = Impl->TitleInfo.GetTitleSimilarity(k) * double(send - sstart + 1) / double(sentLen);
                tsimPenalty += t * t * t * 0.7;
            }

            if (!sents.empty() && sents.back().Intersects(*s)) {
                sents.back().Merge(*s);
            } else {
                sents.push_back(*s);
            }
        }
        x[A2_TSIM] = -tsimPenalty;

        Impl->CalcWordSpans(x, ws);
        Impl->CalcSentSpans(x, sents);
        Impl->CalcWordStat(x, ws);
        Impl->CalcSymSpans(x, ws);
        Impl->CalcSchemaOrgFactors(x, ws);
        x[A2_MCONTENT_BOOST] = 0;
        if (Impl->Cfg.UseMContentBoost() && x[A2_MCONTENT_PCT] > 0.99f &&
            Impl->WordStat.Data().LikeWordSeenCount.NonstopUser >= Impl->WordStat.Data().Query.NonstopUserPosCount()) {
            x[A2_MCONTENT_BOOST] = 1.f;
        }

        for (TSpanCIt w = ws.begin(); w != ws.end(); ++w) {
            if (Impl->Info.RegionMatchInRange(w->First, w->Last)) {
                x[A2_REGION_WORDS] = 1;
                break;
            }
        }
    }

    TWeighter::TWeighter(const TSentsMatchInfo& info, const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSnipTitle* title, float maxLenForLPFactors, const TString& url, const TSchemaOrgArchiveViewer* schemaOrgViewer, bool isFactSnippet)
      : Formula(isFactSnippet ? cfg.GetFactSnippetTotalFormula() : cfg.GetTotalFormula())
      , Info(info)
      , WSpans()
      , SSpans()
      , FCalc(info, cfg, url, wordSpanLen, title, maxLenForLPFactors, schemaOrgViewer, isFactSnippet)
      , FStore(Formula.GetFactorDomain())
    {
    }

    void TWeighter::SetFragment(const TSentMultiword& i, const TSentMultiword& j) {
        WSpans.push_back(TSpan(i.FirstWordId(), j.LastWordId()));
        SSpans.push_back(TSpan(i.GetSent(), j.GetSent()));
    }

    void TWeighter::SetFragment(int i, int j) {
        WSpans.push_back(TSpan(i, j));
        SSpans.push_back(TSpan(Info.SentsInfo.WordId2SentId(i), Info.SentsInfo.WordId2SentId(j)));
    }

    void TWeighter::Clear() {
        WSpans.clear();
        SSpans.clear();
    }

    TFactorStorage& TWeighter::Next() {
        if (IsFull()) {
            FStore.Reset();
        }
        return FStore.Next();
    }

    void TWeighter::CalcAll() {
        FCalc.CalcAll(this->Next(), WSpans, SSpans);
    }

}

