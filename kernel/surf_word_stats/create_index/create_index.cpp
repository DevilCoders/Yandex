#include <kernel/surf_word_stats/lib/builder.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

int main_create_surf(int argc, const char** argv)
{
    TString ywFile;
    TString outFile;

    NLastGetopt::TOpts options;

    options
        .AddCharOption('i', "--  input file")
        .Required()
        .RequiredArgument("FILE")
        .StoreResult(&ywFile);

    options
        .AddCharOption('o', "--  output")
        .Required()
        .RequiredArgument("FILE")
        .StoreResult(&outFile);

    NLastGetopt::TOptsParseResult optParsing(&options, argc, argv);
    Y_UNUSED(optParsing);

    NWordFeatures::BuildSurfIndex(ywFile, outFile, true);

    return 0;
}

int main(int argc, const char** argv)
{
    TModChooser modChooser;

    modChooser.AddMode(
        "build",
        main_create_surf,
        "-- creates surf word trie");

    return modChooser.Run(argc, argv);
}
