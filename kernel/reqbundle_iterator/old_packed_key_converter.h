#pragma once

#include <ysite/yandex/posfilter/old_packed_key.h>

namespace NReqBundleIteratorImpl {
    struct TTermKeyToOldPackedKeyConverter {
        using TConvertedKey = TOldPackedKey;

        bool operator()(const TStringBuf& packedKey, TOldPackedKey* key) const {
            Y_ASSERT(key);
            key->Init(packedKey.data());
            return true;
        }
    };
} // namespace NReqBundleIteratorImpl
