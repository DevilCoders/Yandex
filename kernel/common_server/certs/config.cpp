#include "config.h"

#include <library/cpp/mediator/global_notifications/system_status.h>

namespace NCS {
    void TCertsMangerConfig::Init(const TYandexConfig::Section* section) {
        const TYandexConfig::Directives& directives = section->GetDirectives();
        CAFile = directives.Value("CAFile", CAFile);
        AssertCorrectConfig(!CAFile.IsDefined() || CAFile.Exists(), "CAFile not exists: %s", CAFile.GetPath().c_str());

        CAPath = directives.Value("CAPath", CAPath);
        AssertCorrectConfig(!CAPath.IsDefined() || CAPath.Exists(), "CAPath not exists: %s", CAPath.GetPath().c_str());
    }

    void TCertsMangerConfig::ToString(IOutputStream& os) const {
        os << "CAFile: " << CAFile << Endl;
        os << "CAPath: " << CAPath << Endl;
    }
}
