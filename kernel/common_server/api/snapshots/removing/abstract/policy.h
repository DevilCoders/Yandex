#pragma once
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    class ISnapshotsController;

    class ISnapshotsGroupRemovePolicy {
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const = 0;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const = 0;
    public:
        using TPtr = TAtomicSharedPtr<ISnapshotsGroupRemovePolicy>;
        using TFactory = NObjectFactory::TObjectFactory<ISnapshotsGroupRemovePolicy, TString>;
        virtual ~ISnapshotsGroupRemovePolicy() = default;
        virtual bool MarkToRemove(const TString& groupId, const TString& userId, const ISnapshotsController& server) const = 0;
        NJson::TJsonValue SerializeToJson() const {
            return DoSerializeToJson();
        }
        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            return DoDeserializeFromJson(jsonInfo);
        }
        virtual TString GetClassName() const = 0;
        NFrontend::TScheme GetScheme(const IBaseServer& server) const {
            return DoGetScheme(server);
        }
    };

    class TSnapshotsGroupRemovePolicyContainer: public TBaseInterfaceContainer<ISnapshotsGroupRemovePolicy> {
    private:
        using TBase = TBaseInterfaceContainer<ISnapshotsGroupRemovePolicy>;
    public:
        using TBase::TBase;

    };

}
