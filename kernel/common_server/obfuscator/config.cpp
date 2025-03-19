#include "config.h"

namespace NCS {
    namespace NObfuscator {

        void TDBManagerConfig::Init(const TYandexConfig::Section* section) {
            TBase::Init(section);
            TotalObfuscateByDefault = section->GetDirectives().Value("TotalObfuscateByDefault", TotalObfuscateByDefault);
        }

        void TDBManagerConfig::ToString(IOutputStream& os) const {
            TBase::ToString(os);
            os << "TotalObfuscateByDefault: " << TotalObfuscateByDefault << Endl;
        }

    }
}
