#pragma once

#include <util/system/defaults.h>

namespace NProtoParser {

template <class TOpt>
struct TOptionGetter {
    typedef typename TOpt::TypeTraits TTypeTraits;
    const TOpt& Opt;
    Y_FORCE_INLINE TOptionGetter(const TOpt& o)
        : Opt(o)
    {
    }
    template <class TSource>
    Y_FORCE_INLINE typename TTypeTraits::ConstType operator() (const TSource& src) const {
        return src.options().HasExtension(Opt) ? src.options().GetExtension(Opt) : Opt.default_value();
    }
};

template <class TOpt>
Y_FORCE_INLINE TOptionGetter<TOpt> CreateOptionGetter(const TOpt& opt) {
    return TOptionGetter<TOpt>(opt);
}

} // NProtoParser
