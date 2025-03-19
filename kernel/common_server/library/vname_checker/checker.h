#pragma once
#include <util/generic/string.h>
#include <util/generic/set.h>

namespace NCS {
    class TVariableNameChecker {
    public:
        static bool DefaultObjectId(const TStringBuf objectId, const TSet<char>& additionalChars = {});
    };
}
