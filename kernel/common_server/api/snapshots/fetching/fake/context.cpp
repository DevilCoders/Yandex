#include "context.h"

namespace NCS {
    TFakeSnapshotFetcherContext::TFactory::TRegistrator<TFakeSnapshotFetcherContext> TFakeSnapshotFetcherContext::Registrator(TFakeSnapshotFetcherContext::GetTypeName());

    NJson::TJsonValue TFakeSnapshotFetcherContext::DoSerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        result.InsertValue("ready_objects", ReadyObjects);
        return result;
    }

    bool TFakeSnapshotFetcherContext::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        JREAD_INT(jsonInfo, "ready_objects", ReadyObjects);
        return true;
    }

}
