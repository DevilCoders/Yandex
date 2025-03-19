#pragma once
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/api/snapshots/objects/mapped/object.h>
#include <kernel/common_server/api/snapshots/object.h>

namespace NCS {
    class ISnapshotsController;

    class ISnapshotObjectsCleaningPolicy {
    private:
        CS_ACCESS(ISnapshotObjectsCleaningPolicy, TSet<NCS::TDBSnapshot::ESnapshotStatus>, AllowedStatusesForCleaning, {NCS::TDBSnapshot::ESnapshotStatus::Ready});

    protected:
        virtual bool PrepareCleaningCondition(TSRCondition& cleaningCondition) const = 0;

    public:
        using TPtr = TAtomicSharedPtr<ISnapshotObjectsCleaningPolicy>;
        using TFactory = NObjectFactory::TObjectFactory<ISnapshotObjectsCleaningPolicy, TString>;
        virtual ~ISnapshotObjectsCleaningPolicy() = default;
        bool CleanSnapshotObjects(const TString& groupId, const ISnapshotsController& controller) const;
        virtual NJson::TJsonValue SerializeToJson() const;
        virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
        virtual TString GetClassName() const = 0;
        virtual NFrontend::TScheme GetScheme(const IBaseServer& /*server*/) const;
    };

    class TSnapshotsGroupCleaningPolicyContainer: public TBaseInterfaceContainer<ISnapshotObjectsCleaningPolicy> {
    private:
        using TBase = TBaseInterfaceContainer<ISnapshotObjectsCleaningPolicy>;

    public:
        using TBase::TBase;
    };

}
