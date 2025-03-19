#pragma once

#include "decoded_key.h"

#include <util/generic/yexception.h>

namespace NDoom {


template <class Decoder>
class TKeyDecodingTransformation {
public:
    bool operator()(const TStringBuf& key, TDecodedKey* result) {
        Y_ASSERT(result);
        return Decoder_.Decode(key, result);
    }

private:
    Decoder Decoder_;
};


} // namespace NDoom
