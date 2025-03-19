#include "normalizer.h"

namespace NCS {
    namespace NNormalizer {
        TStringNormalizerNumeric::TFactory::TRegistrator<TStringNormalizerNumeric> TStringNormalizerNumeric::Registrator(TStringNormalizerNumeric::GetTypeName());

        bool TStringNormalizerNumeric::DoNormalize(const TStringBuf sbValue, TString& result) const {
            TString resultLocal;
            for (auto&& i : sbValue) {
                if (i >= '0' && i <= '9') {
                    resultLocal += i;
                }
            }
            std::swap(result, resultLocal);
            return true;
        }

    }
}
