#pragma once

#include <util/generic/fwd.h>

namespace NSnippets {

    class TConfig;
    class TQueryy;
    class TSentsMatchInfo;

    bool LooksLikeDefinition(const TSentsMatchInfo& smi, int sentId);
    bool LooksLikeDefinition(const TQueryy& query, const TUtf16String& sentence, const TConfig& cfg);
}
