#pragma once

#include "dictionary.h"
#include "features_calculator.h"
#include "preprocessor.h"
#include "weighter.h"

#include <kernel/facts/factors_info/factor_names.h>
#include <kernel/facts/features_calculator/analyzed_word/analyzed_word.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {

        struct TCalculatorInput {
            TString InputAnnotation;
            TDictionary* Dictionary = nullptr;
            TWeighter* Weighter = nullptr;
        };

        struct TFeatureGroup {
            TCalculatorInput LeftInput;
            TCalculatorInput RightInput;
            TVector<THolder<TFeaturesCalculator>> Calculators;
        };

        class TWordEmbeddingFeaturesBuilder {
        public:
            TWordEmbeddingFeaturesBuilder(const TString& configFileName, const THashMap<TString, TString>& resourceFiles);
            void BuildFeatures(THashMap<TString, TVector<TAnalyzedWord>>& input, TFactFactorStorage& features) const;

        private:
            void GetWordEmbeddings(TDictionary* dictionary, TWeighter* weighter, const TVector<TAnalyzedWord>& analyzedWords, TVector<TEmbedding>& embeddings) const;
            void BuildPreprocessor(const NSc::TDict& config);
            void BuildWeighter(const NSc::TDict& config);
            void BuildCalculatorInput(const NSc::TDict& config, TCalculatorInput& input) const;
            void BuildCalculator(const NSc::TDict& config, TFeatureGroup& featureGroup) const;

            THashMap<TString, TString> ResourceFiles;
            THashMap<TString, THolder<TDictionary>> Dictionaries;
            THashMap<TString, THolder<TWeighter>> Weighters;
            TVector<THolder<TPreprocessor>> Preprocessors;
            TVector<TFeatureGroup> FeatureGroups;
        };
    }
}
