#pragma once

#include <util/ysaveload.h>
#include <util/system/types.h>

namespace NDoom {

    struct TInvHashEntry {
        ui64 Hash = 0;
        ui32 DocId = 0;

        Y_SAVELOAD_DEFINE(Hash, DocId);

        bool operator==(const TInvHashEntry& rhs) const {
            return Hash == rhs.Hash && DocId == rhs.DocId;
        }

        bool operator!=(const TInvHashEntry& rhs) const {
            return Hash != rhs.Hash || DocId != rhs.DocId;
        }
    };

    class TInvHashEntryVectorizer {
    public:
        enum {
            TupleSize = 4
        };

        template <class Slice>
        static void Gather(Slice&& slice, TInvHashEntry* data) {
            data->Hash = (static_cast<ui64>(slice[0]) << 32) | (slice[1] << 8) | slice[2];
            // -1 to allow zero DocId to be last in the file with AutoEof tuple writer
            data->DocId = slice[3] - 1;
        }

        template <class Slice>
        static void Scatter(TInvHashEntry data, Slice&& slice) {
            slice[0] = data.Hash >> 32;
            slice[1] = static_cast<ui32>(data.Hash) >> 8;
            slice[2] = data.Hash & ((ui32(1) << 8) - 1);
            slice[3] = data.DocId + 1;
        }
    };

    class TInvHashEntryPrefixVectorizer {
    public:
        enum {
            TupleSize = 2
        };

        template <class Slice>
        static void Gather(Slice&& slice, TInvHashEntry* data) {
            data->Hash = (static_cast<ui64>(slice[0]) << 32) | (slice[1] << 8);
            data->DocId = 0;
        }

        template <class Slice>
        static void Scatter(TInvHashEntry data, Slice&& slice) {
            slice[0] = data.Hash >> 32;
            slice[1] = static_cast<ui32>(data.Hash) >> 8;
        }
    };
}
