#pragma once

#include "classificator.h"

#include <catboost/libs/model/model.h>

#include <util/stream/file.h>

#include <type_traits>


namespace NAntiRobot {

template <typename T>
class TCatboostClassificator : public TClassificator<T> {
public:
	template <
		typename U = T,
		typename std::enable_if_t<std::is_same_v<U, TCacherLinearizedFactors>, int> = 0
	>
    explicit TCatboostClassificator(const TString& formulaFilename) {
		static_assert(std::is_same_v<T, U>); // U is necessary for SFINAE.
		InitCacher(formulaFilename);
	}

	template <
		typename U = T,
		typename std::enable_if_t<std::is_same_v<U, TProcessorLinearizedFactors>, int> = 0
	>
	explicit TCatboostClassificator(const TString& formulaFilename) {
		InitProcessor(formulaFilename);
	}

private:
	void InitCacher(const TString& formulaFilename);

	void InitProcessor(const TString& formulaFilename);

	template <typename TError>
	void LoadModel(const TString& formulaFilename) {
		Y_ENSURE_EX(!formulaFilename.empty(), TError() << "Formula filename is empty");

		try {
			TFileInput fin(formulaFilename);
			Model.Load(&fin);
		} catch (yexception& e) {
			ythrow TError() << "Failed to load catboost formula from"
				<< formulaFilename.Quote() << ":" << e.what();
		}
	}

    double Classify(const T& remappedFactors) const final {
		std::array<double, 1> result;
		Model.Calc(remappedFactors, {}, result);

		return result[0];
	}

    TFullModel Model;
};

} // namespace NAntiRobot
