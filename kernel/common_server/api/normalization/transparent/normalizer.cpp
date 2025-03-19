#include "normalizer.h"

namespace NCS {
    namespace NNormalizer {
        TStringNormalizerTransparent::TFactory::TRegistrator<TStringNormalizerTransparent> TStringNormalizerTransparent::Registrator(TStringNormalizerTransparent::GetTypeName());

        bool TStringNormalizerTransparent::DoNormalize(const TStringBuf sbValue, TString& result) const {
            result = TString(sbValue.data(), sbValue.size());
            return true;
        }

    }
}
