#include "avecalc.h"
#include <dict/dictutil/xml_writer.h>

namespace NSnippets {

    TMetricsAveCalc::TMetricsAveCalc(const TString& desc, const TString& stopwordsFile, const TString& pornoWordsConfig, IOutputStream* out)
        : TSnipMetricsCalculator(desc, stopwordsFile, pornoWordsConfig, out)
    {}

    TMetricsAveCalc::~TMetricsAveCalc() {}

    void TMetricsAveCalc::ProcessMetricValue(EMetricName metricName, double value) {
        Stat.SumValue(metricName, value);
    }
    void TMetricsAveCalc::Report() {
        Stat.WriteAsXml(Description, Out);
    }
}



