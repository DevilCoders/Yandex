#include "table.h"
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/algorithm/container.h>

namespace NCS {
    namespace NStorage {
        NStorage::ITableAccessor::EUpdateWithRevisionResult ITableAccessor::UpsertWithRevision(const TTableRecord& rowUpdateExt, const TSet<TString>& uniqueFieldsSet,
            const TMaybe<ui32> currentRevision, const TString& revFieldName, ITransaction::TPtr transaction,
            IBaseRecordsSet* recordsSet /*= nullptr*/, const bool force /*= false*/) {
            TTableRecord rowCondition = rowUpdateExt.FilterColumnsRet(uniqueFieldsSet);
            TTableRecord rowUpdate = rowUpdateExt;
            rowUpdate.ForceSet(revFieldName, "nextval('" + GetTableName() + "_" + revFieldName + "_seq')");
            rowCondition.Remove(revFieldName);
            NCS::NStorage::TTransactionQueryResult result = NCS::NStorage::TTransactionQueryResult::BuildEmpty();
            if (rowCondition.size()) {
                if (!force && currentRevision) {
                    rowCondition.Set(revFieldName, *currentRevision);
                }
                result = UpdateRow(rowCondition, rowUpdate, transaction, recordsSet);
                if (!result || !result->IsSucceed()) {
                    return EUpdateWithRevisionResult::Failed;
                } else if (result->GetAffectedRows()) {
                    return EUpdateWithRevisionResult::Updated;
                }
                rowCondition.Remove(revFieldName);
                result = AddRow(rowUpdate, transaction, "NOT EXISTS (SELECT * FROM " + GetTableName() + " WHERE " + rowCondition.BuildCondition(*transaction) + ")", recordsSet);
            } else {
                result = AddRow(rowUpdate, transaction, "", recordsSet);
            }
            if (!result || !result->IsSucceed()) {
                return EUpdateWithRevisionResult::Failed;
            }
            if (result->GetAffectedRows() != 1) {
                return EUpdateWithRevisionResult::IncorrectRevision;
            }
            return EUpdateWithRevisionResult::Inserted;
        }

        NCS::NStorage::TTransactionQueryResult ITableAccessor::DoUpdateRows(const TVector<TTableRecord>& conditions, const TVector<TTableRecord>& updates, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields) {
            ui32 affectedRows = 0;
            for (ui32 i = 0; i < conditions.size(); ++i) {
                auto resultLocal = UpdateRow(conditions[i], updates[i], transaction, recordsSet, returnFields);
                if (!resultLocal) {
                    return MakeAtomicShared<TQueryResult>(false, 0u);
                }
                affectedRows += resultLocal.GetAffectedRows();
            }
            return MakeAtomicShared<TQueryResult>(true, affectedRows);
        }

        bool ITableAccessor::ValidateRecordSets(const TVector<TTableRecord>& records) const {
            if (records.empty()) {
                return true;
            }
            if (records.front().Empty()) {
                TFLEventLog::Error("no columns in records");
                return false;
            }
            for (auto&& i : records) {
                if (!records.front().IsSameFields(i)) {
                    TFLEventLog::Error("different fields set")("expected", records.front().GetKeys())("real", i.GetKeys());
                    return false;
                }
            }
            return true;
        }

        NCS::NStorage::TTransactionQueryResult ITableAccessor::UpdateRows(const TVector<TTableRecord>& conditions, const TVector<TTableRecord>& updates, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet /*= nullptr*/, const TSet<TString>* returnFields) {
            if (conditions.size() == 0) {
                return MakeAtomicShared<TQueryResult>(true, 0u);
            }
            if (conditions.size() != updates.size()) {
                TFLEventLog::Log("inconsistency updates/conditions records");
                return MakeAtomicShared<TQueryResult>(false, 0u);
            }
            if (!ValidateRecordSets(conditions)) {
                TFLEventLog::Log("inconsistency conditions");
                return MakeAtomicShared<TQueryResult>(false, 0u);
            }
            if (!ValidateRecordSets(updates)) {
                TFLEventLog::Log("inconsistency updates");
                return MakeAtomicShared<TQueryResult>(false, 0u);
            }
            return DoUpdateRows(conditions, updates, transaction, recordsSet, returnFields);
        }

        NCS::NStorage::TTransactionQueryResult ITableAccessor::UpsertRows(const TSet<TString>& uniqueFieldIds, const TVector<TTableRecord>& objects, ITransaction::TPtr transaction) {
            if (uniqueFieldIds.empty()) {
                TFLEventLog::Error("UpsertRows no unique field ids");
                return MakeAtomicShared<TQueryResult>(false, 0u);
            }
            if (objects.empty()) {
                return MakeAtomicShared<TQueryResult>(true, 0u);
            }
            TVector<TTableRecord> conditions;
            for (auto&& i : objects) {
                conditions.emplace_back(i.FilterColumnsRet(uniqueFieldIds));
                if (conditions.back().size() != uniqueFieldIds.size()) {
                    TFLEventLog::Error("inconsistency conditional field with data")("field_ids", uniqueFieldIds)("data", conditions.back().SerializeToString());
                    return MakeAtomicShared<TQueryResult>(false, 0u);
                }
            }
            TRecordsSetWT updatedRecords;
            auto updateResult = UpdateRows(conditions, objects, transaction, &updatedRecords, &uniqueFieldIds);
            if (!updateResult->IsSucceed()) {
                return updateResult;
            }
            TSet<TString> updatedRecordsSet;
            for (auto&& i : updatedRecords) {
                updatedRecordsSet.emplace(i.BuildTableRecord().GetValues(*transaction, &uniqueFieldIds, true));
            }

            TRecordsSet insertRecords;
            for (ui32 i = 0; i < conditions.size(); ++i) {
                const TString values = conditions[i].GetValues(*transaction, &uniqueFieldIds, true);
                if (updatedRecordsSet.contains(values)) {
                    continue;
                }
                insertRecords.AddRow(objects[i]);
            }

            auto addResult = AddRows(insertRecords, transaction);
            if (!updateResult->IsSucceed()) {
                return addResult;
            }
            return MakeAtomicShared<TQueryResult>(true, addResult->GetAffectedRows() + updateResult->GetAffectedRows());
        }

    }
}
