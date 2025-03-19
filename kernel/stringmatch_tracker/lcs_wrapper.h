#pragma once
#include "calcer_base.h"
#include <kernel/stringmatch_tracker/matchers/matcher.h>

namespace NStringMatchTracker {

    class TLCSCalcer : public TCalcer {
    public:
        static const ECalcer CalcerType = ECalcer::LcsCalcer;

    private:
        static const size_t Threshold = 3;
        THolder<::NSequences::TLCSMatcher> Matcher;

    public:
        ECalcer GetCalcerType() const override;
        void NewQuery(const TString& query) override;
        void ProcessDoc(const char* docBeg, const char* docEnd) override;
        void Reset() override;
    };

}
