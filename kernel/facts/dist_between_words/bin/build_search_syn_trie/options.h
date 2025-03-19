#pragma once

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NDistBetweenWords {

    struct TOptions {
        TString InputTable;
        TString OutputFile;

        TOptions(int argc, const char **argv) {
            NLastGetopt::TOpts opts;
            opts.SetTitle("Build trie with search synonyms");
            opts.AddHelpOption();
            opts.SetFreeArgsMax(0);
            opts.AddCharOption('i', "input table")
                    .Required()
                    .StoreResult(&InputTable);
            opts.AddCharOption('o', "path to output file")
                    .Required()
                    .StoreResult(&OutputFile);

            NLastGetopt::TOptsParseResult res(&opts, argc, argv);
        }
    };

}
