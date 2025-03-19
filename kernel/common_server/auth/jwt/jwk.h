#pragma once

#include <kernel/common_server/util/accessor.h>

namespace NJson {
    class TJsonValue;
}

class TJwk {
private:
    CSA_READONLY_DEF(TString, KId);
    CSA_READONLY_DEF(TString, KTy);
    CSA_READONLY_DEF(TString, N);
    CSA_READONLY_DEF(TString, E);

public:
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& json);
    NJson::TJsonValue SerializeToJson() const;

    TJwk() = default;
};
