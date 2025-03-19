#include "video_span.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/snip_builder/snip_builder.h>
#include <kernel/snippets/uni_span_iter/uni_span_iter.h>
#include <kernel/snippets/video/video.h>
#include <kernel/snippets/weight/weighter.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {
    namespace NSnipVideoWordSpans {
        class TOneSnipBuilder {
        private:
            const TConfig& Cfg;
            TMxNetWeighter& Weighter;
            TUniSpanIter& SpanSet;
            const TSentsMatchInfo& SentsMatchInfo;
            const float MaxSize;
            ECandidateSource Source;
            ISnippetCandidateDebugHandler* Callback;
            const char* AlgoName;

        public:
            TOneSnipBuilder(const TConfig& cfg, TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb, const char* aname);

            struct IConsumer {
                virtual void operator()(double weight, const std::pair<int, int>& span, const TFactorStorage& factors) = 0;
                virtual ~IConsumer() {
                }
            };

            void PatchVideoBoost(int z, int w0, int w1);
            void IterateSnippets(IConsumer& consumer);

        };


        TOneSnipBuilder::TOneSnipBuilder(const TConfig& cfg, TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb, const char* aname)
         : Cfg(cfg)
         , Weighter(w)
         , SpanSet(sF)
         , SentsMatchInfo(smInfo)
         , MaxSize(maxSize)
         , Source(source)
         , Callback(cb)
         , AlgoName(aname)
        {
        }
        void TOneSnipBuilder::PatchVideoBoost(int z, int w0, int w1) {
            if (Cfg.VideoAttrWeight()) {
                int s0 = SentsMatchInfo.SentsInfo.WordId2SentId(w0);
                int s1 = SentsMatchInfo.SentsInfo.WordId2SentId(w1);
                double weight = -Max<double>();
                for (int i = s0; i <= s1; ++i) {
                    weight = Max(weight, TreatVideoPassageAttrWeight(SentsMatchInfo.SentsInfo.GetArchiveSent(i).Attr, Cfg));
                }
                Weighter.GetFactors(z)[NFactorSlices::EFactorSlice::SNIPPETS_MAIN][A2_MF_VIDEOATTR] = weight;
            }
        }
        void TOneSnipBuilder::IterateSnippets(IConsumer& consumer)
        {
            typedef TUniSpanIter::TWordpos TWordpos;

            const TSentsInfo& sentsInfo = SentsMatchInfo.SentsInfo;
            IAlgoTop* const cbtopn = Callback ? Callback->AddTop(AlgoName, Source) : nullptr;

            double weights[TFactorStorage128::MAX];
            TVector<TWordpos> is(TFactorStorage128::MAX, sentsInfo.Begin<TWordpos>());
            TVector<TWordpos> js(TFactorStorage128::MAX, sentsInfo.End<TWordpos>());

            SpanSet.Reset(MaxSize);
            Weighter.Reset();
            while (SpanSet.MoveNext()) {
                if (Weighter.IsFull()) {
                    for (size_t z = 0; z < TFactorStorage128::MAX; ++z) {
                        PatchVideoBoost(z, is[z].FirstWordId(), js[z].LastWordId());
                    }
                    Weighter.BatchCalc(weights);
                    for (size_t z = 0; z < TFactorStorage128::MAX; ++z) {
                        if (cbtopn) {
                            cbtopn->Push(TSnip(TSingleSnip(is[z], js[z], SentsMatchInfo), weights[z], Weighter.GetFactors(z)));
                        }
                        consumer(weights[z], std::make_pair(is[z].FirstWordId(), js[z].LastWordId()), Weighter.GetFactors(z));
                    }
                }
                const TWordpos i = SpanSet.GetFirst();
                const TWordpos j = SpanSet.GetLast();
                Weighter.SetSpan(i, j);
                is[Weighter.GetUsed() - 1] = i;
                js[Weighter.GetUsed() - 1] = j;
            }

            if (Weighter.GetUsed() > 0) {
                for (size_t z = 0; z < Weighter.GetUsed(); ++z) {
                    PatchVideoBoost(z, is[z].FirstWordId(), js[z].LastWordId());
                }
                Weighter.BatchCalc(weights);
                for (size_t z = 0; z < Weighter.GetUsed(); ++z) {
                    if (cbtopn) {
                        cbtopn->Push(TSnip(TSingleSnip(is[z], js[z], SentsMatchInfo), weights[z], Weighter.GetFactors(z)));
                    }
                    consumer(weights[z], std::make_pair(is[z].FirstWordId(), js[z].LastWordId()), Weighter.GetFactors(z));
                }
            }
        }

        struct TBestChooser : TOneSnipBuilder::IConsumer {
            const TSentsMatchInfo& SentsMatchInfo;
            const TWordSpanLen& WordSpanLen;
            const float MaxSize;
            bool Some;
            double BestWeight;
            std::pair<int, int> BestSpan;
            TFactorStorage BestFactors;

            TBestChooser(const TSentsMatchInfo& sentsMatchInfo, const TWordSpanLen& wordSpanLen, float maxSize)
              : SentsMatchInfo(sentsMatchInfo)
              , WordSpanLen(wordSpanLen)
              , MaxSize(maxSize)
              , Some(false)
              , BestWeight(0)
              , BestSpan()
            {
            }

            void operator()(double weight, const std::pair<int, int>& span, const TFactorStorage& factors) override {
                if (!Some || weight > BestWeight) {
                    BestWeight = weight;
                    BestSpan = span;
                    BestFactors = factors;
                    Some = true;
                }
            }

            TSnip GetResult() {
                if (!Some) {
                    return TSnip();
                }
                TSnipBuilder b(SentsMatchInfo, WordSpanLen, MaxSize, MaxSize);
                b.Add(TSentMultiword(SentsMatchInfo.SentsInfo.WordId2SentWord(BestSpan.first)), TSentMultiword(SentsMatchInfo.SentsInfo.WordId2SentWord(BestSpan.second)));
                TSnip res = b.Get(BestWeight, std::move(BestFactors));
                return res;
            }

        };

        TSnip GetVideoSnippet(const TConfig& cfg, TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb, const char* aname) {
            TOneSnipBuilder osb(cfg, w, sF, smInfo, maxSize, source, cb, aname);
            TBestChooser bst(smInfo, sF.GetWordSpanLen(), maxSize);
            osb.IterateSnippets(bst);
            return bst.GetResult();
        }
    }
}
