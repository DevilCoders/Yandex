#pragma once

#include <util/generic/strbuf.h>

#include <contrib/libs/mms/cast.h>

namespace NMms {
    template <class T>
    const T& UnsafeCast(const char* buffer, size_t size) {
        return mms::unsafeCast<T>(buffer, size);
    }

    template <class T>
    const T& UnsafeCast(const TStringBuf& buffer) {
        return UnsafeCast<T>(buffer.data(), buffer.size());
    }

    template <class T>
    const T& SafeCast(const char* buffer, size_t size) {
        return mms::safeCast<T>(buffer, size);
    }

    template <class T>
    const T& SafeCast(const TStringBuf& buffer) {
        return SafeCast<T>(buffer.data(), buffer.size());
    }

    // Alias
    template <class T>
    const T& Cast(const char* buffer, size_t size) {
        return UnsafeCast<T>(buffer, size);
    }

    template <class T>
    const T& Cast(const TStringBuf& buffer) {
        return Cast<T>(buffer.data(), buffer.size());
    }
}
