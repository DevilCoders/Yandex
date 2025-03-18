#include <antirobot/daemon_lib/factor_names.h>
#include <antirobot/daemon_lib/classificator.h>

#include <util/string/vector.h>
#include <util/system/backtrace.h>

using namespace NAntiRobot;

namespace {

void HandleUncaughtException() {
    Cerr << TString(80, '*') << Endl;
    Cerr << "UNCAUGHT EXCEPTION! Exception stack trace:" << Endl;
    PrintBackTrace();
    Cerr << "Exception message:" << Endl
         << CurrentExceptionMessage();
    std::exit(1);
}

}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        Cerr << "Usage: check_formula <formula .info-file>" << Endl;
        return 1;
    }

    std::set_terminate(HandleUncaughtException);

    THolder<TClassificator<TProcessorLinearizedFactors>> classify{CreateProcessorClassificator(argv[1])};

    const TFactorNames* fn = TFactorNames::Instance();

    TString line;
    while (Cin.ReadLine(line)) {
        TVector<TString> fields = SplitString(line, "\t");
        if (fields.size() != fn->FactorsCount()) {
            Cerr << "invalid line (expected " << fn->FactorsCount()
                             << " fields but only " << fields.size() << " found): " << line
                             << Endl;
            continue;
        }

        TProcessorLinearizedFactors factors;
        factors.reserve(fn->FactorsCount());
        for (auto i = fields.begin(); i != fields.end(); ++i) {
            factors.push_back(FromString<float>(*i));
        }

        float matrixNetRes = (*classify)(factors);
        Cout << matrixNetRes << Endl;
    }
    return 0;
}
