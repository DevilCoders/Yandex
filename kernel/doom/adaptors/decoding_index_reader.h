#pragma once

#include <type_traits>   /* For std::is_same. */
#include <utility>       /* For std::forward. */

#include <util/generic/strbuf.h>

#include <kernel/doom/key/decoded_key.h>
#include <kernel/doom/key/old_key_decoder.h>

namespace NDoom {


template<class Base, class Decoder = TOldKeyDecoder>
class TDecodingIndexReader: public Base {
    static_assert(std::is_same<typename Base::TKeyRef, TStringBuf>::value, "Base class is expected to use TStringBuf as key.");
    using TDecoder = Decoder;
public:
    using TKey = TDecodedKey;
    using TKeyRef = TDecodedKey;
    using TKeyData = typename Base::TKeyData;

    using Base::Base;

    TDecodingIndexReader() = default;

    template<class... Args>
    TDecodingIndexReader(EKeyDecodingOptions options, Args&&... args)
        : Base(std::forward<Args>(args)...)
        , Decoder_(options)
    {
        if (Base::HasLowerBound && (options & IgnoreSpecialKeysDecodingOption)) {
            /* '$' follows '#' in ascii, so by seeking to '$' we're effectively
             * skipping all prefixed keys. As there can be LOTS of them, this
             * really does save time. */
            Base::LowerBound("$");
        }
    }

    bool ReadKey(TKeyRef* key, TKeyData* data = NULL) {
        while (true) {
            TStringBuf baseKey;
            if (!Base::ReadKey(&baseKey, data))
                return false;

            if (!Decoder_.Decode(baseKey, key))
                continue; /* Skip invalid keys. */

            return true;
        }
    }

private:
    TDecoder Decoder_;
};


} // namespace NDoom
