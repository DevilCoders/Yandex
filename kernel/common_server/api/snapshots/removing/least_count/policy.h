#pragma once
#include <kernel/common_server/api/snapshots/removing/abstract/policy.h>

namespace NCS {
    class TLeastCountRemovePolicy: public ISnapshotsGroupRemovePolicy {
    private:
        CS_ACCESS(TLeastCountRemovePolicy, ui32, Count, 3);
        CS_ACCESS(TLeastCountRemovePolicy, ui32, RemovingCountLimit, 1);
        CS_ACCESS(TLeastCountRemovePolicy, bool, TotallyRemoving, true);
        static TFactory::TRegistrator<TLeastCountRemovePolicy> Registrator;
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const override;
    public:
        virtual bool MarkToRemove(const TString& groupId, const TString& userId, const ISnapshotsController& controller) const override;
        static TString GetTypeName() {
            return "least_count";
        }
        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };

}
