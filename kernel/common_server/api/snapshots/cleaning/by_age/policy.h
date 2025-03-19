#pragma once
#include <kernel/common_server/api/snapshots/cleaning/abstract/policy.h>

namespace NCS {
    class TByAgeCleaningPolicy: public ISnapshotObjectsCleaningPolicy {
    private:
        using TBase = ISnapshotObjectsCleaningPolicy;
        CSA_DEFAULT(TByAgeCleaningPolicy, TString, TimestampFieldName);
        CSA_DEFAULT(TByAgeCleaningPolicy, ui32, AllowedAgeSeconds);
        static TFactory::TRegistrator<TByAgeCleaningPolicy> Registrator;

    protected:
        virtual NJson::TJsonValue SerializeToJson() const override;
        virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        virtual NFrontend::TScheme GetScheme(const IBaseServer& /*server*/) const override;
        virtual bool PrepareCleaningCondition(TSRCondition& cleaningCondition) const override;

    public:
        static TString GetTypeName() {
            return "by_age";
        }
        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };

}
