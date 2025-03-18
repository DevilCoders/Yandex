#pragma once

#include "impl/tags.h"
#include "type_traits.h"

#include <contrib/libs/mms/unordered_set.h>

#include <util/str_stl.h>

namespace NMms {
    template <class P, class T, template <class> class Hash = THash, template <class> class Eq = TEqualTo>
    using TUnorderedSet = mms::unordered_set<P, T, Hash, Eq>;

    template <class P, class T, template <class> class Hash = THash, template <class> class Eq = TEqualTo>
    using TFastUnorderedSet = mms::unordered_set<P, T, Hash, Eq, TFastTraits>;
}
