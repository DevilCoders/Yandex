#pragma once

#include "impl/tags.h"

#include <contrib/libs/mms/set.h>

#include <util/str_stl.h>

namespace NMms {
    template <class P, class T, template <class> class Cmp = TLess>
    using TSetType = mms::set<P, T, Cmp>;
}
