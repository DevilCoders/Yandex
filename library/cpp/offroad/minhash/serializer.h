#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>

namespace NOffroad::NMinHash {

template <typename T>
struct TTriviallyCopyableKeySerializer {
    enum {
        MaxSize = sizeof(T),
    };

    static size_t Serialize(T key, TArrayRef<char> dst) {
        WriteUnaligned<T>(dst.data(), key);
        return sizeof(key);
    }

    static void Deserialize(TConstArrayRef<char> src, T* key) {
        *key = ReadUnaligned<T>(src.data());
    }
};

template <size_t SizeLimit>
struct TLimitedStringKeySerializer {
    enum {
        MaxSize = SizeLimit,
    };

    static size_t Serialize(TStringBuf key, TArrayRef<char> dst) {
        Y_ENSURE(key.size() <= SizeLimit);
        Copy(key.begin(), key.end(), dst.data());
        return key.size();
    }

    static void Deserialize(TConstArrayRef<char> src, TString* key) {
        key->assign(src.begin(), src.end());
    }
};


} // namespace NOffroad::NMinHash
