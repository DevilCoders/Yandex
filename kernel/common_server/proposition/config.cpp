#include "config.h"

namespace NCS {
    namespace NPropositions {
        void TDBManagerConfig::Init(const TYandexConfig::Section* section) {
            TBase::Init(section);
            NeedApprovesCount = section->GetDirectives().Value("NeedApprovesCount", NeedApprovesCount);
        }

        void TDBManagerConfig::ToString(IOutputStream& os) const {
            TBase::ToString(os);
            os << "NeedApprovesCount: " << NeedApprovesCount << Endl;
        }
    }
}
