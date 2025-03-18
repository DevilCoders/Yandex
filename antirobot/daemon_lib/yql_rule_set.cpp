#include "yql_rule_set.h"

#include <util/string/builder.h>


namespace NAntiRobot {


namespace {
    TYqlInputSpec MakeInputSpec() {
        TVector<TString> keys;
        keys.reserve(TAllFactors::AllFactorsCount());

        for (size_t i = 0; i < TAllFactors::AllFactorsCount(); ++i) {
            const auto fancyKey = TAllFactors::GetFactorNameByIndex(i);

            TString key;
            key.reserve(fancyKey.size() + TStringBuf("_aggr").length() - 1);

            for (char c : fancyKey) {
                switch (c) {
                case '^':
                    key += "_aggr";
                    break;
                case '|':
                    key += '_';
                    break;
                default:
                    key += c;
                    break;
                }
            }

            keys.push_back(key);
        }

        return TYqlInputSpec(keys);
    }


    const TYqlInputSpec& GetInputSpec() {
        static TYqlInputSpec inputSpec = MakeInputSpec();
        return inputSpec;
    }
}


TYqlRuleSet::TYqlRuleSet(const TVector<TString>& rules) {
    if (rules.empty()) {
        return;
    }

    TStringBuilder programText;
    programText << "SELECT";

    for (size_t i = 0; i < rules.size(); ++i) {
        programText << ' ' << rules[i] << " AS Y" << i << ',';
    }

    programText.pop_back();
    programText << " FROM Input";

    const auto factory = NYql::NPureCalc::MakeProgramFactory();
    Program = factory->MakePushStreamProgram(
        GetInputSpec(), TRuleYqlOutputSpec(rules.size()),
        programText
    );

    auto outputConsumerHolder = MakeHolder<TOutputConsumer>();
    OutputConsumer = outputConsumerHolder.Get();

    InputConsumer = Program->Apply(std::move(outputConsumerHolder));
}

bool TYqlRuleSet::Empty() const {
    return !Program;
}

TVector<ui32> TYqlRuleSet::Match(const TProcessorLinearizedFactors& factors) {
    if (!Program) {
        return {};
    }

    InputConsumer->OnObject(&factors);
    return OutputConsumer->GetResult();
}


}
