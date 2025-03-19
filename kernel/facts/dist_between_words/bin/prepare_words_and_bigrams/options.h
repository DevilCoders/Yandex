#pragma once

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NDistBetweenWords {

    struct TOptions {
        TString InputTable;
        TString PureFile;
        TString SearchSynTrie;
        TString OutputTable;
        int Threshold;

        TOptions(int argc, const char **argv) {
            NLastGetopt::TOpts opts;
            opts.SetTitle("Filter data by word freq");
            opts.AddHelpOption();
            opts.SetFreeArgsMax(0);
            opts.AddCharOption('i', "input table")
                    .Required()
                    .StoreResult(&InputTable);
            opts.AddLongOption("pure-file", "path to file with pure data")
                    .DefaultValue("pure.lg.groupedtrie.rus")
                    .StoreResult(&PureFile);
            opts.AddCharOption('o', "path to output table")
                    .Required()
                    .StoreResult(&OutputTable);
            opts.AddLongOption("threshold", "threshold for words")
                    .DefaultValue(1000)
                    .StoreResult(&Threshold);
            opts.AddLongOption("search-syn-file", "file with search synonymous trie")
                    .StoreResult(&SearchSynTrie);

            NLastGetopt::TOptsParseResult res(&opts, argc, argv);
        }
    };

}
