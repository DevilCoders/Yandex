#pragma once

#include "decoded_key.h"

namespace NDoom {

class TKeyEncoder {
public:
    TKeyEncoder() = default;

    bool Encode(const TDecodedKey& src, TString* dst) const;
};

} // namespace NDoom
