#include "defs.h"
#include "diversity.h"
#include "doc_hit_aggregator.h"
#include "hits_based_tr.h"
#include "link.h"
#include "rank.h"
#include "utility.h"

#include <kernel/mango/common/quotes.h>

#include <util/stream/mem.h>
#include <util/datetime/base.h>

namespace NMango
{
    inline float CalcSpamReabilitator(float q90) // ?!!
    {
        return (1.3f - 1.3f * q90 / (0.5f + q90));
    }

    template<TFactorGroups GRP>
    void CalcRemainderQuoteFactors(NMango::TAccessor<GRP> factors)
    {
        float cntLinks(factors[LINKS_COUNT]);
        float cntSpam(factors[SPAM_COUNT]);
        float cntAuthors(factors[AUTHOR_COUNT]);
        float quoteAuthoritySum = factors[QUOTE_AUTHORITY_SUM];
        float authorAuthoritySum = factors[AUTHOR_AUTHORITY_SUM];

        factors[SPAM_PROBABILITY]         = (cntLinks + cntSpam > ZERO_EPS) ? cntSpam / (cntLinks + cntSpam) : 0.0f;
        factors[AVERAGE_QUOTE_AUTHORITY]  = quoteAuthoritySum / (cntLinks + 1e-6f);
        factors[AVERAGE_AUTHOR_AUTHORITY] = authorAuthoritySum / (cntAuthors + 1e-6f);
        factors[UNIQUE_AUTHORS_RATIO] = cntAuthors / (cntLinks + 1e-6f);
        factors[UNIQUE_AUTHORS_RATIO_BY_AUTHORITY] = authorAuthoritySum / (quoteAuthoritySum + 1e-6f);

        float q90 = factors[AUTHOR_AUTHORITY_Q90];
        factors[SPAM_PROBABILITY] = Min(1.0f, factors[SPAM_PROBABILITY] * CalcSpamReabilitator(q90));
    }

    template<class TAccessor>
    void NormRelevance(TAccessor& acc, float value)
    {
        acc[COS_TR] *= value;
        acc[MANGO_TR] *= value;
        acc[PHRASE_TR] *= value;
        acc[TITLE_TR] *= value;
    }

    // Relevance factors calcers

    struct TAuthDecayTraits
    {
        inline static bool HasDecay() { return true; }
        inline static float GetAuthority(const TLinkInfo& data) { return data.AuthorAuthority; }
    };


    template<typename TTraits, typename TAggregatedFactor>
    class TRelevanceCalcer
    {
        TAggregatedFactor IdfCosRel, MangoRel, PhraseRel, TitleRel;
        TVector<TAggregatedFactor*> Aggrs;
        const TDecayCalculator* Decayer;
        float ConstQueryTimeBasedDecay;
        bool WasValue;
    public:
        int Count;

        TRelevanceCalcer()
            : Decayer(nullptr)
            , ConstQueryTimeBasedDecay(1.0)
            , WasValue(false)
            , Count(0)
        {
            Aggrs.push_back(&IdfCosRel);
            Aggrs.push_back(&MangoRel);
            Aggrs.push_back(&PhraseRel);
            Aggrs.push_back(&TitleRel);
        }

        void OnResourceStart(float constQueryTimeBasedDecay)
        {
            for (size_t i = 0; i < Aggrs.size(); ++i)
                Aggrs[i]->Reset();
            Count = 0;

            ConstQueryTimeBasedDecay = constQueryTimeBasedDecay;
        }

        void OnAuthorStart()
        {
            for (size_t i = 0; i < Aggrs.size(); ++i)
                Aggrs[i]->OnAuthorStart();
            WasValue = false;
        }

        void OnQuote(const TRawTrFeatures& tr, const TLinkInfo& data)
        {
            if (!tr.IsQuorum)
                return;

            float weight = TTraits::GetAuthority(data);
            if (weight <= 0.0f)
                return;

            WasValue = true;

            if (TTraits::HasDecay())
                weight *= Decayer->CalcDecay(data.PubTime);

            weight *= ConstQueryTimeBasedDecay;

            if (tr.IsQuorum) {
                // факторы текстовой релевантности
                IdfCosRel.OnNewValue(tr.IdfCosRel * weight);
                MangoRel.OnNewValue(tr.MangoLR * weight);
                PhraseRel.OnNewValue(tr.IsPerfectMatch ? weight : 0.0);
                TitleRel.OnNewValue(tr.MangoTitleLR * weight);
            }
        }

