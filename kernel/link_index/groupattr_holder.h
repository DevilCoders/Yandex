#pragma once

#include <kernel/indexer/attrproc/attributer.h>
#include <library/cpp/deprecated/calc_module/simple_module.h>

class TGroupAttributerHolder : public TSimpleModule {
private:
    const TString ConfigPath;
    const TExistenceChecker ExistenceChecker;
    const TOwnerCanonizer* OwnerCanonizerPtr;

    THolder<const TGroupAttributer> GroupAttributer;
    const TGroupAttributer* Attributer;

public:
    TGroupAttributerHolder(const TAttrProcessorConfig& cfg)
        : TSimpleModule("TGroupAttributerHolder")
        , ConfigPath(cfg.AttributerDir)
        , ExistenceChecker(cfg.ExistenceChecker)
        , OwnerCanonizerPtr(nullptr)
        , Attributer(nullptr)
    {
        Bind(this).To<&TGroupAttributerHolder::Init>("init");
        Bind(this).To<const TGroupAttributer*const*>(&Attributer, "group_attributer_output");
        Bind(this).To<const TString&, TString&, &TGroupAttributerHolder::GetHostOwner>("get_host_owner");
    }

private:
    void Init() {
        GroupAttributer.Reset(Attributer = new TGroupAttributer(ConfigPath, ExistenceChecker));
        OwnerCanonizerPtr = &Attributer->GetInternalOwnerCanonizer();
    }
    void GetHostOwner(const TString& host, TString& owner) {
        owner = OwnerCanonizerPtr->GetHostOwner(host.data());
    }
};
