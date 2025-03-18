#include <antirobot/daemon_lib/factor_names.h>

#include <library/cpp/digest/old_crc/crc.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/opt2.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

using namespace NAntiRobot;

void PrintHelp() {
    Cout << "Usage:" << Endl;
    Cout << "\t-h\t\tthis help" << Endl
         << "\t-H <VERSION>\tcalculate ui64 hash for the list of factor names using specified version" << Endl
         << "\t\t\tif <VERSION> is zero, then use FACTORS_VERSION value from daemon_lib/factors.h" << Endl
         ;
}

void PrintFactors(IOutputStream& out) {
    const TFactorNames* fn = TFactorNames::Instance();

    for (size_t i = 0; i < TAllFactors::NumLocalFactors; ++i) {
        if (i) {
            out << "," << Endl;
        }

        out << '"' << fn->GetLocalFactorNameByIndex(i) << '"';
    }
    out << Endl;
}

ui64 CalcHash(const TString& str) {
    return crc64(str.c_str(), str.size());
}

int main(int argc, char* const argv[]) {
    Opt2 options(argc, argv, "hH:");
    long factorsVersion = options.Int('H', "factors version", -1, true);

    if (options.Has('h', "help")) {
        PrintHelp();
        return 2;
    }

    if (factorsVersion >= 0) {
        int ver = factorsVersion ? factorsVersion : NAntiRobot::FACTORS_VERSION;
        TString str;
        TStringOutput out(str);
        out << ver << Endl;
        PrintFactors(out);

        Cout << CalcHash(str) << "ULL" << Endl;
        return 0;
    }

    PrintFactors(Cout);
    return 0;
}
