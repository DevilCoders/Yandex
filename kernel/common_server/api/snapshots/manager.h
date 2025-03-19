#pragma once
#include "config.h"
#include "group.h"
#include <kernel/common_server/api/history/db_entities.h>

namespace NCS {
    class TSnapshotsController;
    class TSnapshotGroupsManager: public TDBEntitiesManager<TDBSnapshotsGroup> {
    private:
        using TBase = TDBEntitiesManager<TDBSnapshotsGroup>;
        NStorage::IDatabase::TPtr Database;
        const IBaseServer& Server;
    protected:
        virtual TDBSnapshotsGroup PrepareForUsage(const TDBSnapshotsGroup& evHistory) const override {
            TDBSnapshotsGroup result;
            CHECK_WITH_LOG(TBaseDecoder::DeserializeFromTableRecordCommon(result, evHistory.SerializeToTableRecord().BuildWT(), false)) << "cannot parse serialized snapshot group for deep copy" << Endl;
            if (!result.MutableObjectsManager().Start(Server)) {
                TFLEventLog::Error("cannot activate snapshot group");
            }
            return result;
        }
    public:
        TSnapshotGroupsManager(NStorage::IDatabase::TPtr db, const THistoryConfig& config, const IBaseServer& server)
            : TBase(db, config)
            , Server(server)
        {
        }

    };

    class TSnapshotsManager: public TDBEntitiesManager<TDBSnapshot> {
    private:
        using TBase = TDBEntitiesManager<TDBSnapshot>;
        NStorage::IDatabase::TPtr Database;
        class TIndexBySnapshotIdPolicy {
        public:
            using TKey = ui32;
            using TObject = TDBSnapshot;
            static ui32 GetKey(const TDBSnapshot& object) {
                return object.GetSnapshotId();
            }
            static TString GetUniqueId(const TDBSnapshot& object) {
                return object.GetSnapshotCode();
            }
        };
        using TIndexBySnapshotId = TObjectByKeyIndex<TIndexBySnapshotIdPolicy>;
        mutable TIndexBySnapshotId IndexBySnapshotId;
    protected:
        virtual bool DoRebuildCacheUnsafe() const override {
            if (!TBase::DoRebuildCacheUnsafe()) {
                return false;
            }
            IndexBySnapshotId.Initialize(TBase::Objects);
            return true;
        }

        virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBSnapshot>& ev) const override {
            TBase::DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
            IndexBySnapshotId.Remove(ev);
        }

        virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBSnapshot>& ev, TDBSnapshot& object) const override {
            TBase::DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
            IndexBySnapshotId.Upsert(ev);
        }
        virtual bool DoUpsertObject(const TDBSnapshot& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdateExt = nullptr, TDBSnapshot* resultObject = nullptr) const override;
    public:
        using TBase::TBase;

        TMaybe<TDBSnapshot> GetSnapshotById(const ui32 snapshotId) const {
            TReadGuard rg(TBase::MutexCachedObjects);
            auto objects = IndexBySnapshotId.GetObjectsByKey(snapshotId);
            if (objects.size() == 1) {
                return objects.front();
            } else {
                return Nothing();
            }
        }

        bool GetGroupSnapshotCodes(const TString& groupId, TSet<TString>& snapshotCodes) const;
        bool GetGroupSnapshots(const TString& groupId, const TSet<TDBSnapshot::ESnapshotStatus>& statusFilter, TVector<TDBSnapshot>& result) const;
        bool GetSnapshotsByStatus(const TDBSnapshot::ESnapshotStatus status, TVector<TDBSnapshot>& result) const;
        TMaybe<TDBSnapshot> GetActualSnapshotInfo(const TString& groupId) const;
        TMaybe<TDBSnapshot> GetActualSnapshotConstruction(const TString& groupId) const;
        bool UpdateForConstruction(const ui32 snapshotId, const TString& userId, NCS::TEntitySession& session) const;
        bool UpdateForDelete(const TSet<ui32>& objectIds, const TString& userId, NCS::TEntitySession& session) const;
        bool UpdateAsRemoved(const TSet<ui32>& objectIds, const TString& userId, NCS::TEntitySession& session) const;
        bool RemovePermanently(const TSet<ui32>& objectIds, const TString& userId, NCS::TEntitySession& session) const;
        bool RestoreByIds(const TSet<ui32>& objectIds, TVector<TDBSnapshot>& objects, NCS::TEntitySession& session) const;

        bool StartSnapshotConstruction(const TDBSnapshot& newSnapshot, ui32& snapshotId, const TString& userId, const ISnapshotsController& controller) const;
        bool FinishSnapshotConstruction(const TString& snapshotCode, const TString& userId) const;
        bool UpdateContext(const TString& snapshotCode, const TSnapshotContentFetcherContext& newContext, const TString& userId) const;
    };
}
