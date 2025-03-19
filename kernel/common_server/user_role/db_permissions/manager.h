#pragma once
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/api/links/link.h>
#include <kernel/common_server/api/links/manager.h>
#include <kernel/common_server/user_role/abstract/abstract.h>
#include "object.h"

class TDBPermissionsManager: public IPermissionsManager {
private:
    using TBase = IPermissionsManager;
    using TLinks = TLinksManager<TDBLinkUserRole>;
    THolder<TLinks> Links;
protected:
    virtual bool Restore(const TString& systemUserId, TUserRolesCompiled& result) const override;
    virtual bool RestoreAll(TVector<TUserRolesCompiled>& result) const override;
    virtual bool Upsert(const TUserRolesCompiled& result, const TString& userId, bool* isUpdate) const override;
    virtual bool DoStart() override {
        return TBase::DoStart() && Links->Start();
    }
    virtual bool DoStop() override {
        return Links->Stop() && TBase::DoStop();
    }
public:
    TDBPermissionsManager(const TString& dbName, const THistoryConfig& hConfig, const IPermissionsManagerConfig& config, const IBaseServer& server);
};
