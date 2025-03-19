#include "config.h"
#include "queue.h"
#include <library/cpp/logger/init_context/yconf.h>
#include <library/cpp/mediator/global_notifications/system_status.h>

namespace NCS {
    TPQFakeConfig::TFactory::TRegistrator<TPQFakeConfig> TPQFakeConfig::Registrator(TPQFakeConfig::GetTypeName());
    THolder<IPQClient> TPQFakeConfig::DoConstruct(const IPQConstructionContext& context) const {
        return MakeHolder<TPQFake>(GetClientId(), *this, context);
    }

    void TPQFakeConfig::DoInit(const TYandexConfig::Section* /*section*/) {
    }

    void TPQFakeConfig::DoToString(IOutputStream& /*os*/) const {
    }
}
