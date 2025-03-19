#include "jwk.h"

#include <kernel/common_server/util/json_processing.h>

bool TJwk::DeserializeFromJson(const NJson::TJsonValue& json) {
    JREAD_STRING(json, "kid", KId);
    JREAD_STRING(json, "kty", KTy);
    JREAD_STRING(json, "n", N);
    JREAD_STRING(json, "e", E);
    return true;
}

NJson::TJsonValue TJwk::SerializeToJson() const {
    NJson::TJsonValue result;
    JWRITE(result, "kid", KId);
    JWRITE(result, "kty", KTy);
    JWRITE(result, "n", N);
    JWRITE(result, "e", E);
    return result;
}