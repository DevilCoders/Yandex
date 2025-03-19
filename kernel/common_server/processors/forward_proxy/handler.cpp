#include "handler.h"

namespace NCS {
    namespace NForwardProxy {
        bool THandlerConfig::InitFeatures(const TYandexConfig::Section* section) {
            const auto& dir = section->GetDirectives();
            dir.GetValue("TargetApiName", TargetApiName);
            return true;
        }

        void THandlerConfig::ToStringFeatures(IOutputStream& os) const {
            os << "TargetApiName: " << TargetApiName << Endl;
        }
    }
}
