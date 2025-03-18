#pragma once

#include "offset_key_data.h"
#include "hit_count_key_data.h"

namespace NOffroad {
    template <class KeyData>
    struct TKeyDataTraits;

    template <>
    struct TKeyDataTraits<TOffsetKeyData> {
        using TFactory = TOffsetKeyDataFactory;
        using TSerializer = TOffsetKeyDataSerializer;
        using TSubtractor = TOffsetKeyDataSubtractor;
        using TVectorizer = TOffsetKeyDataVectorizer;
    };

    template <>
    struct TKeyDataTraits<THitCountKeyData> {
        using TFactory = THitCountKeyDataFactory;
        using TSerializer = THitCountKeyDataSerializer;
        using TSubtractor = THitCountKeyDataSubtractor;
        using TVectorizer = THitCountKeyDataVectorizer;
    };

}
