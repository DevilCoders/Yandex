#include <util/generic/string.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>

#include <library/cpp/getopt/last_getopt.h>

#include "json_to_jsonlines.h"

struct TOptions {
    TString InputFileName;
    TString OutputFileName;
    bool ConvertToJsonLines = true;
    bool Reverse = false;
};

TOptions ParseOptions(int argc, const char* argv[]) {
    TOptions res;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('h', "help", "Print usage").NoArgument().Handler1([&](const NLastGetopt::TOptsParser* p) {
        p->PrintUsage(Cout);
        Cout << Endl;
        exit(0);
    });
    opts.SetFreeArgsNum(0);

    opts
        .AddLongOption('i', "input", "Input file name (- for stdin)")
        .Required()
        .StoreResult(&res.InputFileName);

    opts
        .AddLongOption('o', "output", "Output file name (- for stdout)")
        .Required()
        .StoreResult(&res.OutputFileName);

    opts
        .AddLongOption('n', "no-conversion", "Disable conversion")
        .Optional()
        .NoArgument()
        .StoreValue<bool>(&res.ConvertToJsonLines, false);

    opts
        .AddLongOption('r', "reverse", "Reverse mode")
        .Optional()
        .NoArgument()
        .StoreValue(&res.Reverse, true);

    NLastGetopt::TOptsParseResult parseRes(&opts, argc, argv);

    return res;
}

int main(int argc, const char* argv[]) {

    try {
        TOptions options = ParseOptions(argc, argv);
        NJsonToJsonLines::TJsonToJsonLinesStreamParser parser(
            options.InputFileName,
            options.OutputFileName,
            options.ConvertToJsonLines,
            options.Reverse);
        parser.RunConversion();

    } catch (const yexception& exc) {
        Cerr << "Error occured (main): " << exc << Endl;
        return 1;
    }

    return 0;
}
