#pragma once

#include <util/stream/input.h>

namespace NLemmer {
    void SetMorphFixList(const char* filename, bool replaceAlreadyLoaded = true);
    void SetMorphFixList(IInputStream& src, bool replaceAlreadyLoaded = true);
}
