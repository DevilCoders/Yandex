#pragma once

#include "dictionary.h"

#include <kernel/facts/features_calculator/analyzed_word/analyzed_word.h>

#include <kernel/dssm_applier/utils/utils.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {

        class TPreprocessor {
        public:
            virtual ~TPreprocessor() {}
            virtual void Do(THashMap<TString, TVector<TAnalyzedWord>>& data) const = 0;
        protected:
        };

        class TSubtractLemmasPreprocessor : public TPreprocessor {
        public:
            TSubtractLemmasPreprocessor(const TString& leftInputAnnotation, const TString& rightInputAnnotation, const TString& outputAnnotation)
                : LeftInputAnnotation(leftInputAnnotation)
                , RightInputAnnotation(rightInputAnnotation)
                , OutputAnnotation(outputAnnotation) {}
            void Do(THashMap<TString, TVector<TAnalyzedWord>>& data) const override;
        private:
            void GetTopLemmas(const TVector<TAnalyzedWord>& input, TVector<TUtf16String>& lemmas) const;
            const TString LeftInputAnnotation;
            const TString RightInputAnnotation;
            const TString OutputAnnotation;
        };
    }
}
