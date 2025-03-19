#include "two_span.h"
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/hits/topn.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/snip_builder/snip_builder.h>
#include <kernel/snippets/uni_span_iter/uni_span_iter.h>
#include <kernel/snippets/weight/weighter.h>
#include <kernel/snippets/wordstat/wordstat.h>
#include <kernel/snippets/wordstat/wordstat_data.h>


namespace NSnippets
{
    namespace NSnipWordSpans
    {
        struct TWeightedSingleFrag {
            double Weight;
            std::pair<int, int> WordRange;
            TDynBitMap UsefullWordsBSet;
            TFactorStorage Factors; // may be not filled

            TWeightedSingleFrag(double weight, int first, int last, const TVector<bool> &seenLikePos, const TQueryy &query, const TFactorStorage& factors)
              : Weight(weight)
              , WordRange(first, last)
              , UsefullWordsBSet(GenUsefullWordsBSet(seenLikePos, query))
              , Factors(factors)
            {
            }

            inline int GetFirst() const {
                return WordRange.first;
            }
            inline int GetLast() const {
                return WordRange.second;
            }

            struct TPtrWeightGreater {
                bool operator()(const TWeightedSingleFrag* a, const TWeightedSingleFrag* b) const {
                    return a->Weight > b->Weight;
                }
            };
        };
        typedef TVector<std::pair<int, int>> TCandSpans;
        typedef TVector<const TWeightedSingleFrag*> TCand;

        struct TWeightedCandArr {
            double Weights[TFactorStorage128::MAX];
            TCandSpans Cands[TFactorStorage128::MAX];
        };

        struct TWeightedSingleFragPtrHash {
            hash<std::pair<int, int>> Impl;
            size_t operator()(const TWeightedSingleFrag* twf) const {
                return Impl(twf->WordRange);
            }
        };

        struct TWeightedSingleFragPtrEqual {
            bool operator()(const TWeightedSingleFrag* a, const TWeightedSingleFrag* b) const {
                return a->WordRange == b->WordRange;
            }
        };

        class TGetTwoPlusSnip::TImpl {
            typedef TMultiTop<const TWeightedSingleFrag*, TWeightedSingleFrag::TPtrWeightGreater, double,
                TWeightedSingleFragPtrHash, TWeightedSingleFragPtrEqual> TMT;
        private:
            const TConfig& Cfg;
            TMxNetWeighter& Weighter;
            ECandidateSource Source;
            TUniSpanIter &SpanSet;
            const TSentsMatchInfo &SMInfo;
            ISnippetCandidateDebugHandler *Callback;
            bool DontGrow;
            bool NeedCheckTitle;
            float RepeatedTitlePessimizeFactor;

            const TVector<double> PosWeights;
            TList<TWeightedSingleFrag> LQw;
            TMT Qw;
            double BestCandWeight;
            TCandSpans BestCandSpans;
            TFactorStorage BestCandFactors;
            TWeightedCandArr WCands;
            TVector<const TWeightedSingleFrag*> SingleFrags;
            TVector<const TWeightedSingleFrag*> SingleFragsFor4Frags;

        private:
            void Reset();
            void ProcessCombiner(IAlgoTop* topNpairs);
            TSnip GetSnipFromFrags(float maxSize, float maxPartSize);

            void AddToCombiner(IAlgoTop *topNpairs, const TCandSpans& cand);

            void GetSingleFrags(float maxSize, float maxPartSize, int fragCnt, TString algoName);
            bool JustCheck3FragSnip(const TVector<TDynBitMap> &snipBitSet);
            bool JustCheck4FragSnip(const TVector<TDynBitMap> &snipBitSet);
            TSnip GetTwoSnippets(float maxSize, float maxPartSize);
            TSnip GetThreeSnippets(float maxSize, float maxPartSize, const TVector<TDynBitMap> &snipBitSet);
            TSnip GetFourSnippets(float maxSize, float maxPartSize, const TVector<TDynBitMap> &snipBitSet);
            void GrowMultiSpan(TSnipBuilder& b, TCandSpans& cand) const;
        public:
            TImpl(const TConfig& cfg, TMxNetWeighter& weighter, TUniSpanIter &spanSet, const TSentsMatchInfo &sMInfo,
                ECandidateSource source, ISnippetCandidateDebugHandler *callback, bool dontGrow, bool needCheckTitle, float repeatedTitlePessimizeFactor);
            TSnip GetSnip(float maxSize, float maxPartSize, size_t fragCnt, const TVector<TDynBitMap> &snipBitSet);
        };

