#pragma once

#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/rt_background/settings.h>

class TRTBackgroundPermissions: public TAdministrativePermissions {
private:
    using TBase = TAdministrativePermissions;
    static TFactory::TRegistrator<TRTBackgroundPermissions> Registrator;
    CSA_READONLY_DEF(TSet<TString>, AvailableOwners);
    CSA_READONLY_DEF(TSet<TString>, AvailableClasses);
    CSA_READONLY_DEF(TSet<TString>, AvailableProcesses);
public:
    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        TJsonProcessor::WriteContainerArray(result, "available_classes", AvailableClasses);
        TJsonProcessor::WriteContainerArray(result, "available_processes", AvailableProcesses);
        TJsonProcessor::WriteContainerArray(result, "available_owners", AvailableOwners);
        return result;
    }

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
        if (!TBase::DeserializeFromJson(info)) {
            return false;
        }
        if (!TJsonProcessor::ReadContainer(info, "available_classes", AvailableClasses)) {
            return false;
        }
        if (!TJsonProcessor::ReadContainer(info, "available_processes", AvailableClasses)) {
            return false;
        }
        if (!TJsonProcessor::ReadContainer(info, "available_owners", AvailableClasses)) {
            return false;
        }
        return true;
    }

    static TString GetTypeName() {
        return "rt_background";
    }

    virtual TString GetClassName() const override {
        return GetTypeName();
    }

    bool Check(const EObjectAction action) const {
        return GetActions().contains(action);
    }

    bool Check(const EObjectAction action, const TRTBackgroundProcessContainer& bgObject) const {
        if (!bgObject) {
            return false;
        }
        if (!GetActions().contains(action)) {
            return false;
        }
        if (!bgObject.CheckOwner(bgObject->GetOwners())) {
            return false;
        }
        if (AvailableClasses.size() && !AvailableClasses.contains(bgObject->GetType()) && !AvailableClasses.contains("*")) {
            return false;
        }
        if (AvailableProcesses.size() && !AvailableProcesses.contains(bgObject->GetType()) && !AvailableProcesses.contains("*")) {
            return false;
        }
        return true;
    }
};

class TRTBackgroundListProcessor : public TCommonSystemHandler<TRTBackgroundListProcessor> {
    using TBase = TCommonSystemHandler<TRTBackgroundListProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "rt_background-list";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};

class TRTBackgroundUpsertProcessor: public TCommonSystemHandler<TRTBackgroundUpsertProcessor> {
    using TBase = TCommonSystemHandler<TRTBackgroundUpsertProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "rt_background-upsert";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};

class TRTBackgroundSwitchingProcessor: public TCommonSystemHandler<TRTBackgroundSwitchingProcessor> {
    using TBase = TCommonSystemHandler<TRTBackgroundSwitchingProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "rt_background-switching";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};

class TRTBackgroundRemoveProcessor : public TCommonSystemHandler<TRTBackgroundRemoveProcessor> {
    using TBase = TCommonSystemHandler<TRTBackgroundRemoveProcessor>;
public:
    using TBase::TBase;

    static TString GetTypeName() {
        return "rt_background-remove";
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
};
