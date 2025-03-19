#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/vector.h>
#include <util/system/defaults.h>

typedef ui32 TSentenceZones;

namespace NZoneFactors {
    enum TZoneFactorType { zftBM25 = 0, zftCM, zftCZL, zftZL, zftCZ, zftIF, zftNoZone };
}

class ISentenceZonesReader {
public:
    //zoneId is power of 2. TSentenceZones result is 'or' operation all zones for sentence.
    virtual TSentenceZones GetSentZones(const ui32 docId, const ui32 sent) const = 0;
    //zoneNumber is bit number in zoneId
    virtual bool GetFeatureIndex(const NZoneFactors::TZoneFactorType factor, const ui32 zoneNumber, const EFormClass matchLevel, ui32& featureIndex) const = 0;
    virtual ui32 GetNumberOfZones() const = 0;
    virtual ui32 GetZoneLength(const ui32 docId, const ui32 zoneNumber) const = 0;
    virtual ui32 GetZoneAvgLength(const ui32 zoneNumber) const = 0;
    virtual ~ISentenceZonesReader() {}
};
