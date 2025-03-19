#pragma once

#include "offroad_packed_key.h"

namespace NReqBundleIteratorImpl {
    struct TKeyToOffroadPackedKeyConverter {
        using TConvertedKey = TOffroadPackedKey;

        bool operator()(const TStringBuf& packedKey, TOffroadPackedKey* key) {
            Y_ASSERT(key);
            return key->Init(packedKey);
        }
    };
} // namespace NReqBundleIteratorImpl
