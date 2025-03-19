#pragma once

#include <util/generic/fwd.h>

#include <library/cpp/yconf/conf.h>

#include <kernel/common_server/util/datetime/datetime.h>
#include <kernel/common_server/util/accessor.h>

namespace NCS {

    class TGeobaseConfig {
    private:
        CSA_READONLY_DEF(TString, Path);
        CSA_FLAG(TGeobaseConfig, LockMemory, false);
        CSA_FLAG(TGeobaseConfig, Preloading, false);
        CSA_FLAG(TGeobaseConfig, Enabled, false);
        CSA_READONLY(TTimeZoneHelper, DefaultTimeZone, TTimeZoneHelper::FromHours(3));
        CSA_READONLY(i32, DefaultRegionId, 225);

    public:
        void Init(const TYandexConfig::Section* section);
        void ToString(IOutputStream& os) const;
    };

}
