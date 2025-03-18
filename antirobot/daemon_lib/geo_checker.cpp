#include "geo_checker.h"

#include <antirobot/lib/addr.h>
#include <kernel/geo/utils.h>

namespace {
#include "geo_timeoffset.inc"
}

namespace NAntiRobot {
    TGeoChecker::TGeoChecker() {
    }

    TGeoChecker::TGeoChecker(const TString& geoDataBinPath)
        : RegTimeOffs(TIMEOFFSET_DATA_SIZE)
        , GeobaseLookup(new TGeobaseLookup(geoDataBinPath))
    {
        LoadHash(RegTimeOffs);
    }

    float TGeoChecker::GetRemoteTime(TGeoRegion geoRegion, TInstant timeOfIp) const {
        time_t tTime = timeOfIp.TimeT();

        int timeOffs = 0;
        TRegTimeOffsHash::const_iterator it = RegTimeOffs.find(geoRegion);
        if (it != RegTimeOffs.end())
            timeOffs = it->second;

        tTime += timeOffs;

        return float(tTime % (24 * 60 * 60));
    }

    bool TGeoChecker::IsEuropean(TGeoRegion geoRegion) const {
        if (!GeobaseLookup) {
            return false;
        }

        const auto region = GeobaseLookup->GetRegionById(geoRegion);
        return region.IsFieldExists("is_eu") && region.GetBoolField("is_eu");
    }

    ui32 TGeoChecker::GetCountryId(TGeoRegion geoRegion) const {
        if (!GeobaseLookup) {
            return 0;
        }
        return GeobaseLookup->GetCountryId(geoRegion);
    }

    const TGeobaseLookup& TGeoChecker::GetGeobase() const {
        return *GeobaseLookup;
    }

    void TGeoChecker::LoadHash(TRegTimeOffsHash& hash) {
        for (size_t i = 0; i < TIMEOFFSET_DATA_SIZE; i++)
            hash[TIMEOFFSET_DATA[i].regionid] = TIMEOFFSET_DATA[i].offset;
    }
}