        void OnAuthorFinish()
        {
            for (size_t i = 0; i < Aggrs.size(); ++i)
                Aggrs[i]->OnAuthorFinish();
            if (WasValue) {
                ++Count;
            }
        }

        template<TFactorGroups Group>
        void OnResourceFinish(TAccessor<Group>& factors)
        {
            factors[COS_TR] = IdfCosRel.GetResult();
            factors[MANGO_TR] = MangoRel.GetResult();
            factors[PHRASE_TR] = PhraseRel.GetResult();
            factors[TITLE_TR] = TitleRel.GetResult();
        }

        void SetDecay(const TDecayCalculator& decayer)
        {
            Decayer = &decayer;
        }
    };



    // Window factors calcers

    class TWindowCalcerBase
    {
    protected:
        TVector<TAuthorAggregatedFactor*> Aggrs;

        TAuthorAggregatedFactor RelAuthorAuthoritySum, DecayedRelAuthorAuthoritySum;
#ifndef ENABLE_MANGO_1_0_OPTIMISATION
        TRelevanceCalcer<TAuthDecayTraits, TTop5AggregatedFactor> AuthDecayRelCalcer;
#endif
        TRelevanceCalcer<TAuthDecayTraits, TAuthorAggregatedFactor> AuthDecayRelCalcerSum;

        size_t CntRelLinks, CntRelForcedLinks, CntRelMegaForcedLinks, CntRelSpam, CntRelTitleLinks;
        float RelQuoteAuthoritySum, DecayedRelQuoteAuthoritySum, RelMaxAuthority;
        bool IsRelAuthor;
        size_t CntRelAuthors;

        TDecayCalculator Decayer;
        ui64 TimeFrom, TimeTo;
        ui64 TimeFromSoft;

        int CntRelUniqueLinksPerAuthor;
        TMergeableDiversityProcessor<3,16> RelDiversityProcessor, TempDivProcessor;
        bool Fast, OnlineSearch;
        bool SocialVerticalWindowMode;

        TFMSummator<ui32, 20, 16> RelAuthorQuantiles;

    public:
        TWindowCalcerBase()
            : Fast(false)
            , OnlineSearch(false)
            , SocialVerticalWindowMode(false)
        {
            Aggrs.push_back(&RelAuthorAuthoritySum);
            Aggrs.push_back(&DecayedRelAuthorAuthoritySum);
        }

        virtual ~TWindowCalcerBase()
        {
        }

        void SetDecay(time_t requestTime, time_t decayPeriod, float fading)
        {
            Decayer = TDecayCalculator(requestTime - decayPeriod, requestTime, fading);
            TimeFrom = static_cast<ui64>(requestTime - decayPeriod);
            TimeFromSoft = static_cast<ui64>(requestTime - decayPeriod * 5);
            TimeTo = static_cast<ui64>(requestTime);

#ifndef ENABLE_MANGO_1_0_OPTIMISATION
            AuthDecayRelCalcer.SetDecay(Decayer);
#endif
            AuthDecayRelCalcerSum.SetDecay(Decayer);
        }

        void SetMode(bool fast, bool onlineSearch, bool socialVerticalWindowMode)
        {
            Fast = fast;
            OnlineSearch = onlineSearch;
            SocialVerticalWindowMode = socialVerticalWindowMode;
        }

