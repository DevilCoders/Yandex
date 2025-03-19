#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/api/snapshots/object.h>

namespace NCS {
    namespace NSnapshots {

        enum class ESnapshotAction: ui64 {
            Add = 1 /* "add" */,
            Modify = 1 << 1 /* "modify" */,
            Observe = 1 << 2 /* "observe" */,
            Remove = 1 << 3 /* "remove" */,
            RemovePermanently = 1 << 4 /* "remove_permanently" */,
            FillRebuild = 1 << 5 /* "fill" */,
            FillUpsert = 1 << 6 /* "fill_upsert" */

        };

        class TSnapshotPermissions: public TAdministrativePermissionsImpl<ESnapshotAction, ui64> {
        private:
            using TBase = TAdministrativePermissionsImpl<ESnapshotAction, ui64>;
            static TFactory::TRegistrator<TSnapshotPermissions> Registrator;

        public:
            static TString GetTypeName() {
                return "snapshot";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TSnapshotsInfoProcessor: public NCS::NHandlers::TCommonInfo<TSnapshotsInfoProcessor, TDBSnapshot, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TCommonInfo<TSnapshotsInfoProcessor, TDBSnapshot, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "snapshots-info";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

        class TSnapshotsUpsertProcessor: public NCS::NHandlers::TCommonUpsert<TSnapshotsUpsertProcessor, TDBSnapshot, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TCommonUpsert<TSnapshotsUpsertProcessor, TDBSnapshot, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "snapshots-upsert";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

        class TSnapshotFillProcessor: public NCS::NHandlers::TCommonImpl<TSnapshotFillProcessor, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TCommonImpl<TSnapshotFillProcessor, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual bool DoFillHandlerScheme(NCS::NScheme::THandlerScheme& scheme, const IBaseServer& /*server*/) const override {
                auto& rMethod = scheme.Method(NCS::NScheme::ERequestMethod::Post);
                rMethod.MutableParameters().AddQuery<TFSString>("snapshot_code");
                rMethod.MutableParameters().AddQuery<TFSString>("snapshot_group");
                rMethod.MutableParameters().AddQuery<TFSString>("delimiter");
                rMethod.MutableParameters().AddQuery<TFSBoolean>("upsert_diff");
                rMethod.Body().Content("text/plain");
                rMethod.Response(HTTP_OK);
                return true;
            }
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "snapshots-fill";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

        class TSnapshotsRemoveProcessor: public NCS::NHandlers::TCommonRemove<TSnapshotsRemoveProcessor, TDBSnapshot, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = NCS::NHandlers::TCommonRemove<TSnapshotsRemoveProcessor, TDBSnapshot, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "snapshots-remove";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };
    }
}
