#pragma once

#include <util/generic/flags.h>

namespace NDoom {

enum EKeyDecodingOption {
    NoDecodingOptions = 0,
    IgnoreSpecialKeysDecodingOption = 0x1,  /**< Ignore keys with prefixes and zone keys. */
    RecodeToUtf8DecodingOption = 0x2,       /**< Treat keys as yandex-encoded with optional UTF8 prefix. */
};

Y_DECLARE_FLAGS(EKeyDecodingOptions, EKeyDecodingOption)
Y_DECLARE_OPERATORS_FOR_FLAGS(EKeyDecodingOptions)

} // namespace NDoom