        void OnResourceStart(float constQueryTimeBasedDecay)
        {
#ifndef ENABLE_MANGO_1_0_OPTIMISATION
            AuthDecayRelCalcer.OnResourceStart(constQueryTimeBasedDecay);
#endif
            AuthDecayRelCalcerSum.OnResourceStart(constQueryTimeBasedDecay);

            for (size_t i = 0; i < Aggrs.size(); ++i)
                Aggrs[i]->Reset();

            RelDiversityProcessor.Reset();

            CntRelLinks = CntRelForcedLinks = CntRelMegaForcedLinks = CntRelSpam = CntRelAuthors = CntRelTitleLinks = 0;
            RelQuoteAuthoritySum = 0.f;
            DecayedRelQuoteAuthoritySum = 0.f;
            RelMaxAuthority = 0.0f;

            if (!Fast) {
                // возможно, замена конструирования объекта его копированием повысит скорость работы
                RelAuthorQuantiles = TFMSummator<ui32, 20, 16>(1e1, 1e-2);
            }
        }

        void OnAuthorStart()
        {
#ifndef ENABLE_MANGO_1_0_OPTIMISATION
            AuthDecayRelCalcer.OnAuthorStart();
#endif
            AuthDecayRelCalcerSum.OnAuthorStart();

            for (size_t i = 0; i < Aggrs.size(); ++i)
                Aggrs[i]->OnAuthorStart();

            IsRelAuthor = false;
            CntRelUniqueLinksPerAuthor = 0;
        }

        void OnAuthorFinish()
        {
#ifndef ENABLE_MANGO_1_0_OPTIMISATION
            AuthDecayRelCalcer.OnAuthorFinish();
#endif
            AuthDecayRelCalcerSum.OnAuthorFinish();

            for (size_t i = 0; i < Aggrs.size(); ++i)
                Aggrs[i]->OnAuthorFinish();

            if (IsRelAuthor) {
                ++CntRelAuthors;
            }
        }

        bool OnLink(const TRawTrFeatures& tr, const TLinkInfo& link)
        {
            if (link.IsBiased) {
                return false;
            }

            if (TimeTo < link.PubTime)
                return false;

            int count = static_cast<int>(link.Count) + static_cast<int>(link.RepostCount);

            if (link.IsSpammer) {
                if (TimeFrom <= link.PubTime)
                    CntRelSpam += count;
                return false;
            }

            // не прошли кворум или не попали в окно
            if (!tr.IsQuorum || link.PubTime < TimeFromSoft)
                return false;

#ifndef ENABLE_MANGO_1_0_OPTIMISATION
            AuthDecayRelCalcer.OnQuote(tr, link);
#endif
            AuthDecayRelCalcerSum.OnQuote(tr, link);

            if (link.PubTime < TimeFrom)
                return false;

            // если репост - то не надо обрабатывать в тематических факторах и факторах разнообразия
            if (IsRepost(static_cast<TQuoteExtractionType>(link.ExtractionType)))
                return true;

            RelMaxAuthority = Max(RelMaxAuthority, link.AuthorAuthority);
            CntRelLinks += count;
            if (tr.IsForcedQuorum) {
                CntRelForcedLinks += count;
            }
            if (tr.IsMegaForcedQuorum) {
                CntRelMegaForcedLinks += count;
            }
            if (tr.IsTitleQuorum) {
                CntRelTitleLinks += count;
            }

            float decay = Decayer.CalcDecay(link.PubTime);
            RelQuoteAuthoritySum += link.AuthorAuthority * count;
            DecayedRelQuoteAuthoritySum += link.AuthorAuthority * decay * count;
            RelAuthorAuthoritySum.OnNewValue(link.AuthorAuthority);
            DecayedRelAuthorAuthoritySum.OnNewValue(link.AuthorAuthority * decay);

            IsRelAuthor = true;

            ++CntRelUniqueLinksPerAuthor;
            if (!Fast) {
                if (CntRelUniqueLinksPerAuthor <= 3) {
                    TMemoryInput inp(link.DiversitySerialize.Data, sizeof(link.DiversitySerialize.Data));
                    TempDivProcessor.Load(&inp);
                    RelDiversityProcessor.Merge(TempDivProcessor);
                }

                char authorHash[15];
                sprintf(authorHash, "%d", link.AuthorHash);
                RelAuthorQuantiles.Add(authorHash, link.AuthorAuthority);
            }

            return true;
        }
        virtual void OnResourceFinish(float *factors) = 0;
    };

