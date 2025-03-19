#include "config.h"

namespace NCS {

    void ISettingsConfig::Init(const TYandexConfig::Section* section) {
        AssertCorrectConfig(section, "Incorrect section for config");
        return DoInit(*section);
    }

    void ISettingsConfig::ToString(IOutputStream& os) const {
        return DoToString(os);
    }

}
