#pragma once

#include <antirobot/lib/addr.h>

#include <kernel/geo/utils.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>

namespace NAntiRobot {
    class TAddr;

    class TGeoChecker {
    public:
        TGeoChecker();
        TGeoChecker(const TString& geodataBinPath);
        TGeoChecker(TGeoChecker&&) = default;

        TMaybe<NGeobase::TIpBasicTraits> GetTraits(const TAddr& addr) const {
            if (!GeobaseLookup) {
                return Nothing();
            }
            return GeobaseLookup->GetBasicTraitsByIp(addr.ToString());
        }

        float GetRemoteTime(TGeoRegion geoRegion, TInstant localtime) const;
        ui32 GetCountryId(TGeoRegion geoRegion) const;
        bool IsEuropean(TGeoRegion geoRegion) const;
        const TGeobaseLookup& GetGeobase() const;
    private:
        typedef THashMap<TGeoRegion, int> TRegTimeOffsHash;
        TRegTimeOffsHash RegTimeOffs;

        THolder<TGeobaseLookup> GeobaseLookup;

    private:
        static void LoadHash(TRegTimeOffsHash& hash);
    };
}

