#pragma once

#include "factors.h"
#include <kernel/mango/common/types.h>
#include <util/generic/ptr.h>

namespace NMango
{

    struct TRawTrFeatures;
    class TDocHitAggregator;

    class TLinkFactorsAggregator
    {
    public:
        TLinkFactorsAggregator();
        ~TLinkFactorsAggregator();
        void SetRequestTime(time_t requestTime);
        void OnResourceStart(float constQueryTimeBasedDecay);
        void OnAuthorStart();
        bool OnLink(const TRawTrFeatures &rawFeatures, const TLinkInfo& link);
        void OnAuthorFinish();
        void OnResourceFinish(const TDocHitAggregator* docHitAggregator, float *factors);
        // in fast mode we don't calc windowed factors (except for W0)
        void SetMode(bool fast, bool onlineSearch, bool socialVerticalMode);
    private:
        class TImpl;
        THolder<TImpl> Impl;
    }; // TLinkFactorsAggregator

    template<TFactorGroups GrpQuoteRel>
    bool CheckForSpamOnSearch(const TFactors& factors)
    {
        return factors.Get<GrpQuoteRel>(SPAM_PROBABILITY) > 0.5f ||
               factors.Get<GROUP_QUOTE_STATIC>(SPAM_PROBABILITY) > 0.6f ||
               factors.Get<GROUP_QUOTE_HOST>(SPAM_PROBABILITY) > 0.7f;
    }

    template<TFactorGroups GrpQuoteRel>
    bool CheckForSpamOnSearch(const TErfInfo& /*erf*/, const TFactors& factors)
    {
        return CheckForSpamOnSearch<GrpQuoteRel>(factors);
    }

    void CalcRemainderStaticFactors(NMango::TAccessor<GROUP_QUOTE_STATIC> staticFactors);
    void CalcRemainderHostFactors(NMango::TAccessor<GROUP_QUOTE_HOST> hostFactors);

} // NMango

