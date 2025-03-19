#pragma once

#include <util/generic/string.h>

#include "decoded_key.h"
#include "key_decoding_options.h"

namespace NDoom {

class TOldKeyDecoder {
public:
    TOldKeyDecoder(EKeyDecodingOptions options = RecodeToUtf8DecodingOption): Options_(options) {}

    bool Decode(const TStringBuf& src, TDecodedKey* dst) const;

private:
    EKeyDecodingOptions Options_;
};


} // namespace NDoom

