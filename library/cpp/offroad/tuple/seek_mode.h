#pragma once

#include <type_traits>

namespace NOffroad {
    enum ESeekMode {
        /**
         * In-block seek that integrates through the block to get the
         * value at the given position. Thus works in `O(p)`, where `p` is a position
         * inside the block.
         *
         * That's basically the 'sane seek'.
         */
        IntegratingSeek,

        /**
         * In-block seek that simply jumps into provided position, discarding
         * whatever integration data was accumulated before. Basically, only works
         * on positions that were marked with `WriteSeekPoint` calls in writer.
         */
        SeekPointSeek,
    };

    using TIntegratingSeek = std::integral_constant<ESeekMode, IntegratingSeek>;
    using TSeekPointSeek = std::integral_constant<ESeekMode, SeekPointSeek>;

}