    template<TWindow Wnd>
    class TWindowCalcer : public TWindowCalcerBase
    {
        #define GrpQuoteRel     static_cast<TFactorGroups>(TWindowTraits<Wnd>::QuoteRel)
        #define GrpRelevanceAt  static_cast<TFactorGroups>(TWindowTraits<Wnd>::RelAt)
        #define GrpRelevanceSum static_cast<TFactorGroups>(TWindowTraits<Wnd>::RelSum)
        #define GrpRelevanceAvg static_cast<TFactorGroups>(TWindowTraits<Wnd>::RelAvg)
        #define GrpOther        static_cast<TFactorGroups>(TWindowTraits<Wnd>::Other)
        #define GrpFinal        static_cast<TFactorGroups>(TWindowTraits<Wnd>::Final)
        #define GrpFinalFast    static_cast<TFactorGroups>(TWindowTraits<Wnd>::FinalFast)

        template<TFactorGroups GrpFinalLine>
        void CalcFinalFactors(float *factors, bool fast)
        {
            NMango::TAccessor<GrpFinalLine> finalDynamicFactors(factors);
            NMango::TAccessor<GrpQuoteRel>  quoteRelFactors(factors);

            // составные части конечной формулы
            finalDynamicFactors[SIMPLE_PESSIMISATION]          = MangoPessimisation<GrpQuoteRel, GrpOther>(factors, fast);
            finalDynamicFactors[SIMPLE_RELEVANCE_AT]           = MangoSimpleRelevance<GrpRelevanceAt, COS_TR>(factors, fast);
            finalDynamicFactors[SIMPLE_RELEVANCE_SUM]          = MangoSimpleRelevance<GrpRelevanceSum, COS_TR>(factors, fast);
            finalDynamicFactors[SIMPLE_RELEVANCE_AVG]          = MangoSimpleRelevance<GrpRelevanceAvg, COS_TR>(factors, fast);
            finalDynamicFactors[SIMPLE_RELEVANCE_MNG_AT]       = MangoSimpleRelevance<GrpRelevanceAt, MANGO_TR>(factors, fast);
            finalDynamicFactors[SIMPLE_RELEVANCE_MNG_SUM]      = MangoSimpleRelevance<GrpRelevanceSum, MANGO_TR>(factors, fast);
            finalDynamicFactors[SIMPLE_RELEVANCE_MNG_AVG]      = MangoSimpleRelevance<GrpRelevanceAvg, MANGO_TR>(factors, fast);
            finalDynamicFactors[SIMPLE_INTERESTINGNESS]        = MangoSimpleInterestingness<GrpQuoteRel, GrpOther>(factors, fast);
            finalDynamicFactors[SIMPLE_TRASHLESS]              = MangoSimpleTrashless<GrpQuoteRel>(factors, fast);
            finalDynamicFactors[SIMPLE_STATIC_INTERESTINGNESS] = MangoSimpleInterestingnessStatic(factors, fast);
            finalDynamicFactors[SIMPLE_STATIC_TRASHLESS]       = MangoSimpleTrashlessStatic(factors, fast);
            finalDynamicFactors[SIMPLE_HOST_INTERESTINGNESS]   = MangoSimpleInterestingnessHost(factors, fast);
            finalDynamicFactors[SIMPLE_HOST_TRASHLESS]         = MangoSimpleTrashlessHost(factors, fast);

            float res = finalDynamicFactors[SIMPLE_RELEVANCE_MNG_SUM] / sqrt(quoteRelFactors[LINKS_COUNT])
                      * finalDynamicFactors[SIMPLE_INTERESTINGNESS]
                      * (1.0f + log(1.0f + finalDynamicFactors[SIMPLE_STATIC_INTERESTINGNESS]))
                      * finalDynamicFactors[SIMPLE_TRASHLESS]
                      * finalDynamicFactors[SIMPLE_STATIC_TRASHLESS]
                      * finalDynamicFactors[SIMPLE_PESSIMISATION];
            finalDynamicFactors[SIMPLE_FORMULA_1] = IsNan(res) ? 0 : res;
        }

    public:

