#include "matrixnet.h"

#include "factor_names.h"

#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/system/rwlock.h>

namespace NAntiRobot {

namespace {

const TString FACTOR_NAMES_KEY = "factor_names";

} // anonymous namespace

template<>
TMatrixNetClassificator<TProcessorLinearizedFactors>::TMatrixNetClassificator(const TString& formulaFilename) {
    using TError = typename TClassificator<TProcessorLinearizedFactors>::TLoadError;

    if (formulaFilename.empty()) {
        ythrow TError() << "Formula filename is empty";
    }

    try {
        TFileInput fin(formulaFilename);
        ::Load(&fin,Formula);
    } catch (yexception& e) {
        ythrow TError() << "Failed to load matrixnet formula from"
                        << formulaFilename.Quote() << ":" << Endl << e.what();
    }

    if (Formula.Empty()) {
        ythrow TError() << "Got empty matrixnet formula from " << formulaFilename.Quote();
    }

    if (Formula.Info.find(FACTOR_NAMES_KEY) == Formula.Info.end()) {
        ythrow TError() << "Matrixnet formula file " << formulaFilename.Quote()
                        << " doesn't contain \"" << FACTOR_NAMES_KEY
                        << "\" property";
    }

    TVector<TString> factorNames = SplitString(Formula.Info[FACTOR_NAMES_KEY], "\t");
    const size_t requiredFactorSize = Formula.MaxFactorIndex() + 1;
    if (requiredFactorSize > factorNames.size()) {
        ythrow TError() << "Wrong factors count in file " << formulaFilename.Quote()
                        << ". Factor names count: " << factorNames.size()
                        << ". Factors count: " << requiredFactorSize;
    }

    const TFactorNames* fn = TFactorNames::Instance();
    for (size_t i = 0; i < factorNames.size(); ++i) {
        FactorsRemap.push_back(fn->GetFactorIndexByName(factorNames[i]));
    }
}

} // namespace NAntiRobot
