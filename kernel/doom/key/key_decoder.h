#pragma once

#include "decoded_key.h"
#include "key_decoding_options.h"

namespace NDoom {

class TKeyDecoder {
public:
    TKeyDecoder(EKeyDecodingOptions options = NoDecodingOptions)
        : Options_(options)
    {
        Y_ASSERT((Options_ & EKeyDecodingOption::IgnoreSpecialKeysDecodingOption) == Options_);
    }

    bool Decode(const TStringBuf& src, TDecodedKey* dst) const;

private:
    EKeyDecodingOptions Options_;
};

} // namespace NDoom
