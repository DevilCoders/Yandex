#pragma once

#include <util/generic/strbuf.h>

namespace NUtil {
    TStringBuf GetAttributeValue(const TStringBuf& name, TStringBuf query);
}
