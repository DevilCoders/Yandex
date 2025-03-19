#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NUrlId {
    typedef ui64 TUrlId;

    char* Hash2Str(TUrlId hash, char* buf);

    static inline TString Hash2Str(TUrlId hash) {
        char buf[16];

        return TString(buf, Hash2Str(hash, buf));
    }

    template <bool prependZ>
    static inline TString& Hash2Str(TUrlId hash, TString& buffer) {
        buffer.resize(16 + prependZ);

        if (prependZ)
            *buffer.begin() = 'Z';

        char* end = Hash2Str(hash, buffer.begin() + prependZ);
        buffer.resize(end - buffer.data());
        return buffer;
    }

    static inline TString& Hash2Str(TUrlId hash, TString& buffer) {
        return Hash2Str<false>(hash, buffer);
    }

    static inline TString& Hash2StrZ(TUrlId hash, TString& buffer) {
        return Hash2Str<true>(hash, buffer);
    }

    //! @note This overload is a bit slower than Hash2StrZ (hash, buffer)
    static inline TString Hash2StrZ(TUrlId hash) {
        TString buffer;
        return Hash2Str<true>(hash, buffer);
    }

    //! Read the @c buf with ASCII hex-encoded number and decode it to @c to.
    //! This function performs no additional parsing.
    void Str2Hash(const char* buf, size_t len, TUrlId* to);

    static inline TUrlId Str2Hash(const char* buf, size_t len) {
        TUrlId ret;

        Str2Hash(buf, len, &ret);

        return ret;
    }

    template <class T>
    static inline TUrlId Str2Hash(const T& from) {
        return Str2Hash(from.data(), from.size());
    }
}
