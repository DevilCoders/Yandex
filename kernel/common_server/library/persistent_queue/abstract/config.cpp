#include "config.h"
#include <library/cpp/mediator/global_notifications/system_status.h>
#include "pq.h"

namespace NCS {

    IPQClient::TPtr IPQClientConfig::Construct(const IPQConstructionContext& context) const {
        THolder<IPQClient> result = DoConstruct(context);
        return result.Release();
    }

    void IPQClientConfig::Init(const TYandexConfig::Section* section) {
        ClientId = section->Name;
        AssertCorrectConfig(!!ClientId, "Incorrect client id for pq client");
        DoInit(section);
    }

    void IPQClientConfig::ToString(IOutputStream& os) const {
        os << "ClientId: " << ClientId << Endl;
        DoToString(os);
    }

}
