#include "one_span.h"

#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/snip_builder/snip_builder.h>
#include <kernel/snippets/uni_span_iter/uni_span_iter.h>
#include <kernel/snippets/weight/weighter.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {
    namespace NSnipWordSpans {
        class TOneSnipBuilder {
        private:
            TMxNetWeighter& Weighter;
            TUniSpanIter& SpanSet;
            const TSentsMatchInfo& SentsMatchInfo;
            const float MaxSize;
            ECandidateSource Source;
            ISnippetCandidateDebugHandler* Callback;
            ISnippetCandidateDebugHandler* FactSnipCandidatesCallback;

            const char* AlgoName;

        public:
            TOneSnipBuilder(TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb,
                            ISnippetCandidateDebugHandler* fsccb, const char* aname);

            struct IConsumer {
                virtual void operator()(double weight, const std::pair<int, int>& span, const TFactorStorage& factors) = 0;
                virtual ~IConsumer() {
                }
            };

            void IterateSnippets(IConsumer& consumer);

        };


        TOneSnipBuilder::TOneSnipBuilder(TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb,
                                         ISnippetCandidateDebugHandler* fsccb, const char* aname)
         : Weighter(w)
         , SpanSet(sF)
         , SentsMatchInfo(smInfo)
         , MaxSize(maxSize)
         , Source(source)
         , Callback(cb)
         , FactSnipCandidatesCallback(fsccb)
         , AlgoName(aname)
        {
        }
        void TOneSnipBuilder::IterateSnippets(IConsumer& consumer)
        {
            typedef TUniSpanIter::TWordpos TWordpos;

            const TSentsInfo& sentsInfo = SentsMatchInfo.SentsInfo;
            IAlgoTop* const cbtopn = Callback ? Callback->AddTop(AlgoName, Source) : nullptr;
            IAlgoTop* const fscbtopn = FactSnipCandidatesCallback ? FactSnipCandidatesCallback->AddTop(AlgoName, Source) : nullptr;


            double weights[TFactorStorage128::MAX];
            TVector<TWordpos> is(TFactorStorage128::MAX, sentsInfo.Begin<TWordpos>());
            TVector<TWordpos> js(TFactorStorage128::MAX, sentsInfo.End<TWordpos>());

            SpanSet.Reset(MaxSize);
            Weighter.Reset();
            while (SpanSet.MoveNext()) {
                if (Weighter.IsFull()) {
                    Weighter.BatchCalc(weights);
                    for (size_t z = 0; z < TFactorStorage128::MAX; ++z) {
                        if (cbtopn) {
                            cbtopn->Push(TSnip(TSingleSnip(is[z], js[z], SentsMatchInfo), weights[z], Weighter.GetFactors(z)));
                        }
                        if (fscbtopn) {

                            fscbtopn->Push(TSnip(TSingleSnip(is[z], js[z], SentsMatchInfo), weights[z], Weighter.GetFactors(z)));
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
                Weighter.BatchCalc(weights);
                for (size_t z = 0; z < Weighter.GetUsed(); ++z) {
                    if (cbtopn) {

                        cbtopn->Push(TSnip(TSingleSnip(is[z], js[z], SentsMatchInfo), weights[z], Weighter.GetFactors(z)));
                    }
                    if (fscbtopn) {
                        fscbtopn->Push(TSnip(TSingleSnip(is[z], js[z], SentsMatchInfo), weights[z], Weighter.GetFactors(z)));
                    }
                    consumer(weights[z], std::make_pair(is[z].FirstWordId(), js[z].LastWordId()), Weighter.GetFactors(z));
                }
            }
        }

        struct TBestChooser : TOneSnipBuilder::IConsumer {
            const TSentsMatchInfo& SentsMatchInfo;
            const TWordSpanLen& WordSpanLen;
            const float MaxSize;
            const bool NeedCheckTitle;
            const float RepeatedTitlePessimizeFactor;
            bool Some;
            double BestWeight;
            std::pair<int, int> BestSpan;
            TFactorStorage BestFactors;

            TBestChooser(const TSentsMatchInfo& sentsMatchInfo, const TWordSpanLen& wordSpanLen, float maxSize, bool needCheckTitle, float repeatedTitlePessimizeFactor)
              : SentsMatchInfo(sentsMatchInfo)
              , WordSpanLen(wordSpanLen)
              , MaxSize(maxSize)
              , NeedCheckTitle(needCheckTitle)
              , RepeatedTitlePessimizeFactor(repeatedTitlePessimizeFactor)
              , Some(false)
              , BestWeight(0)
              , BestSpan(0, 0)
            {
            }

            void operator()(double weight, const std::pair<int, int>& span, const TFactorStorage& factors) override {
                if (NeedCheckTitle) {
                    // Check first sentence
                    if (SentsMatchInfo.SentRepeatsTitle(SentsMatchInfo.SentsInfo.WordId2SentId(span.first))) {
                        weight -= RepeatedTitlePessimizeFactor;
                    }
                }
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

        TSnip GetTSnippet(TMxNetWeighter& w, TUniSpanIter& sF, const TSentsMatchInfo& smInfo, float maxSize, ECandidateSource source, ISnippetCandidateDebugHandler* cb,
                          ISnippetCandidateDebugHandler* fsccb, const char* aname, bool needCheckTitle, float repeatedTitlePessimizeFactor) {
            TOneSnipBuilder osb(w, sF, smInfo, maxSize, source, cb, fsccb, aname);
            TBestChooser bst(smInfo, sF.GetWordSpanLen(), maxSize, needCheckTitle, repeatedTitlePessimizeFactor);
            osb.IterateSnippets(bst);
            return bst.GetResult();
        }
    }
}
