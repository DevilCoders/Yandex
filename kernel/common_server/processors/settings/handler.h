#pragma once

#include <kernel/common_server/processors/common/handler.h>


class TSettingsPermissions: public TAdministrativePermissions {
private:
    using TBase = TAdministrativePermissions;
    static TFactory::TRegistrator<TSettingsPermissions> Registrator;

public:
    static TString GetTypeName() {
        return "settings";
    }

    virtual TString GetClassName() const override {
        return GetTypeName();
    }

    bool Check(const EObjectAction& action) const {
        return GetActions().contains(action);
    }
};


class TSettingsHistoryProcessor: public TCommonSystemHandler<TSettingsHistoryProcessor> {
private:
    using TBase = TCommonSystemHandler<TSettingsHistoryProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "settings_history";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};


class TSettingsInfoProcessor: public TCommonSystemHandler<TSettingsInfoProcessor> {
private:
    using TBase = TCommonSystemHandler<TSettingsInfoProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "settings_info";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};


class TSettingsUpsertProcessor: public TCommonSystemHandler<TSettingsUpsertProcessor> {
private:
    using TBase = TCommonSystemHandler<TSettingsUpsertProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "settings_upsert";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};


class TSettingsRemoveProcessor: public TCommonSystemHandler<TSettingsRemoveProcessor> {
private:
    using TBase = TCommonSystemHandler<TSettingsRemoveProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "settings_remove";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};
