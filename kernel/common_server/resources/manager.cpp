#include "manager.h"

namespace NCS {
    namespace NResources {

        bool TDBManager::RestoreByResourceKey(const TString& resourceKey, TMaybe<TDBResource>& result, NCS::TEntitySession& session) const {
            NCS::NStorage::TObjectRecordsSet<TDBResource> records;
            TSRSelect reqSelect(TDBResource::GetTableName(), &records);
            reqSelect.InitCondition<TSRBinary>(TDBResource::GetIdFieldName(), resourceKey);
            if (!session.ExecRequest(reqSelect)) {
                return false;
            }
            if (records.size() == 1) {
                result = std::move(records.DetachObjects().front());
            } else if (records.size() == 0) {
                result = Nothing();
            } else {
                session.Error("incorrect records count for id key selection")("key", resourceKey);
                return false;
            }
            return true;
        }

        bool TDBManager::RemoveObjects(const TSet<TString>& keys, NCS::TEntitySession& session) const {
            TSRDelete reqDelete(TDBResource::GetTableName());
            reqDelete.InitCondition<TSRBinary>(TDBResource::GetIdFieldName(), keys);
            if (!session.ExecRequest(reqDelete)) {
                return false;
            }
            return true;
        }

        bool TDBManager::UpsertObjects(const TVector<TDBResource>& objects, NCS::TEntitySession& session) const {
            TSRInsert reqInsert(TDBResource::GetTableName());
            reqInsert.FillRecords(objects);
            reqInsert.SetConflictPolicy(TSRInsert::EConflictPolicy::Update).SetUniqueFieldId(TDBResource::GetIdFieldName());
            if (!session.ExecRequest(reqInsert)) {
                return false;
            }
            return true;
        }

        NCS::TEntitySession TDBManager::BuildNativeSession(const bool readOnly) const {
            return NCS::TEntitySession(Database->TransactionMaker().ReadOnly(readOnly).Build());
        }

        bool TDBManager::RestoreByAccessId(const TString& accessId, TVector<NCS::NResources::TDBResource>& result, NCS::TEntitySession& session) const {
            NCS::NStorage::TObjectRecordsSet<TDBResource> records;
            TSRSelect reqSelect(TDBResource::GetTableName(), &records);
            reqSelect.InitCondition<TSRBinary>("access_id", accessId);
            if (!session.ExecRequest(reqSelect)) {
                return false;
            }
            result = std::move(records.DetachObjects());
            return true;
        }

        bool TDBManager::RestoreKeysByAccessId(const TString& accessId, TSet<TString>& result, NCS::TEntitySession& session) const {
            TVector<TDBResource> resultObjects;
            if (!RestoreByAccessId(accessId, resultObjects, session)) {
                return false;
            }
            TSet<TString> localResult;
            for (auto&& i : resultObjects) {
                localResult.emplace(i.GetKey());
            }
            std::swap(localResult, result);
            return true;
        }

    }
}
