#pragma once

#include <library/cpp/deprecated/calc_module/copy_points.h>
#include <library/cpp/deprecated/calc_module/simple_module.h>

#include <kernel/erfcreator/canonizers.h> // for TCanonizers

class TMirrorOwnerModule : public TSimpleModule {
private:
    TMasterCopyPoint<const TCanonizers*> CanonizersInputPoint;
    const TCanonizers* Canonizers;

public:
    TMirrorOwnerModule()
        : TSimpleModule("TMirrorOwnerModule")
        , CanonizersInputPoint(this, /*default=*/nullptr, "canonizers_input")
    {
        Bind(this).To<const TString&, TString&, &TMirrorOwnerModule::GetMirrorOwner>("mirror_owner_output");
        Bind(this).To<&TMirrorOwnerModule::Init>("init");
        AddInitDependency("canonizers_input");
    }

private:
    void Init() {
        Canonizers = CanonizersInputPoint.GetValue();
    }
    void GetMirrorOwner(const TString& host, TString& ownerName) {
        const TString moHost = Canonizers->MirrorOwnerCachedCanonizer->CanonizeHost(host);
        ownerName = Canonizers->ReverseDomainCanonizer->CanonizeHost(moHost);
    }
};
