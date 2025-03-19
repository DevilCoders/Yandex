#pragma once
#include <kernel/common_server/processors/common/handler.h>


class TNotifierPermissions: public TAdministrativePermissions {
private:
    using TBase = TAdministrativePermissions;
    static TFactory::TRegistrator<TNotifierPermissions> Registrator;

public:
    static TString GetTypeName() {
        return "notifiers";
    }

    virtual TString GetClassName() const override {
        return GetTypeName();
    }

    bool Check(const EObjectAction& action) const {
        return GetActions().contains(action);
    }
};


class TNotifiersInfoProcessor: public TCommonSystemHandler<TNotifiersInfoProcessor> {
private:
    using TBase = TCommonSystemHandler<TNotifiersInfoProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "notifiers-info";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};


class TNotifiersUpsertProcessor: public TCommonSystemHandler<TNotifiersUpsertProcessor> {
private:
    using TBase = TCommonSystemHandler<TNotifiersUpsertProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "notifiers-upsert";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};


class TNotifiersRemoveProcessor: public TCommonSystemHandler<TNotifiersRemoveProcessor> {
private:
    using TBase = TCommonSystemHandler<TNotifiersRemoveProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "notifiers-remove";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};
