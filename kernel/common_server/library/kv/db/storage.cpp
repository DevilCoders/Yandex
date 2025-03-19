#include "storage.h"
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/storage/records/set.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/library/storage/records/record.h>
#include <library/cpp/digest/md5/md5.h>

namespace NCS {
    namespace NKVStorage {
        bool TDBStorage::DoPutData(const TString& path, const TBlob& data, const TWriteOptions& options) const {
            const TString table = options.GetVisibilityScope() ? options.GetVisibilityScope() : DefaultTableName;
            TSRInsert srInsert(table);
            NCS::NStorage::TTableRecord trInput;
            trInput.Set("id", path);
            trInput.SetBytes("data", TStringBuf(data.AsCharPtr(), data.size()));
            trInput.Set("timestamp", options.GetTimestamp().Seconds());
            trInput.Set("hash", !!options.GetDataHash() ? options.GetDataHash() : MD5::Calc(data));
            trInput.Set("user_id", options.GetUserId());
            trInput.Set("meta_data", options.GetMetaData());
            srInsert.AddRecord(trInput);
            auto tr = Database->CreateTransaction(false);
            auto result = tr->ExecRequest(srInsert);
            if (!result || !result->IsSucceed()) {
                TFLEventLog::Error("cannot add data")("path", path)("data_size", data.size());
                return false;
            }
            if (!tr->Commit()) {
                TFLEventLog::Error("cannot commit transaction")("path", path)("data_size", data.size());
                return false;
            }
            return true;
        }

        bool TDBStorage::DoGetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options) const {
            const TString table = options.GetVisibilityScope() ? options.GetVisibilityScope() : DefaultTableName;
            NCS::NStorage::TRecordsSetWT records;
            TSRSelect srSelect(table, &records);
            srSelect.InitCondition<TSRBinary>("id", path);
            auto tr = Database->CreateTransaction(true);
            auto result = tr->ExecRequest(srSelect);
            if (!result || !result->IsSucceed()) {
                return false;
            }
            if (records.size() == 0) {
                data = Nothing();
                return true;
            } else if (records.size() == 1) {
                TMaybe<TString> dataLocal = records.front().GetBytes("data");
                if (!dataLocal) {
                    data = Nothing();
                } else {
                    data = TBlob::FromString(*dataLocal);
                }
                return true;
            } else {
                TFLEventLog::Error("inconsistency unique index")("incorrect_size", records.size());
                return false;
            }
        }

        TString TDBStorage::DoGetPathByHash(const TBlob& data, const TReadOptions& options) const {
            const TString table = options.GetVisibilityScope() ? options.GetVisibilityScope() : DefaultTableName;
            NCS::NStorage::TRecordsSetWT records;
            TSRSelect srSelect(table, &records);
            srSelect.InitCondition<TSRBinary>("hash", !!options.GetDataHash() ? options.GetDataHash() : MD5::Calc(data));
            auto tr = Database->CreateTransaction(true);
            auto result = tr->ExecRequest(srSelect);
            if (!result || !result->IsSucceed()) {
                return "";
            }
            if (records.size() == 0) {
                return "";
            } else if (records.size() == 1) {
                return records.front().GetString("id");
            } else {
                const auto strBuf = data.AsStringBuf();
                for (auto&& i: records) {
                 TMaybe<TString> dataLocal = i.GetBytes("data");
                    if (!!dataLocal && *dataLocal == strBuf) {
                        return i.GetString("id");
                    }
                }
                return "";
            }
        }

        bool TDBStorage::DoPrepareEnvironment(const TBaseOptions& options) const {
            const TString tableName = options.GetVisibilityScope() ? options.GetVisibilityScope() : DefaultTableName;
            {
                TReadGuard g(MutexExistedTables);
                if (ExistedTables->contains(tableName)) {
                    return true;
                }
            }

            TWriteGuard g(MutexExistedTables);
            if (ExistedTables->contains(tableName)) {
                return true;
            }
            NCS::NStorage::TCreateTableQuery query;
            query.SetTableName(tableName);
            query.AddColumn("id").SetNullable(false).SetType(NCS::NStorage::EColumnType::Text);
            query.AddColumn("data").SetNullable(false).SetType(NCS::NStorage::EColumnType::Binary);
            query.AddColumn("timestamp").SetNullable(false).SetType(NCS::NStorage::EColumnType::I32);
            query.AddColumn("hash").SetNullable(false).SetType(NCS::NStorage::EColumnType::Text);
            query.AddColumn("user_id").SetNullable(false).SetType(NCS::NStorage::EColumnType::Text);
            query.AddColumn("meta_data").SetNullable(false).SetType(NCS::NStorage::EColumnType::Text);
            if (!Database->CreateTable(query)) {
                TFLEventLog::Error("cannot create table")("name", tableName);
                return false;
            }
            ExistedTables->emplace(tableName);
            return true;
        }
    }
}