        void OnResourceFinish(float *factors) override
        {
            // heuristic that is actual for fastrank: if there are no relevant quotes, document is bad
            if (CntRelLinks == 0 ||
                    (SocialVerticalWindowMode && NMango::TAccessor<GROUP_OTHER_STATIC>(factors)[CREATION_TIME] < TimeFrom))
            {
                NMango::TAccessor<GrpFinal> finalDynamicFactors(factors);
                finalDynamicFactors[SIMPLE_FORMULA_1] = 0;
                return;
            }
            // релевантность
            NMango::TAccessor<GrpRelevanceAt> relevanceFactorsAt(factors);
            NMango::TAccessor<GrpRelevanceSum> relevanceFactorsSum(factors);
#ifndef ENABLE_MANGO_1_0_OPTIMISATION
            AuthDecayRelCalcer.OnResourceFinish(relevanceFactorsAt);
#endif
            AuthDecayRelCalcerSum.OnResourceFinish(relevanceFactorsSum);

            NMango::TAccessor<GrpRelevanceAvg> relevanceFactorsAvg(factors);
            relevanceFactorsAvg.CopyFrom(relevanceFactorsSum);
            NormRelevance(relevanceFactorsAvg, 1.0f / static_cast<float>(AuthDecayRelCalcerSum.Count));

            NMango::TAccessor<GROUP_QUOTE_HOST> hostFactors(factors);
            NMango::TAccessor<GROUP_QUOTE_STATIC> staticFactors(factors);
            NMango::TAccessor<GrpQuoteRel> quoteRelFactors(factors);
            NMango::TAccessor<GrpOther> otherDynamicFactors(factors);

            float cntStaticLinks(staticFactors[LINKS_COUNT]);
            float cntStaticAuthors(staticFactors[AUTHOR_COUNT]);
            float authorAuthoritySum(staticFactors[AUTHOR_AUTHORITY_SUM]);
            float quoteAuthoritySum(staticFactors[QUOTE_AUTHORITY_SUM]);
            float relAuthorAuthoritySum = RelAuthorAuthoritySum.GetResult();

            // прочие факторы
            quoteRelFactors[LINKS_COUNT]              = CntRelLinks;
            quoteRelFactors[AUTHOR_COUNT]             = CntRelAuthors;
            quoteRelFactors[QUOTE_AUTHORITY_SUM]      = RelQuoteAuthoritySum;
            quoteRelFactors[AUTHOR_AUTHORITY_SUM]     = relAuthorAuthoritySum;
            quoteRelFactors[SPAM_COUNT]               = CntRelSpam;
            quoteRelFactors[MAX_AUTHORITY]            = RelMaxAuthority;
            quoteRelFactors[DIVERSITY]                = RelDiversityProcessor.CalcDiversity();
            quoteRelFactors[AVERAGE_QUOTE_AUTHORITY]  = RelQuoteAuthoritySum / CntRelLinks;
            quoteRelFactors[AVERAGE_AUTHOR_AUTHORITY] = relAuthorAuthoritySum / CntRelAuthors;
            quoteRelFactors[SPAM_PROBABILITY]         = (CntRelLinks + CntRelSpam > 0) ? CntRelSpam / static_cast<float>(CntRelLinks + CntRelSpam) : 0.0f;
            quoteRelFactors[UNIQUE_AUTHORS_RATIO]     = CntRelAuthors / (CntRelLinks + 1e-6f);
            quoteRelFactors[UNIQUE_AUTHORS_RATIO_BY_AUTHORITY] = relAuthorAuthoritySum / (RelQuoteAuthoritySum + 1e-6f);

            if (Fast) {
                quoteRelFactors[AUTHOR_AUTHORITY_Q50] =  1;
                quoteRelFactors[AUTHOR_AUTHORITY_Q75] =  1;
                quoteRelFactors[AUTHOR_AUTHORITY_Q90] =  1;
                quoteRelFactors[AUTHOR_AUTHORITY_AVG75] =1;
            } else {
                float q75, q90;
                quoteRelFactors[AUTHOR_AUTHORITY_Q50] = RelAuthorQuantiles.CalcQuantile(0.50f);
                quoteRelFactors[AUTHOR_AUTHORITY_Q75] = q75=RelAuthorQuantiles.CalcQuantile(0.75f);
                quoteRelFactors[AUTHOR_AUTHORITY_Q90] = q90=RelAuthorQuantiles.CalcQuantile(0.90f);
                quoteRelFactors[AUTHOR_AUTHORITY_AVG75] = RelAuthorQuantiles.CalcAvgValueInInterval(q75, 1e6);

                quoteRelFactors[SPAM_PROBABILITY] = Min(1.0f, quoteRelFactors[SPAM_PROBABILITY] * CalcSpamReabilitator(q90));
            }

            otherDynamicFactors[TOPIC_SHARE]                   = RelQuoteAuthoritySum / (quoteAuthoritySum + THEMATIC_DELTA);
            otherDynamicFactors[AUTHOR_TOPIC_SHARE]            = relAuthorAuthoritySum / (authorAuthoritySum + THEMATIC_DELTA);
            otherDynamicFactors[UNWEIGHTED_TOPIC_SHARE]        = CntRelLinks / (cntStaticLinks + THEMATIC_DELTA);
            otherDynamicFactors[UNWEIGHTED_AUTHOR_TOPIC_SHARE] = CntRelAuthors / (cntStaticAuthors + THEMATIC_DELTA);
            otherDynamicFactors[DECAYED_QUOTE_AUTHORITY_SUM]   = DecayedRelQuoteAuthoritySum;
            otherDynamicFactors[DECAYED_AUTHOR_AUTHORITY_SUM]  = DecayedRelAuthorAuthoritySum.GetResult();
            otherDynamicFactors[FORCED_LINKS_COUNT]            = CntRelForcedLinks;
            otherDynamicFactors[MEGA_FORCED_LINKS_COUNT]       = CntRelMegaForcedLinks;
            otherDynamicFactors[TITLE_LINKS_COUNT]             = CntRelTitleLinks;
            otherDynamicFactors[NORMALIZED_LINKS_COUNT]        = (cntStaticLinks > 0) ? (CntRelLinks / sqrt(cntStaticLinks)) : 0;

            // ну и итоговые факторы
            CalcFinalFactors<GrpFinal>(factors, Fast);
        }

