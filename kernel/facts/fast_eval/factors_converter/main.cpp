#include <kernel/facts/factors_info/factors_gen.h>

#include <library/cpp/getopt/last_getopt.h>


int main(int argc, char** argv) {
    NLastGetopt::TOpts opts;
    TVector<TString> factors;
    opts.SetTitle("Convert factor names to it number");
    opts.AddCharOption('f', "Factors for analysis")
            .Required()
            .AppendTo(&factors);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    TVector<NUnstructuredFeatures::EFactorId> featuresId;
    for (auto& factor : factors) {
        Cout << factor <<  "\t" << int(FromString<NUnstructuredFeatures::EFactorId>(factor)) << Endl;
    }
}




