#pragma once

#include <kernel/snippets/urlmenu/common/common.h>

#include <util/generic/string.h>

namespace NJson {
    class TJsonValue;
}

namespace NUrlMenu {
    bool Deserialize(TUrlMenuVector& urlMenu, const TString& savedObject);
    NJson::TJsonValue SerializeToJsonValue(const TUrlMenuVector& urlMenu);
    TString Serialize(const TUrlMenuVector& urlMenu);
}
