#pragma once

#include <util/generic/map.h>

namespace NTranslit {
    using TCharTable = TMap<size_t, TStringBuf>;

    const TCharTable& GetLatin();
    const TCharTable& GetCyrillic();
    const TCharTable& GetGreek();
}
