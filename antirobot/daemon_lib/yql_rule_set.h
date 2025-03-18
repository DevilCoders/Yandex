#pragma once

#include "factors.h"
#include "yql_io_spec.h"

#include <yql/library/purecalc/common/interface.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NAntiRobot {


class TYqlRuleSet {
public:
    TYqlRuleSet() = default;
    explicit TYqlRuleSet(const TVector<TString>& rules);

    bool Empty() const;

    TVector<ui32> Match(const TProcessorLinearizedFactors& factors);

private:
    class TOutputConsumer final:
        public NYql::NPureCalc::IConsumer<const TVector<ui32>*>
    {
    public:
        void OnObject(const TVector<ui32>* result) override {
            Result = result;
        }

        void OnFinish() override {}

        const TVector<ui32>& GetResult() const {
            return *Result;
        }

    private:
        const TVector<ui32>* Result;
    };

private:
    THolder<NYql::NPureCalc::TPushStreamProgram<
        TYqlInputSpec, TRuleYqlOutputSpec
    >> Program;

    THolder<NYql::NPureCalc::IConsumer<const TVector<float>*>> InputConsumer;
    const TOutputConsumer* OutputConsumer = nullptr;
};


}
