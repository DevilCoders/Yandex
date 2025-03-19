#pragma once

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NDistBetweenWords {

    struct TOptions {
        TString InputTable;
        TVector<TString> FactorNames;
        TString OutputFile;
        int FreqThreshold;

        TOptions(int argc, const char **argv) {
            NLastGetopt::TOpts opts;
            opts.SetTitle("Find queries with diff only in one word");
            opts.AddHelpOption();
            opts.SetFreeArgsMax(0);
            opts.AddCharOption('i', NLastGetopt::REQUIRED_ARGUMENT, "input table")
                    .StoreResult(&InputTable);
            opts.AddLongOption("factor-names", "name of factors to load from MR table")
                    .AppendTo(&FactorNames);
            opts.AddCharOption('o', NLastGetopt::REQUIRED_ARGUMENT, "path to output file")
                    .StoreResult(&OutputFile);
            opts.AddLongOption("freq-threshold", "words freq threshold to filter")
                    .DefaultValue(0)
                    .StoreResult(&FreqThreshold);
            NLastGetopt::TOptsParseResult res(&opts, argc, argv);

            if (!FactorNames) {
                FactorNames = {"host5", "host10", "urls5", "urls10"};
            }
        }
    };

}
