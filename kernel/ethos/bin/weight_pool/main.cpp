#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/cast.h>

int main(int argc, const char** argv) {
    double weight = 1.;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("weight")
            .Required()
            .StoreResult(&weight);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    {
        TString dataStr;
        while (Cin.ReadLine(dataStr)) {
            TStringBuf dataStrBuf(dataStr);

            Cout << dataStrBuf.NextTok('\t') << "\t";
            Cout << dataStrBuf.NextTok('\t') << "\t";
            Cout << dataStrBuf.NextTok('\t') << "\t";

            dataStrBuf.NextTok('\t');

            Cout << weight << "\t" << dataStrBuf << "\n";
        }
    }

    return 0;
}
