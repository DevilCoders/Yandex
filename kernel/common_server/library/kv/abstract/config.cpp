#include "config.h"
#include <library/cpp/mediator/global_notifications/system_status.h>

namespace NCS {
    namespace NKVStorage {
        void IConfig::Init(const TYandexConfig::Section* section) {
            StorageName = section->GetDirectives().Value("StorageName", StorageName);
            ThreadsCount = section->GetDirectives().Value("ThreadsCount", ThreadsCount);
            AssertCorrectConfig(!!StorageName, "incorrect StorageName in KVStorage config");
            DoInit(section);
        }

        void IConfig::ToString(IOutputStream& os) const {
            os << "StorageName: " << StorageName << Endl;
            os << "ThreadsCount: " << ThreadsCount << Endl;
            DoToString(os);
        }

    }
}
