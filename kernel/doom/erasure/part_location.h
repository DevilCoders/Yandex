#pragma once

#include <util/system/types.h>

namespace NDoom {
    struct TErasureBlobLocation {
        ui32 Part = 0;
        ui64 Offset = 0;
        ui64 Size = 0;
    };

    class TErasureBlobLocationVectorizer {
    public:
        enum {
            TupleSize = 5
        };

        template <class Slice>
        static void Gather(Slice&& slice, TErasureBlobLocation* data) {
            data->Part = slice[0];
            data->Offset = (static_cast<ui64>(slice[1]) << 32) | slice[2];
            data->Size = ((static_cast<ui64>(slice[3]) << 32) | slice[4]);
        }

        template <class Slice>
        static void Scatter(TErasureBlobLocation data, Slice&& slice) {
            slice[0] = data.Part;
            slice[1] = data.Offset >> 32;
            slice[2] = data.Offset;
            slice[3] = data.Size >> 32;
            slice[4] = data.Size;
        }
    };

    struct TDocInPartLocation {
        ui32 Part = 0;
        ui32 PartPosition = 0;
    };

    class TDocInPartLocationVectorizer {
    public:
        enum {
            TupleSize = 2
        };

        template <class Slice>
        static void Gather(Slice&& slice, TDocInPartLocation* data) {
            data->Part = slice[0];
            data->PartPosition = slice[1];
        }

        template <class Slice>
        static void Scatter(TDocInPartLocation data, Slice&& slice) {
            slice[0] = data.Part;
            slice[1] = data.PartPosition;
        }
    };
}
