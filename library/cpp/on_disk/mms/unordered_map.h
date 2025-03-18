#pragma once

#include "impl/tags.h"
#include "type_traits.h"

#include <contrib/libs/mms/unordered_map.h>

#include <util/str_stl.h>

namespace NMms {
    template <class P, class K, class V, template <class> class Hash = THash, template <class> class Eq = TEqualTo>
    using TUnorderedMap = mms::unordered_map<P, K, V, Hash, Eq>;

    template <class P, class K, class V, template <class> class Hash = THash, template <class> class Eq = TEqualTo>
    using TFastUnorderedMap = mms::unordered_map<P, K, V, Hash, Eq, TFastTraits>;
}
