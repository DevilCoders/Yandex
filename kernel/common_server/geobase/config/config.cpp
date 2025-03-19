#include "config.h"

#include <util/folder/path.h>

#include <library/cpp/mediator/global_notifications/system_status.h>

namespace NCS {

    void TGeobaseConfig::Init(const TYandexConfig::Section* section) {
        Path = section->GetDirectives().Value("Path", Default<TString>());
        EnabledFlag = section->GetDirectives().Value("Enabled", EnabledFlag);
        if (EnabledFlag) {
            AssertCorrectConfig(TFsPath(Path).Exists(), "Incorrect geobase filename %s", Path.data());
        }
        LockMemoryFlag = section->GetDirectives().Value("LockMemory", LockMemoryFlag);
        PreloadingFlag = section->GetDirectives().Value("Preloading", PreloadingFlag);
        DefaultTimeZone = TTimeZoneHelper::FromMinutes(section->GetDirectives().Value("DefaultTimeZone", DefaultTimeZone.Minutes()));
        DefaultRegionId = section->GetDirectives().Value("DefaultRegionId", DefaultRegionId);
    }

    void TGeobaseConfig::ToString(IOutputStream& os) const {
        os << "Path: " << Path << Endl;
        os << "LockMemory: " << LockMemoryFlag << Endl;
        os << "Preloading: " << PreloadingFlag << Endl;
        os << "Enabled: " << EnabledFlag << Endl;
        os << "DefaultTimeZone" << DefaultTimeZone.Minutes() << Endl;
        os << "DefaultRegionId" << DefaultRegionId << Endl;
    }

}