        typedef TUniSpanIter::TWordpos TWordpos;

        struct TFragCmp {
            bool operator() (const TWeightedSingleFrag* a, const TWeightedSingleFrag* b) const {
                return a->GetFirst() < b->GetFirst();
            };
        };

        inline void FillCand(TCand& cand, const TWeightedSingleFrag* f1 = nullptr, const TWeightedSingleFrag* f2 = nullptr, const TWeightedSingleFrag* f3 = nullptr, const TWeightedSingleFrag* f4 = nullptr)
        {
            cand.clear();
            if (f1) {
                cand.push_back(f1);
            }
            if (f2) {
                cand.push_back(f2);
            }
            if (f3) {
                cand.push_back(f3);
            }
            if (f4) {
                cand.push_back(f4);
            }
        }

        inline void TGetTwoPlusSnip::TImpl::GrowMultiSpan(TSnipBuilder& b, TCandSpans& cand) const
        {
            // reset snippet builder
            b.Reset();

            // add fragments
            for (size_t i = 0; i < cand.size(); ++i) {
                b.Add(SMInfo.SentsInfo.WordId2SentWord(cand[i].first), SMInfo.SentsInfo.WordId2SentWord(cand[i].second));
            }

            // grow & glue
            if (!DontGrow) {
                b.GrowLeftToSent();
                while (b.GrowLeftWordInSent()) {
                    continue;
                }
                b.GrowRightToSent();
                while (b.GrowRightWordInSent()) {
                    continue;
                }
            }
            b.GlueTornSents();
            b.GlueSents(SpanSet.GetSkipRestr());

            // fill result
            cand.resize(b.GetPartsSize());
            for (int z = 0; z < b.GetPartsSize(); ++z) {
                cand[z].first = b.GetPartL(z).FirstWordId();
                cand[z].second = b.GetPartR(z).LastWordId();
            }
        }

        inline void Convert(const TCand& frags, TCandSpans& spans)
        {
            spans.clear();
            spans.reserve(frags.size());
            for (size_t i = 0; i < frags.size(); ++i) {
                spans.push_back(frags[i]->WordRange);
            }
        }

        inline bool SortCheckCrossed(TCand& frags)
        {
            Sort(frags.begin(), frags.end(), TFragCmp());
            for (size_t i = 1; i < frags.size(); ++i) {
                if (frags[i]->GetFirst() < frags[i - 1]->GetLast() + 1) {
                    return true;
                }
            }
            return false;
        }

