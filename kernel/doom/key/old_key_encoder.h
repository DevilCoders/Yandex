#pragma once

#include <util/generic/string.h>

#include "decoded_key.h"

namespace NDoom {

class TOldKeyEncoder {
public:
    TOldKeyEncoder() = default;

    bool Encode(const TDecodedKey& src, char* dst, size_t* dstLength) const;
    bool Encode(const TDecodedKey& src, char* dst) const;
    bool Encode(const TDecodedKey& src, TString* dst) const;
    TString Encode(const TDecodedKey& src) const;
};

} // namespace NDoom
