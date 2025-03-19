#include <kernel/ethos/lib/reg_tree_applier_lib/model.h>
#include <library/cpp/getopt/last_getopt.h>
#include <util/stream/file.h>

int main(int argc, const char** argv) {
    TString modelFileName;

    try {
        NLastGetopt::TOpts opts;

        opts.AddLongOption('m', "model", "model filename")
            .StoreResult(&modelFileName);

        opts.SetFreeArgsMax(0);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    } catch (yexception& ex) {
        Cerr << "Error parsing command line options: " << ex.what() << "\n";
        return 1;
    }

    NRegTree::TCompactModel model;
    {
        TFileInput modelIn(modelFileName);
        model.Load(&modelIn);
    }

    TString dataStr;
    while (Cin.ReadLine(dataStr)) {
        TStringBuf dataStrBuf(dataStr);

        const TStringBuf queryId = dataStrBuf.NextTok('\t');
        const TStringBuf target  = dataStrBuf.NextTok('\t');
        const TStringBuf url = dataStrBuf.NextTok('\t');
        const TStringBuf weight = dataStrBuf.NextTok('\t');

        TVector<float> features;
        while (dataStrBuf) {
            features.push_back(FromString<float>(dataStrBuf.NextTok('\t')));
        }

        Cout << queryId << "\t" << target << "\t" << url << "\t" << weight << "\t" << model.Prediction(features) << "\n";
    }

    return 0;
}
