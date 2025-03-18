#pragma once

#include "impl/tags.h"

#include <contrib/libs/mms/map.h>

#include <util/str_stl.h>

namespace NMms {
    template <class P, class K, class V, template <class> class Cmp = TLess>
    using TMapType = mms::map<P, K, V, Cmp>;
}
