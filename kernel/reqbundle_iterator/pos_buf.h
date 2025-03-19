#pragma once

#include "position.h"
#include "reqbundle_iterator_fwd.h"

namespace NReqBundleIterator {
    struct TPosBuf {
        NReqBundleIterator::TPosition* Pos = nullptr;
        ui16* RichTreeForms = nullptr;
        size_t Count = 0;
    };
} // NReqBundleIterator