        inline bool IsUniqContent(const TCand& frags, const TVector<TDynBitMap> &snipBitSet)
        {
            if (snipBitSet.empty()) {
                return false;
            }
            TDynBitMap curCandMask;
            curCandMask.Reserve(snipBitSet.begin()->Size());
            for (size_t id = 0; id < frags.size(); ++id) {
                if (frags[id]->UsefullWordsBSet.Empty()) {
                    return false;
                }
                curCandMask |= frags[id]->UsefullWordsBSet;
            }
            for (size_t i = 0; i < snipBitSet.size(); ++i) {
                if ((snipBitSet[i] | curCandMask) == snipBitSet[i]) {
                    return false;
                }
            }

            size_t n = frags.size();
            if (n == 4) {
                for (size_t i = 0; i < n; ++i) {
                    for (size_t j = i + 1; j < n; ++j) {
                        if (frags[i]->UsefullWordsBSet == frags[j]->UsefullWordsBSet) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        inline void TGetTwoPlusSnip::TImpl::Reset()
        {
            Weighter.Reset();
            BestCandWeight = -1e6;
            BestCandSpans.clear();
            BestCandFactors = TFactorStorage();
        }

        inline void TGetTwoPlusSnip::TImpl::ProcessCombiner(IAlgoTop* topNpairs)
        {
            if (Weighter.GetUsed() == 0) {
                return;
            }
            Weighter.BatchCalc(WCands.Weights);
            for (size_t z = 0; z < Weighter.GetUsed(); ++z) {
                if (topNpairs) {
                    TList<TSingleSnip> l;
                    for (size_t fragNum = 0; fragNum < WCands.Cands[z].size(); ++fragNum) {
                        l.push_back(TSingleSnip(WCands.Cands[z][fragNum].first,
                            WCands.Cands[z][fragNum].second, SMInfo));
                    }
                    topNpairs->Push(TSnip(l, WCands.Weights[z], Weighter.GetFactors(z)));
                }

                float candWeight = WCands.Weights[z];
                if (NeedCheckTitle) {
                    // Check first sentence of first fragment
                    if (!WCands.Cands[z].empty() && SMInfo.SentRepeatsTitle(SMInfo.SentsInfo.WordId2SentId(WCands.Cands[z][0].first))) {
                        candWeight -= RepeatedTitlePessimizeFactor;
                    }
                }
                if (candWeight > BestCandWeight) {
                    BestCandWeight = candWeight;
                    BestCandSpans = WCands.Cands[z];
                    BestCandFactors = Weighter.GetFactors(z);
                }
           }
        }

        inline TSnip TGetTwoPlusSnip::TImpl::GetSnipFromFrags(float maxSize, float maxPartSize)
        {
            double weight = BestCandWeight;
            TCandSpans& bCand = BestCandSpans;
            if (bCand.empty()) {
                return TSnip();
            }

            const TSentsInfo &sentsInfo = SMInfo.SentsInfo;
            TSnipBuilder b(SMInfo, SpanSet.GetWordSpanLen(), maxSize, maxPartSize);
            for (size_t curFrag = 0; curFrag < bCand.size(); ++curFrag) {
                b.Add(sentsInfo.WordId2SentWord(bCand[curFrag].first),
                        sentsInfo.WordId2SentWord(bCand[curFrag].second));
            }
            // do we really use BestCandFactors after this and need to copy it?
            TSnip res = b.Get(weight, TFactorStorage(BestCandFactors));
            return res;
        }

        inline void TGetTwoPlusSnip::TImpl::AddToCombiner(IAlgoTop *topNpairs, const TCandSpans& cand)
        {
            // Now the candidate seems to be perfect to weight it!
            Weighter.SetSpans(cand);
            const size_t k = Weighter.GetUsed() - 1;
            WCands.Cands[k] = cand;

            // we just put something, maybe it's time to weight?
            if (Weighter.IsFull()) {
                ProcessCombiner(topNpairs);
            }
        }

        TGetTwoPlusSnip::TImpl::TImpl(const TConfig& cfg, TMxNetWeighter& w, TUniSpanIter &spanSet, const TSentsMatchInfo &sMInfo,
            ECandidateSource source, ISnippetCandidateDebugHandler *callback, bool dontGrow, bool needCheckTitle, float repeatedTitlePessimizeFactor)
            : Cfg(cfg)
            , Weighter(w)
            , Source(source)
            , SpanSet(spanSet)
            , SMInfo(sMInfo)
            , Callback(callback)
            , DontGrow(dontGrow)
            , NeedCheckTitle(needCheckTitle)
            , RepeatedTitlePessimizeFactor(repeatedTitlePessimizeFactor)
            , PosWeights(sMInfo.Query.PosSqueezer->GetSqueezedWeights())
            , Qw(Cfg.Algo3TopLen(), PosWeights,
                    Cfg.IsDumpAllAlgo3Pairs(), Cfg.UseDiversity2())
            , BestCandWeight(-1e6)

        {
        }

        TSnip TGetTwoPlusSnip::TImpl::GetSnip(float maxSize, float maxPartSize, size_t fragCnt,
            const TVector<TDynBitMap> &snipBitSet)
        {
            if (fragCnt == 2) {
                return GetTwoSnippets(maxSize, maxPartSize);
            } else if (fragCnt == 3) {
                if (JustCheck3FragSnip(snipBitSet)) { // we can generate new content from previous Frags
                    return GetThreeSnippets(maxSize, maxPartSize, snipBitSet);
                } else {
                    return TSnip();
                }
            } else if (fragCnt == 4) {
                if (JustCheck4FragSnip(snipBitSet)) {
                    return GetFourSnippets(maxSize, maxPartSize, snipBitSet);
                } else {
                    return TSnip();
                }
            } else {
                return TSnip();
            }
        }

        inline TSnip TGetTwoPlusSnip::TImpl::GetTwoSnippets(float maxSize, float maxPartSize)
        {
            const size_t fragCnt = 2;
            GetSingleFrags(maxSize, maxPartSize, fragCnt, "AlgoPairs_single");
            if (SingleFrags.size() < fragCnt) {
                return TSnip();
            }

            IAlgoTop* const topNpairs = Callback ? Callback->AddTop("Algo3_pairs", Source) : nullptr;
            Reset();

            TSnipBuilder b(SMInfo, SpanSet.GetWordSpanLen(), maxSize, maxPartSize);

            const int n = (int)SingleFrags.size();
            TCand cand;
            TCandSpans spans;
            TVector<TCandSpans> seen;
            seen.reserve(n * (n - 1) / 2);
            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    // performance hack goes first
                    if (i >= Cfg.Hack2FragMax() && j >= Cfg.Hack2FragMax()) {
                        continue;
                    }

                    FillCand(cand, SingleFrags[i], SingleFrags[j]);

                    // first always must be earlier in text
                    if (SortCheckCrossed(cand))
                        continue;

                    Convert(cand, spans);
                    GrowMultiSpan(b , spans);
                    seen.push_back(spans);
                }
            }
            Sort(seen.begin(), seen.end());
            for (size_t i = 0; i < seen.size(); ++i) {
                if (i == 0 || seen[i] != seen[i - 1]) {
                    AddToCombiner(topNpairs, seen[i]);
                }
            }
            ProcessCombiner(topNpairs);
            return GetSnipFromFrags(maxSize, maxPartSize);
        }

        inline TSnip TGetTwoPlusSnip::TImpl::GetThreeSnippets(float maxSize, float maxPartSize, const TVector<TDynBitMap> &snipBitSet)
        {
            const size_t fragCnt = 3;
            GetSingleFrags(maxSize, maxPartSize, fragCnt, "AlgoTriple_single");
            if (SingleFrags.size() < fragCnt) {
                return TSnip();
            }
            IAlgoTop* const topNpairs = Callback ? Callback->AddTop("Algo4_triple", Source) : nullptr;
            Reset();


            const size_t n = SingleFrags.size();
            TSnipBuilder b(SMInfo, SpanSet.GetWordSpanLen(), maxSize, maxPartSize);
            TCand cand;
            TCandSpans spans;
            TVector<TCandSpans> seen;
            seen.reserve(64);
            for (size_t i = 0; i < Min<size_t>(n, 4); ++i) {
                for (size_t j = i + 1; j < Min<size_t>(n, 7); ++j) {
                    for (size_t k = j + 1; k < n; ++k) {
                        FillCand(cand, SingleFrags[i], SingleFrags[j], SingleFrags[k]);

                        if (SortCheckCrossed(cand)) {
                            continue;
                        }
                        if (IsUniqContent(cand, snipBitSet)) {
                            Convert(cand, spans);
                            GrowMultiSpan(b, spans);
                            seen.push_back(spans);
                        }
                    }
                }
            }
            Sort(seen.begin(), seen.end());
            for (size_t i = 0; i < seen.size(); ++i) {
                if (i == 0 || seen[i] != seen[i - 1]) {
                    AddToCombiner(topNpairs, seen[i]);
                }
            }
            ProcessCombiner(topNpairs);
            return GetSnipFromFrags(maxSize, maxPartSize);
        }

        inline TSnip TGetTwoPlusSnip::TImpl::GetFourSnippets(float maxSize, float maxPartSize, const TVector<TDynBitMap> &snipBitSet)
        {
            const size_t fragCnt = 4;
            if (SingleFragsFor4Frags.size() < fragCnt) {
                return TSnip();
            }
            IAlgoTop* const topNpairs = Callback ? Callback->AddTop("Algo5_quadro", Source) : nullptr;
            Reset();

            const size_t n = Min<size_t>(10, SingleFragsFor4Frags.size()); // 10 is just another one magic number

            TSnipBuilder b(SMInfo, SpanSet.GetWordSpanLen(), maxSize, maxPartSize);

            int totalWeight = 0;
            TCand cand;
            TCandSpans spans;
            TVector<TCandSpans> seen;
            seen.reserve(64);
            for (size_t i = 0; i < Min<size_t>(n, 4); ++i) {
                for (size_t j = i + 1; j < Min<size_t>(n, 7); ++j) {
                    for (size_t k = j + 1; k < n; ++k) {
                        for (size_t fr = k + 1; fr < n; ++fr) {
                            // hack
                            if (totalWeight > 200)
                                break;

                            FillCand(cand, SingleFragsFor4Frags[i], SingleFragsFor4Frags[j], SingleFragsFor4Frags[k], SingleFragsFor4Frags[fr]);

                            if (SortCheckCrossed(cand)) {
                                continue;
                            }

                            if (IsUniqContent(cand, snipBitSet)) {
                                ++totalWeight;
                                Convert(cand, spans);
                                GrowMultiSpan(b, spans);
                                seen.push_back(spans);
                            }
                        }
                    }
                }
            }
            Sort(seen.begin(), seen.end());
            for (size_t i = 0; i < seen.size(); ++i) {
                if (i == 0) {
                    AddToCombiner(topNpairs, seen[i]);
                } else {
                    if (seen[i] != seen[i - 1]) {
                        AddToCombiner(topNpairs, seen[i]);
                    }
                }
            }
            ProcessCombiner(topNpairs);
            return GetSnipFromFrags(maxSize, maxPartSize);
        }

        bool TGetTwoPlusSnip::TImpl::JustCheck3FragSnip(const TVector<TDynBitMap> &snipBitSet)
        {
            size_t fragCnt = 3;
            if (SingleFrags.size() < fragCnt) {
                return false;
            }

            TCand curCand;

            const size_t n = Min<size_t>(Cfg.Hack2FragMax(), SingleFrags.size()); // use the same old hack
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    for (size_t k = j + 1; k < n; ++k) {
                        FillCand(curCand, SingleFrags[i], SingleFrags[j], SingleFrags[k]);

                        // Check if this fragments aren't crossed
                        if (SortCheckCrossed(curCand)) {
                            continue;
                        }
                        // Check for uniq words in each fragment
                        if (IsUniqContent(curCand, snipBitSet)) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        bool TGetTwoPlusSnip::TImpl::JustCheck4FragSnip(const TVector<TDynBitMap> &snipBitSet)
        {
            size_t fragCnt = 4;
            SingleFragsFor4Frags = Qw.ToSortedVector(MTFM_EXPLICIT_SHRINK); // use the same fragments which were used for 3-frag, but shrinked
            if (SingleFragsFor4Frags.size() < fragCnt) {
                return false;
            }

            TCand curCand;
            const size_t n = Min((size_t)10, SingleFragsFor4Frags.size()); // use the same old hack
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    for (size_t k = j + 1; k < n; ++k) {
                        for (size_t fr = k + 1; fr < n; ++fr){
                            FillCand(curCand, SingleFragsFor4Frags[i], SingleFragsFor4Frags[j], SingleFragsFor4Frags[k], SingleFragsFor4Frags[fr]);

                            // Check if this fragments aren't crossed
                            if (SortCheckCrossed(curCand)) {
                                continue;
                            }
                            // Check for uniq words in each fragment
                            if (IsUniqContent(curCand, snipBitSet)) {
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }

        inline void TGetTwoPlusSnip::TImpl::GetSingleFrags(float maxSize, float maxPartSize, int fragCnt, TString algoName)
        {
            IAlgoTop* const topNsingle = Callback ? Callback->AddTop(algoName.data(), Source) : nullptr;
            IAlgoTop* const topNmulti = Callback ? Callback->AddTop((algoName + "_multiTop").data(), Source) : nullptr;

            Weighter.Reset();
            Weighter.SetSingleFragmentMode(true);
            Qw.Reset();
            LQw.clear();
            const TSentsInfo& sentsInfo = SMInfo.SentsInfo;

            float partSize = (maxSize / fragCnt < maxPartSize ? maxSize / fragCnt : maxPartSize) * Cfg.ShortMultifragCoef();
            SpanSet.Reset(partSize);
            double ws[TFactorStorage128::MAX];
            TVector<TWordpos> is(TFactorStorage128::MAX, sentsInfo.End<TWordpos>());
            TVector<TWordpos> js(TFactorStorage128::MAX, sentsInfo.End<TWordpos>());
            TVector<bool> origTops[TFactorStorage128::MAX];

            const size_t topsCount = SMInfo.Query.PosSqueezer->GetSqueezedPosCount();

            for (size_t i = 0; i < TFactorStorage128::MAX; ++i) {
                origTops[i].resize(SMInfo.Query.PosCount());
            }
            while (SpanSet.MoveNext()) {
                if (Weighter.IsFull()) {
                    Weighter.BatchCalc(ws);
                    for (size_t z = 0; z < TFactorStorage128::MAX; ++z) {
                        if (topNsingle) {
                            topNsingle->Push(TSnip(TSingleSnip(is[z], js[z], SMInfo), ws[z], Weighter.GetFactors(z)));
                        }
                        LQw.push_back(TWeightedSingleFrag(ws[z], is[z].FirstWordId(), js[z].LastWordId(), origTops[z], SMInfo.Query, Weighter.GetFactors(z)));
                        Qw.PushVector(SMInfo.Query.PosSqueezer->Squeeze(origTops[z]), &LQw.back());
                    }
                }
                const TWordpos w0 = SpanSet.GetFirst();
                const TWordpos w1 = SpanSet.GetLast();
                Weighter.SetSpan(w0, w1);
                const int k = Weighter.GetUsed() - 1;
                is[k] = w0;
                js[k] = w1;

                const TVector<int> &seenLikePos = Weighter.GetWordStat().Data().SeenLikePos;
                for (size_t i = 0; i < seenLikePos.size(); ++i) {
                    origTops[k][i] = seenLikePos[i] > 0;
                }

            }
            if (Weighter.GetUsed() > 0) {
                Weighter.BatchCalc(ws);
                for (size_t z = 0; z < Weighter.GetUsed(); ++z) {
                    if (topNsingle) {
                        topNsingle->Push(TSnip(TSingleSnip(is[z], js[z], SMInfo), ws[z], Weighter.GetFactors(z)));
                    }
                    LQw.push_back(TWeightedSingleFrag(ws[z], is[z].FirstWordId(), js[z].LastWordId(), origTops[z], SMInfo.Query, Weighter.GetFactors(z)));
                    Qw.PushVector(SMInfo.Query.PosSqueezer->Squeeze(origTops[z]), &LQw.back());
                }
            }
            Weighter.Reset();
            Weighter.SetSingleFragmentMode(false);
            SingleFrags = Qw.ToSortedVector(topsCount > SMInfo.Query.PosSqueezer->GetThreshold() / 2 ? MTFM_EFFECTIVE_LEN | MTFM_ADAPTIVE_SHRINK : MTFM_ADAPTIVE_SHRINK);
            if (topNmulti) {
                //Note: SingleFragsFor4Frags aren't dumped
                for (size_t i = 0; i < SingleFrags.size(); ++i) {
                    topNmulti->Push(TSnip(TSingleSnip(SingleFrags[i]->WordRange, SMInfo), SingleFrags[i]->Weight, SingleFrags[i]->Factors));
                }
            }
        }

        TGetTwoPlusSnip::TGetTwoPlusSnip(const TConfig& cfg, TMxNetWeighter& w, TUniSpanIter &spanSet, const TSentsMatchInfo &sMInfo,
            ECandidateSource source, ISnippetCandidateDebugHandler *callback, bool dontGrow, bool needCheckTitle, float repeatedTitlePessimizeFactor)
          : Impl(new TImpl(cfg, w, spanSet, sMInfo, source, callback, dontGrow, needCheckTitle, repeatedTitlePessimizeFactor))
        {
        }

        TGetTwoPlusSnip::~TGetTwoPlusSnip()
        {
        }

        TSnip TGetTwoPlusSnip::GetSnip(float maxSize, float maxPartSize, size_t fragCnt,
            const TVector<TDynBitMap> &snipBitSet) {
            return Impl->GetSnip(maxSize, maxPartSize, fragCnt, snipBitSet);
        }
    }

}
