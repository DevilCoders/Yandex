#pragma once

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NDistBetweenWords {

    struct TOptions {
        TString InputTable;
        TString PureFile;
        TString OutputTable;
        TString FilterByFreqTable;

        TOptions(int argc, const char **argv) {
            NLastGetopt::TOpts opts;
            opts.SetTitle("Filter data by word freq");
            opts.AddHelpOption();
            opts.SetFreeArgsMax(0);
            opts.AddCharOption('i', NLastGetopt::REQUIRED_ARGUMENT, "input table")
                    .StoreResult(&InputTable);
            opts.AddLongOption("pure-file", "path to file with pure data")
                    .DefaultValue("pure.lg.groupedtrie.rus")
                    .StoreResult(&PureFile);
            opts.AddCharOption('o', NLastGetopt::REQUIRED_ARGUMENT, "path to output table")
                    .StoreResult(&OutputTable);
            opts.AddLongOption("filtered-table", "path to table with filtered rows")
                    .Required()
                    .StoreResult(&FilterByFreqTable);
            NLastGetopt::TOptsParseResult res(&opts, argc, argv);
        }
    };

}
