#include "json.h"

#include <util/generic/yexception.h>


namespace NAntiRobot {


void TKeyChecker::Check(const NJson::TJsonValue& value) const {
    const auto& map = value.GetMapSafe();

    for (const auto& [key, _] : map) {
        Y_ENSURE(
            RequiredKeys.contains(key) || OptionalKeys.contains(key),
            "Unknown key: " << key
        );
    }

    for (const auto& key : RequiredKeys) {
        Y_ENSURE(map.contains(key), "Missing key: " << key);
    }
}


}
