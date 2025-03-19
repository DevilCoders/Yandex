#include "normalizer.h"

namespace NCS {
    namespace NNormalizer {
        TFilterStringNormalizer::TFactory::TRegistrator<TFilterStringNormalizer> TFilterStringNormalizer::Registrator(TFilterStringNormalizer::GetTypeName());

        bool TFilterStringNormalizer::DoNormalize(const TStringBuf sbValue, TString& result) const {
            TString valueLocal;
            switch (RegisterNormalization) {
                case ERegisterNormalization::LowerCase:
                    valueLocal = ToLowerUTF8(sbValue);
                    break;
                case ERegisterNormalization::UpperCase:
                    valueLocal = ToUpperUTF8(sbValue);
                    break;
                case ERegisterNormalization::None:
                    valueLocal = ::ToString(sbValue);
                    break;
            }
            if (Filters.empty()) {
                std::swap(valueLocal, result);
                return true;
            }
            TString resultLocal;
            for (auto&& c : valueLocal) {
                bool filtered = false;
                for (auto&& f : Filters) {
                    if (f.GetMinChar() <= c && c <= f.GetMaxChar()) {
                        filtered = true;
                        break;
                    }
                }
                if (filtered) {
                    resultLocal += c;
                }
            }
            std::swap(resultLocal, result);
            return true;
        }

    }
}