        #undef GrpQuoteRel
        #undef GrpRelevanceAt
        #undef GrpRelevanceSum
        #undef GrpRelevanceAvg
        #undef GrpOther
        #undef GrpFinal
        #undef GrpFinalFast
    };

    // TLinkFactorsAggregator implementation

    class TLinkFactorsAggregator::TImpl
    {
        TVector<TWindowCalcerBase*> WindowCalcers;
        TWindowCalcer<WINDOW_0> WindowCalcer0;
        TWindowCalcer<WINDOW_1> WindowCalcer1;
        TWindowCalcer<WINDOW_2> WindowCalcer2;
        TWindowCalcer<WINDOW_3> WindowCalcer3;

        TDecayCalculator Decayer;
        time_t RequestTime;

        size_t ActualWindowsCount;
    public:
        TImpl();
        void SetRequestTime(time_t requestTime);
        void OnResourceStart(float authKoef);
        void OnAuthorStart();
        bool OnLink(const TRawTrFeatures& rawFeatures, const TLinkInfo& link);
        void OnAuthorFinish();
        void OnResourceFinish(const TDocHitAggregator* docHitAggregator, float *factors);
        void SetMode(bool fast, bool onlineSearch, bool socialVerticalWindowMode);
    };


    // ----- TLinkFactorsAggregator::TImpl -----

    TLinkFactorsAggregator::TImpl::TImpl()
    {
        WindowCalcers = {
            &WindowCalcer0,
            &WindowCalcer1,
            &WindowCalcer2,
            &WindowCalcer3
        };

        ActualWindowsCount = WindowCalcers.size();
    }


    void TLinkFactorsAggregator::TImpl::SetRequestTime(time_t requestTime)
    {
        Decayer = TDecayCalculator(requestTime - DEFAULT_DECAY_PERIOD, requestTime, DEFAULT_FADING);

        for (size_t i = 0; i < WindowCalcers.size(); ++i)
            WindowCalcers[i]->SetDecay(requestTime, WindowDescriptors[i].Period, WindowDescriptors[i].Fading);

        RequestTime = requestTime;
    }

