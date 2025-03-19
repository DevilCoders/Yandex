#include <kernel/cluster/lib/cluster_metrics/base.h>

#include <library/cpp/getopt/last_getopt.h>

int main(int argc, const char **argv) {
    TString markupFileName, clusterFileName, titlesFileName, importantUrlsFileName, baselineClusterFileName;
    bool printAlexRecallMetrics = false;
    bool printBCubedMetrics = false;
    bool printDetailedInfo = false;
    bool doRegularization = true;
    double factor = 0;
    size_t totalDocumentsCount = 1;

    NLastGetopt::TOpts opts(NLastGetopt::TOpts::Default());

    opts.AddCharOption('a', "export AlexRecall metrics").Optional()
        .StoreValue(&printAlexRecallMetrics, true).NoArgument();
    opts.AddCharOption('b', "export BCubed metrics").Optional()
        .StoreValue(&printBCubedMetrics, true).NoArgument();
    opts.AddCharOption('v', "detailed export mode").Optional()
        .StoreValue(&printDetailedInfo, true).NoArgument();
    opts.AddCharOption('c', "sample clusterization file name").Required()
        .StoreResult(&clusterFileName).RequiredArgument("FILENAME");
    opts.AddCharOption('m', "markup clusterization file name").Required()
        .StoreResult(&markupFileName).RequiredArgument("FILENAME");
    opts.AddCharOption('i', "important urls file name").Optional()
        .StoreResult(&importantUrlsFileName).RequiredArgument("FILENAME");
    opts.AddCharOption('p', "probability of unmarked document to be relevant").Optional()
        .StoreResult(&factor).RequiredArgument("PROBABILITY").DefaultValue("0");
    opts.AddCharOption('r', "do regularization (1) or not (0)").Optional()
        .StoreResult(&doRegularization).DefaultValue("1");
    opts.AddLongOption("total", "totalDocumentsCount").Optional()
        .StoreResult(&totalDocumentsCount).DefaultValue(ToString(totalDocumentsCount));
    opts.AddLongOption("baseline", "baseline clusterization filename").Optional()
        .StoreResult(&baselineClusterFileName);

    opts.SetFreeArgsMax(0);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    THashSet<TString> importantUrls;
    if (!!importantUrlsFileName) {
        TFileInput importantIn(importantUrlsFileName);
        TString dataStr;
        while (importantIn.ReadLine(dataStr)) {
            importantUrls.insert(dataStr);
        }
        Cerr << importantUrls.size() << " urls marked as important\n";
    }

    NClusterMetrics::TClusterization clusterization;
    clusterization.Read(clusterFileName);

    NClusterMetrics::TClusterization markup(doRegularization, factor);
    markup.Read(markupFileName);

    if (!baselineClusterFileName) {
        markup.SetupMetrics(clusterization, importantUrls, totalDocumentsCount);

        if (printDetailedInfo && printAlexRecallMetrics) {
            markup.PrintDetailedAlexRecallMetrics(Cout, markup.Flat() && clusterization.Flat());
        } else if (printDetailedInfo && printBCubedMetrics) {
            markup.PrintDetailedBCubedMetrics(Cout);
        } else if (printDetailedInfo) {
            markup.PrintMatchings(clusterization, Cout);
        }

        NClusterMetrics::TClusterMetrics metrics = markup.GetIntegralMetrics();
        NClusterMetrics::TClusterMetrics metricBounds = markup.GetIntegralMetricsBounds();

        if (printAlexRecallMetrics) {
            NClusterMetrics::PrintAlexRecallMetrics(Cout, metrics, metricBounds);
        } else if (printBCubedMetrics) {
            NClusterMetrics::PrintBCubedMetrics(Cout, metrics, metricBounds);
        } else {
            NClusterMetrics::PrintIntegralMetrics(Cout, metrics, metricBounds);
        }

        return 0;
    }

    NClusterMetrics::TClusterization baselineClusterization;
    baselineClusterization.Read(baselineClusterFileName);

    std::pair<NClusterMetrics::TClusterMetrics, NClusterMetrics::TClusterMetrics> compareMetrics =
        NClusterMetrics::TClusterization::Compare(clusterization, baselineClusterization, markup, importantUrls, totalDocumentsCount);

    markup.SetupMetrics(clusterization, importantUrls, totalDocumentsCount);
    NClusterMetrics::TClusterMetrics sampleMetrics = markup.GetIntegralMetrics();
    NClusterMetrics::TClusterMetrics sampleMetricsBounds = markup.GetIntegralMetricsBounds();

    markup.SetupMetrics(baselineClusterization, importantUrls, totalDocumentsCount);
    NClusterMetrics::TClusterMetrics baseMetrics = markup.GetIntegralMetrics();
    NClusterMetrics::TClusterMetrics baseMetricsBounds = markup.GetIntegralMetricsBounds();

    if (printAlexRecallMetrics) {
        NClusterMetrics::PrintAlexRecallMetrics(Cout, compareMetrics.first, compareMetrics.second, "compare metrics");
        NClusterMetrics::PrintAlexRecallMetrics(Cout, sampleMetrics, sampleMetricsBounds, "sample metrics");
        NClusterMetrics::PrintAlexRecallMetrics(Cout, baseMetrics, baseMetricsBounds, "baseline metrics");
    } else if (printBCubedMetrics) {
        NClusterMetrics::PrintBCubedMetrics(Cout, compareMetrics.first, compareMetrics.second, "compare metrics");
        NClusterMetrics::PrintBCubedMetrics(Cout, sampleMetrics, sampleMetricsBounds, "sample metrics");
        NClusterMetrics::PrintBCubedMetrics(Cout, baseMetrics, baseMetricsBounds, "baseline metrics");
    } else {
        NClusterMetrics::PrintIntegralMetrics(Cout, compareMetrics.first, compareMetrics.second, "compare metrics");
        NClusterMetrics::PrintIntegralMetrics(Cout, sampleMetrics, sampleMetricsBounds, "sample metrics");
        NClusterMetrics::PrintIntegralMetrics(Cout, baseMetrics, baseMetricsBounds, "baseline metrics");
    }

    return 0;
}
