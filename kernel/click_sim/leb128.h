#pragma once

#include <util/system/types.h>
#include <limits>

/***
 *  LEB128 or Little Endian Base 128 is a variable-length code compression used to store arbitrarily large integers in a small number of bytes.
 */

// TBufferIt can be raw ui8* pointer
template <typename UIntT, typename TBufferIt>
TBufferIt WriteLeb128(UIntT value, TBufferIt buffer) {
    static_assert(!std::numeric_limits<UIntT>::is_signed);
    static_assert(sizeof(UIntT) >= 2);

    while (value >= 0x80) {
        *buffer = 0x80u | (value & 0x7Fu);
        ++buffer;
        value >>= 7u;
    }
    *buffer = value;
    ++buffer;
    return buffer;
}

// TBufferIt can be raw const ui8* pointer
template <typename UIntT, typename TBufferIt>
TBufferIt ReadLeb128(UIntT& value, TBufferIt buffer) {
    static_assert(!std::numeric_limits<UIntT>::is_signed);
    static_assert(sizeof(UIntT) >= 2);

    value = static_cast<ui32>(*buffer & 0x7Fu);
    ui32 bitOffset = 0;
    while (*buffer & 0x80u) {
        ++buffer;
        bitOffset += 7;
        value |= static_cast<ui32>(*buffer & 0x7Fu) << bitOffset;
    }
    ++buffer;
    return buffer;
}
