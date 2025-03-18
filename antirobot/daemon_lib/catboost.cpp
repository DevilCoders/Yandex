#include "catboost.h"

#include "cacher_factors.h"
#include "factor_names.h"


namespace NAntiRobot {

template <>
void TCatboostClassificator<TCacherLinearizedFactors>::InitCacher(const TString& formulaFilename) {
	LoadModel<TClassificator::TLoadError>(formulaFilename);

	for (const auto& floatFeature : Model.ModelTrees->GetFloatFeatures()) {
		try {
			FactorsRemap.push_back(TRawCacherFactors::GetIndexByName(floatFeature.FeatureId));
		} catch (const yexception& exc) {
			throw yexception()
				<< "failed to initialize formula: " << formulaFilename
				<< ": " << exc.what();
		}
	}
}

template <>
void TCatboostClassificator<TProcessorLinearizedFactors>::InitProcessor(const TString& formulaFilename) {
	LoadModel<TClassificator<TProcessorLinearizedFactors>::TLoadError>(formulaFilename);

    const TFactorNames* fn = TFactorNames::Instance();
	for (const auto& floatFeature : Model.ModelTrees->GetFloatFeatures()) {
        FactorsRemap.push_back(fn->GetFactorIndexByName(floatFeature.FeatureId));
	}
}

} // namespace NAntiRobot
