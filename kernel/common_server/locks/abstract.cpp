#include "abstract.h"
#include "fake.h"
#include <library/cpp/mediator/global_notifications/system_status.h>

void TLocksManagerConfigContainer::BuildFake() {
    Config = MakeAtomicShared<TFakeLocksManagerConfig>();
}

void TLocksManagerConfigContainer::Init(const TYandexConfig::Section* section) {
    const TString type = section->GetDirectives().Value<TString>("Type", "fake");
    Config = ILocksManagerConfig::TFactory::Construct(type);
    AssertCorrectConfig(!!Config, "Incorrect locks manager type: %s", type.data());
    Config->Init(section);
}
