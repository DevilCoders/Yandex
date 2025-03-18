#pragma once

#include <util/generic/bitops.h>

// https://github.com/lemire/streamvbyte/blob/master/src/streamvbyte_zigzag.c
i64 ZigZagDecode(ui64 value) {
    return (value >> 1) ^ -(value & 1);
}

// https://github.com/git/git/blob/1c4b6604126781361fe89d88ace70f53079fbab5/varint.c
ui64 VarIntDecode(const ui8** buf, size_t size) {
    const ui8* ptr = *buf;
    const ui8* const end = *buf + size;
    if (ptr == end) {
        return 0;
    }
    ui8 c = *ptr++;
    ui64 value = c & 0x7F;
    while (c & 0x80) {
        value += 1;
        if (!value || (value & InverseMaskLowerBits(64 - 7))) {
            return 0; // overflow
        }
        if (ptr == end) {
            return 0;
        }
        c = *ptr++;
        value = (value << 7) + (c & 0x7F);
    }
    *buf = ptr;
    return value;
}
