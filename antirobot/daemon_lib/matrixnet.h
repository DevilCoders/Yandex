#pragma once

#include "classificator.h"

#include <kernel/matrixnet/mn_dynamic.h>

namespace NAntiRobot {

template<typename T>
class TMatrixNetClassificator : public TClassificator<T> {
public:
    explicit TMatrixNetClassificator(const TString& formulaFilename);

private:
    double Classify(const T& remappedFactors) const final {
		return Formula.CalcRelev(remappedFactors);
	}

    NMatrixnet::TMnSseDynamic Formula;
};

} // namespace NAntiRobot
