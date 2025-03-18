#pragma once

namespace NFraming {
    enum class EFormat {
        Auto = 0,         // protoseq codec in packer and auto detect codec for unpacker
        Protoseq = 1,     // protoseq codec, when each frame followed by special sequence of bytes, which allows
                          // to restore part of chunks even message was partially corrupted
        Lenval = 2,       // lightweight version of codec, which has small size, but do not safe from partial corrupts
        LightProtoseq = 3,// protoseq codec without sync frame
    };
}
