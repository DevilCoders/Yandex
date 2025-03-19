#pragma once

#include <kernel/groupattrs/metainfo.h> // for NGroupingAttrs::TMetainfo

#include <library/cpp/deprecated/calc_module/simple_module.h>

class TGeoaHolderModule : public TSimpleModule {
private:
    const TString GeoaFileName;
    THolder<const NGroupingAttrs::TMetainfo> AttrGeoHolder;
    const NGroupingAttrs::TMetainfo* AttrGeo;

public:
    TGeoaHolderModule(const TString& geoaFileName)
        : TSimpleModule("TGeoaHolderModule")
        , GeoaFileName(geoaFileName)
        , AttrGeo(nullptr)
    {
        Bind(this).To<&TGeoaHolderModule::Init>("init");
        Bind(this).To<const NGroupingAttrs::TMetainfo* const*>(&AttrGeo, "geoa_output");
    }

private:
    void Init() {
        NGroupingAttrs::TMetainfo* ptr = nullptr;
        AttrGeoHolder.Reset(AttrGeo = ptr = new NGroupingAttrs::TMetainfo(/*dynamicC2N = */ false));
        ptr->Scan(GeoaFileName.data(), NGroupingAttrs::TMetainfo::C2P);
    }
};
