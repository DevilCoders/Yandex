#include "context.h"

namespace NCS {
    TYTSnapshotFetcherContext::TFactory::TRegistrator<TYTSnapshotFetcherContext> TYTSnapshotFetcherContext::Registrator(TYTSnapshotFetcherContext::GetTypeName());

    NJson::TJsonValue TYTSnapshotFetcherContext::DoSerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        result.InsertValue("start_table_index", StartTableIndex);
        result.InsertValue("errors_count", ErrorsCount);
        result.InsertValue("snapshot_size", SnapshotSize);

        return result;
    }

    bool TYTSnapshotFetcherContext::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        JREAD_INT(jsonInfo, "start_table_index", StartTableIndex);
        JREAD_INT(jsonInfo, "errors_count", ErrorsCount);
        JREAD_INT(jsonInfo, "snapshot_size", SnapshotSize);
        return true;
    }

}
