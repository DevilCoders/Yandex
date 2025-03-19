#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGzt {

template <typename TCharType>
struct TCharStringTraits {
    using TStringType = TBasicString<TCharType>;
};


// for TCompactTrieBuilder::TKey
typedef ui32 TWordId;

template <>
struct TCharStringTraits<TWordId> {
    using TStringType = TVector<TWordId>;
};

} // namespace NGzt
