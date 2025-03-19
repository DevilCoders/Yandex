#include "config.h"

namespace NCS {
    namespace NFallbackProxy {
        void TServerConfig::Init(const TYandexConfig::Section* section) {
            TBase::Init(section);
        }

        void TServerConfig::DoToString(IOutputStream& os) const {
            TBase::DoToString(os);
        }
    }
}
