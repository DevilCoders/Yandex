#include <library/cpp/on_disk/aho_corasick/writer.h>
#include <library/cpp/on_disk/aho_corasick/helpers.h>
#include <library/cpp/getopt/small/last_getopt.h>

#include <util/stream/file.h>

int main(int argc, char** argv) {
    NLastGetopt::TOpts options = NLastGetopt::TOpts::Default();

    TString inputLines;
    options
        .AddLongOption('i', "input", "file name with the string set to build aho corasick from")
        .Required()
        .RequiredArgument("FILE")
        .StoreResult(&inputLines);

    TString outputAho;
    options
        .AddLongOption('o', "output", "output file to contain the built aho corasick")
        .Required()
        .RequiredArgument("FILE")
        .StoreResult(&outputAho);

    options.SetFreeArgsNum(0);

    NLastGetopt::TOptsParseResult arguments(&options, argc, argv);

    TDefaultAhoCorasickBuilder builder;
    TFileInput input(inputLines);
    for (TString line; input.ReadLine(line);) {
        builder.AddString(line, 0);
    }

    const auto blob = builder.Save();

    {
        TUnbufferedFileOutput output(outputAho);
        output.Write(blob.Data(), blob.Size());
    }

    return 0;
}
