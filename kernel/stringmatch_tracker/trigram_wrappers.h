#pragma once
#include "calcer_base.h"

#include <kernel/stringmatch_tracker/matchers/matcher.h>
#include <util/generic/ptr.h>

namespace NStringMatchTracker {

    class TQueryTrigramOverDocCalcer : public TCalcer {
    public:
        static const ECalcer CalcerType = ECalcer::QueryTrigramCalcer;

    private:
        THolder<::NSequences::TTrigramMatcher> Matcher;
        bool SplitByWords = true;

    public:
        TQueryTrigramOverDocCalcer(bool splitByWords = true) : SplitByWords(splitByWords) {
        }
        ECalcer GetCalcerType() const override;
        void NewQuery(const TString& query) override;
        void ProcessDoc(const char* docBeg, const char* docEnd) override;
        void Reset() override;
    };

    class TDocTrigramOverQueryCalcer : public TCalcer {
    public:
        static const ECalcer CalcerType = ECalcer::DocTrigramCalcer;

    private:
        TString Query;
        bool SplitByWords = true;

    public:
        TDocTrigramOverQueryCalcer(bool splitByWords = true) : SplitByWords(splitByWords) {
        }
        ECalcer GetCalcerType() const override;
        void NewQuery(const TString& query) override;
        void ProcessDoc(const char* docBeg, const char* docEnd) override;
        void Reset() override;
    };
}
