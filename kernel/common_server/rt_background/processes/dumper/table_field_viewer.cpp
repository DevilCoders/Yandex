#include "table_field_viewer.h"

namespace NCS {

    ITableFieldViewer::ITableFieldViewer(const IBaseServer& server)
        : Server(server)
    {
    }

    TDBTableFieldViewer::TDBTableFieldViewer(const TString& tableName, const TString& dBName, const TString& dBType,
                                             bool dbDefineSchema, const IBaseServer& server)
        : ITableFieldViewer(server)
        , TableName(tableName)
        , DBName(dBName)
        , DBType(dBType)
        , DBDefineSchema(dbDefineSchema)
    {
    }

    IDumperMetaParser::TPtr TDBTableFieldViewer::Construct(const TString& key) const {
        auto metaParser = IDumperMetaParser::TFactory::Construct(TableName + "." + key);
        if (!metaParser) {
            metaParser = IDumperMetaParser::TFactory::Construct(key);
        }
        return metaParser;
    }

    NYT::TTableSchema TDBTableFieldViewer::GetYtSchema() const {
        if (DBDefineSchema) {
            if (DBType == "tags_history") {
                TSchema schema = {
                    {"data", NYT::VT_STRING},
                    {"history_action", NYT::VT_STRING},
                    {"history_comment", NYT::VT_STRING},
                    {"history_event_id", NYT::VT_UINT64},
                    {"history_originator_id", NYT::VT_STRING},
                    {"history_timestamp", NYT::VT_UINT64},
                    {"history_user_id", NYT::VT_STRING},
                    {"object_id", NYT::VT_STRING},
                    {"performer", NYT::VT_STRING},
                    {"priority", NYT::VT_UINT64},
                    {"snapshot", NYT::VT_STRING},
                    {"tag", NYT::VT_STRING},
                    {"tag_id", NYT::VT_STRING},
                    {"unpacked_data", NYT::VT_ANY},
                    {"unpacked_snapshot", NYT::VT_ANY},
                };
                return TYtProcessTraits::GetYtSchema(schema);
            } else {
                return TYtProcessTraits::GetYtSchema(GetNativeYTSchema());
            }
        }
        return {};
    }

    TSchema TDBTableFieldViewer::GetNativeYTSchema() const {
        NStorage::IDatabase::TPtr db = Server.GetDatabase(DBName);
        if (!db) {
            return TSchema();
        }

        NCS::NStorage::TRecordsSetWT records;

        auto transaction = db->CreateTransaction(true);
        auto queryResult = transaction->Exec("select column_name, data_type from INFORMATION_SCHEMA.COLUMNS where TABLE_NAME='" + TableName + "';", &records);
        if (!queryResult || !queryResult->IsSucceed()) {
            TFLEventLog::Alert("Failed to get table schema")("table_name", TableName);
            return TSchema();
        }

        TSchema result = {
            {"unpacked_data", NYT::VT_ANY},
            {"unpacked_snapshot", NYT::VT_ANY},
        };
        for (auto&& record : records) {
            const auto column = record.CastTo<TString>("column_name").GetOrElse("");
            const auto strType = record.CastTo<TString>("data_type").GetOrElse("");
            if (strType == "text" || strType.StartsWith("varchar") || strType == "uuid") {
                result.emplace(column, NYT::VT_STRING);
                continue;
            }
            if (strType == "integer" || strType == "bigint") {
                result.emplace(column, NYT::VT_INT64);
                continue;
            }
            if (strType == "real") {
                result.emplace(column, NYT::VT_DOUBLE);
                continue;
            }
            if (strType == "boolean" || strType == "bool") {
                result.emplace(column, NYT::VT_BOOLEAN);
                continue;
            }
            result.emplace(column, NYT::VT_ANY);
        }

        return result;
    }

} // namespace NCS
