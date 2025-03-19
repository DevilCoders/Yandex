#include "config.h"
#include "settings.h"

namespace NCS {
    TFakeSettingsConfig::TFactory::TRegistrator<TFakeSettingsConfig> TFakeSettingsConfig::Registrator(TFakeSettingsConfig::GetTypeName());

    ISettings::TPtr TFakeSettingsConfig::Construct(const IBaseServer& /*server*/) const {
        return MakeAtomicShared<TFakeSettings>(*this);
    }

}
