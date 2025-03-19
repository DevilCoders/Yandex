#pragma once

#include <kernel/country_data/countries.h>
#include <kernel/groupattrs/metainfo.h>

/**
 * Class for mapping region codes to the corresponding country codes.
 *
 * Note: inside the basesearch use GetCountry() from kernel/relevgeo/geo_regions.h instead,
 *       required data are already loaded in NGroupingAttrs::TDocsAttrs.
 */
class TRegion2Country {
private:
    THolder<NGroupingAttrs::TMetainfo> GeoAttrHolder;
    const NGroupingAttrs::TMetainfo* GeoAttr;

public:
    /**
     * @param[in] geoAttrC2pFile name of geoa.c2p with path
     */
    explicit TRegion2Country(const TString& geoAttrC2pFile);

    /**
     *  @param[in] geoAttr pointer on GeaAttr info
     */
    explicit TRegion2Country(const NGroupingAttrs::TMetainfo* geoAttr);

    /**
     * @param[in] regionId region identifier
     *
     * @return country code or COUNTRY_INVALID
     */
    TCateg GetCountry(TCateg regionId);
};

inline TRegion2Country::TRegion2Country(const TString& geoAttrC2pFile)
{
    GeoAttrHolder.Reset(new NGroupingAttrs::TMetainfo(false));
    GeoAttrHolder->Scan(geoAttrC2pFile.data(), NGroupingAttrs::TMetainfo::C2P);
    GeoAttr = GeoAttrHolder.Get();
}

inline TRegion2Country::TRegion2Country(const NGroupingAttrs::TMetainfo* geoAttr)
    : GeoAttr(geoAttr)
{
}

inline TCateg TRegion2Country::GetCountry(TCateg regionId)
{
    while (regionId != END_CATEG && !IsCountry(regionId))
        regionId = GeoAttr->Categ2Parent(regionId);
    return regionId;
}