    void TLinkFactorsAggregator::TImpl::OnResourceStart(float constQueryTimeBasedDecay)
    {
        for (size_t i = 0; i < ActualWindowsCount; ++i)
            WindowCalcers[i]->OnResourceStart(constQueryTimeBasedDecay);
    }

    void TLinkFactorsAggregator::TImpl::OnAuthorStart()
    {
        for (size_t i = 0; i < ActualWindowsCount; ++i)
            WindowCalcers[i]->OnAuthorStart();
    }

    void TLinkFactorsAggregator::TImpl::OnAuthorFinish()
    {
        for (size_t i = 0; i < ActualWindowsCount; ++i)
            WindowCalcers[i]->OnAuthorFinish();
    }

    bool TLinkFactorsAggregator::TImpl::OnLink(const TRawTrFeatures& tr, const TLinkInfo& link)
    {
        bool fOk = false;
        for (size_t i = 0; i < ActualWindowsCount; ++i) {
            if (WindowCalcers[i]->OnLink(tr, link)) {
                fOk = true;
            }
        }

        return fOk;
    }

    void TLinkFactorsAggregator::TImpl::OnResourceFinish(const TDocHitAggregator* docHitAggregator, float *factors)
    {
        NMango::TAccessor<GROUP_QUOTE_HOST> hostFactors(factors);
        NMango::TAccessor<GROUP_QUOTE_STATIC> staticFactors(factors);

        CalcRemainderQuoteFactors(staticFactors);
        CalcRemainderQuoteFactors(hostFactors);

        if (docHitAggregator) {
            NMango::TAccessor<GROUP_DOC_RELEVANCE> docRelevanceFactors(factors);
            NMango::TConstAccessor<GROUP_QUOTE_STATIC> constStaticQuoteFactors(factors);
            docHitAggregator->GetFactors(docRelevanceFactors, constStaticQuoteFactors);
        }

        for (size_t i = 0; i < ActualWindowsCount; ++i)
            WindowCalcers[i]->OnResourceFinish(factors);
    }

    void TLinkFactorsAggregator::TImpl::SetMode(bool fast, bool onlineSearch, bool socialVerticalWindowMode)
    {
        ActualWindowsCount = fast ? 1 : WindowCalcers.size();
        for (size_t i = 0; i < WindowCalcers.size(); ++i)
            WindowCalcers[i]->SetMode(fast, onlineSearch, socialVerticalWindowMode);
    }

    // ----- TLinkFactorsAggregator -----

    TLinkFactorsAggregator::TLinkFactorsAggregator()
        : Impl(new TImpl())
    {
        Impl->SetRequestTime(Now().TimeT());
    }

    TLinkFactorsAggregator::~TLinkFactorsAggregator()
    {}

    void TLinkFactorsAggregator::SetRequestTime(time_t requestTime)
    {
        Impl->SetRequestTime(requestTime);
    }

    void TLinkFactorsAggregator::OnResourceStart(float constQueryTimeBasedDecay)
    {
        Impl->OnResourceStart(constQueryTimeBasedDecay);
    }

    void TLinkFactorsAggregator::OnAuthorStart()
    {
        Impl->OnAuthorStart();
    }

    bool TLinkFactorsAggregator::OnLink(const TRawTrFeatures& rawFeatures, const TLinkInfo& link)
    {
        return Impl->OnLink(rawFeatures, link);
    }

    void TLinkFactorsAggregator::OnAuthorFinish()
    {
        Impl->OnAuthorFinish();
    }

    void TLinkFactorsAggregator::OnResourceFinish(const TDocHitAggregator* docHitAggregator, float *factors)
    {
        Impl->OnResourceFinish(docHitAggregator, factors);
    }

    void TLinkFactorsAggregator::SetMode(bool fast, bool onlineSearch, bool socialWindowMode)
    {
        Impl->SetMode(fast, onlineSearch, socialWindowMode);
    }
} // NMango

