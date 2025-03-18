#pragma once

#include <nginx/modules/strm_packager/src/common/h2645_util.h>

namespace NStrm::NPackager::Nh2645 {
    using TEPDBitReader = NMP4Muxer::TBitReader<Nh2645::TEmulationPreventionDecodeByteReader>;

    // table 'Name association to sliceType' of ISO 14496-10
    //   0 P     5 P
    //   1 B     6 B
    //   2 I     7 I
    //   3 SP    8 SP
    //   4 SI    9 SI
    enum class EAvcSlice {
        P = 0,
        B = 1,
        I = 2,
        SP = 3,
        SI = 4,
    };

    enum class EHevcSlice {
        B = 0,
        P = 1,
        I = 2,
    };

    static inline ui32 CeilLog2(ui32 x) {
        ui32 result = 0;
        --x;
        while (x > 0) {
            x >>= 1;
            ++result;
        }
        return result;
    }

    static inline bool CheckByteAligmentBits(TEPDBitReader& reader) {
        if (reader.ReadBit() != 1) { // = rbsp_stop_one_bit
            return false;
        }

        while (reader.BitsInByteLeft() != 0) {
            if (reader.ReadBit() != 0) { // = rbsp_alignment_zero_bit
                return false;
            }
        }

        return true;
    }

}
