#include "calc_target_metrics.h"

#include <library/cpp/getopt/last_getopt.h>

int main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddCharOption('p', "pairwise").NoArgument();
    opts.AddCharOption('i', "input file").RequiredArgument("FILE").DefaultValue("features.txt");
    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
    bool pairwise = optsParseResult.Has('p');
    TString inputFile(optsParseResult.Get('i'));

    TMetricsCalculator metricsCalculator(inputFile, pairwise);
    metricsCalculator.PrintMetrics();

    return 0;
}
