#include "database.h"

namespace NDBTableScanner {

    void TCommonScannerPolicy::FillScheme(NFrontend::TScheme& scheme, const IBaseServer& server) {
        scheme.Add<TFSVariants>("db_name", "База данных").SetVariants(server.GetDatabaseNames());
        scheme.Add<TFSString>("table_name", "Имя таблицы в базе");
        scheme.Add<TFSString>("event_id_column", "Название колонки с event_id").SetDefault("history_event_id");
    }

    void TCommonScannerPolicy::SerializeToJson(NJson::TJsonValue& result) const {
        result.InsertValue("table_name", TableName);
        result.InsertValue("db_name", DBName);
        result.InsertValue("event_id_column", EventIdColumn);
    }

    bool TCommonScannerPolicy::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        JREAD_STRING_OPT(jsonInfo, "event_id_column", EventIdColumn);
        JREAD_STRING(jsonInfo, "table_name", TableName);
        JREAD_STRING_OPT(jsonInfo, "db_name", DBName);
        return true;
    }

}
