#pragma once
#include "abstract.h"
#include <util/generic/vector.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/library/scheme/scheme.h>
#include <kernel/common_server/library/scheme/fields.h>
#include <kernel/common_server/library/storage/records/abstract.h>
#include <kernel/common_server/library/storage/records/db_value.h>

namespace NCS {
    namespace NStorage {

        class TTableRecord;
        class TTableRecordWT;

        class TColumnInfo {//: public INativeProtoSerialization<NCSProto::TColumnInfo> {
        private:
            CSA_DEFAULT(TColumnInfo, TString, Id);
            CSA_FLAG(TColumnInfo, Nullable, true);
            CSA_FLAG(TColumnInfo, Primary, false);
            CSA_FLAG(TColumnInfo, Unique, false);
            CSA_FLAG(TColumnInfo, Autogeneration, false);
            CS_ACCESS(TColumnInfo, EColumnType, Type, EColumnType::Text);
        public:
            bool NeedQuote() const;

            bool IsCorrectValue(const TString& value) const;

            bool WriteValue(TTableRecord& tr, const TString& value) const;
            bool WriteValue(TTableRecordWT& tr, const TString& value) const;
            TMaybe<TDBValue> GetDBValue(const TString& value) const;

            static NCS::NScheme::TScheme GetScheme();

            TColumnInfo() = default;

            TColumnInfo(const TString& id)
                : Id(id)
            {

            }
            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            NJson::TJsonValue SerializeToJson() const;

/*
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromProto(const NCSProto::TColumnInfo& proto) override {
                Id = proto.GetId();
                if (!Id) {
                    TFLEventLog::Error("no column name info");
                    return false;
                }
                if (proto.HasNullable()) {
                    NullableFlag = proto.GetNullable();
                }
                if (proto.HasPrimary()) {
                    PrimaryFlag = proto.GetPrimary();
                }
                if (proto.HasUnique()) {
                    UniqueFlag = proto.GetUnique();
                }
                if (!proto.HasType()) {
                    TFLEventLog::Error("no column type info");
                    return false;
                }
                if (!TEnumWorker<EColumnType>::TryParseFromInt(proto.GetType())) {
                    TFLEventLog::Error("incorrect column type info")("type", proto.GetType());
                    return false;
                }
                return true;
            }
            virtual void SerializeToProto(NCSProto::TColumnInfo& proto) const override {
                proto.SetId(Id);
                proto.SetNullable(IsNullable());
                proto.SetPrimary(IsPrimary());
                proto.SetUnique(IsUnique());
                proto.SetType((ui32)Type);
            }
            */
        };

        class TCreateTableQuery: public IQuery {
        private:
            CSA_DEFAULT(TCreateTableQuery, TString, TableName);
            CSA_READONLY_DEF(TVector<TColumnInfo>, Columns);
        public:
            TCreateTableQuery() = default;
            TCreateTableQuery(const TString& tableName, const TVector<TColumnInfo>& columns)
                : TableName(tableName)
                , Columns(columns) {

            }
            TCreateTableQuery(const TString& tableName)
                : TableName(tableName) {

            }
            TColumnInfo& AddColumn(const TString& columnId);
        };

        class TAddColumnQuery: public IQuery {
        private:
            CSA_DEFAULT(TAddColumnQuery, TString, TableName);
            CSA_DEFAULT(TAddColumnQuery, TColumnInfo, Column);
        public:
            TAddColumnQuery() = default;
            TAddColumnQuery(const TString& tableName, const TColumnInfo& column)
                : TableName(tableName)
                , Column(column) {

            }
        };

        class TCreateIndexQuery: public IQuery {
        private:
            CSA_READONLY_DEF(TString, TableName);
            CSA_FLAG(TCreateIndexQuery, Unique, false);
            CSA_READONLY_DEF(TVector<TString>, Columns);
        public:
            TCreateIndexQuery(const TString& tableName)
                : TableName(tableName)
            {

            }
            TCreateIndexQuery& AddColumn(const TString& columnId) {
                Columns.emplace_back(columnId);
                return *this;
            }
        };
    }
}
