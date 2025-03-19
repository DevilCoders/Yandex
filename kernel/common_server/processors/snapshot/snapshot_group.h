#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/api/snapshots/group.h>

namespace NCS {
    namespace NSnapshots {

        class TSnapshotsGroupPermissions: public TAdministrativePermissions {
        private:
            using TBase = TAdministrativePermissions;
            static TFactory::TRegistrator<TSnapshotsGroupPermissions> Registrator;

        public:
            static TString GetTypeName() {
                return "snapshots-group";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            bool Check(const EObjectAction& action) const {
                return GetActions().contains(action);
            }
        };


        class TSnapshotGroupsInfoProcessor: public NCS::NHandlers::TCommonInfo<TSnapshotGroupsInfoProcessor, TDBSnapshotsGroup, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TCommonInfo<TSnapshotGroupsInfoProcessor, TDBSnapshotsGroup, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "snapshot-groups-info";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };


        class TSnapshotGroupsUpsertProcessor: public NCS::NHandlers::TCommonUpsert<TSnapshotGroupsUpsertProcessor, TDBSnapshotsGroup, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TCommonUpsert<TSnapshotGroupsUpsertProcessor, TDBSnapshotsGroup, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "snapshot-groups-upsert";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };


        class TSnapshotGroupsRemoveProcessor: public NCS::NHandlers::TCommonRemove<TSnapshotGroupsRemoveProcessor, TDBSnapshotsGroup, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TCommonRemove<TSnapshotGroupsRemoveProcessor, TDBSnapshotsGroup, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "snapshot-groups-remove";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };
    }
}
