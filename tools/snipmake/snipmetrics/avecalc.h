#pragma once
#include "snipmetrics.h"

namespace NSnippets {

    class TMetricsAveCalc : public  TSnipMetricsCalculator {
    private:
        TMetricArray Stat;
    protected:
        void ProcessMetricValue(EMetricName metricName, double value) override;
    public:
        TMetricsAveCalc(const TString& desc, const TString& stopwordsFile, const TString& pornoWordsConfig, IOutputStream* out);
        ~TMetricsAveCalc() override;
        void Report() override;
    };

}
