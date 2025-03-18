#pragma once

/*
TRank data structure providing ranking over a bit array.
The rank of a bit is the number of ones that precede it in bit array (the bit itself is not included).

TRankSelect data structure providing selection (and ranking) over a bit array.
The result of select(n) is the position of the leftmost bit set to one and preceded by n ones.

Bits in bit array are numbered from zero.
*/

#include <util/memory/blob.h>
#include <util/system/defaults.h>

#define ONES_STEP_4 (0x1111111111111111ULL)
#define ONES_STEP_8 (0x0101010101010101ULL)
#define ONES_STEP_9 (1ULL << 0 | 1ULL << 9 | 1ULL << 18 | 1ULL << 27 | 1ULL << 36 | 1ULL << 45 | 1ULL << 54)
#define MSBS_STEP_8 (0x80ULL * ONES_STEP_8)
#define MSBS_STEP_9 (0x100ULL * ONES_STEP_9)
#define INCR_STEP_8 (0x80ULL << 56 | 0x40ULL << 48 | 0x20ULL << 40 | 0x10ULL << 32 | 0x8ULL << 24 | 0x4ULL << 16 | 0x2ULL << 8 | 0x1)
#define LEQ_STEP_8(x, y) ((((((y) | MSBS_STEP_8) - ((x) & ~MSBS_STEP_8)) ^ (x) ^ (y)) & MSBS_STEP_8) >> 7)
#define UCOMPARE_STEP_9(x, y) (((((((x) | MSBS_STEP_9) - ((y) & ~MSBS_STEP_9)) | (x ^ y)) ^ (x | ~y)) & MSBS_STEP_9) >> 8)
#define ULEQ_STEP_9(x, y) (((((((y) | MSBS_STEP_9) - ((x) & ~MSBS_STEP_9)) | (x ^ y)) ^ (x & ~y)) & MSBS_STEP_9) >> 8)
#define ZCOMPARE_STEP_8(x) (((x | ((x | MSBS_STEP_8) - ONES_STEP_8)) & MSBS_STEP_8) >> 7)

class IOutputStream;
class IInputStream;

namespace NSuccinctArrays {
    class TRankSelect;

    class TRank {
        friend class TRankSelect;
        ui64 NumWords_;
        ui64 NumCounts_;
        ui64 NumOnes_;
        ui64* Counts_;
        bool OwnsData_;

    public:
        TRank();
        TRank(const ui64* bits, ui64 num_bits);
        TRank(const TRank&) = delete;
        TRank& operator=(const TRank&) = delete;
        TRank(TRank&& other);
        TRank& operator=(TRank&& other);
        ~TRank();
        ui64 Rank(const ui64* bits, ui64 pos) const;
        void Save(IOutputStream* out) const;
        void SaveForReadonlyAccess(IOutputStream* out) const;
        void Load(IInputStream* inp);
        TBlob LoadFromBlob(const TBlob& blob);
        ui64 Space() const;
    };

    class TRankSelect {
    private:
        ui64 InventorySize_;
        ui64 OnesPerInventory_;
        ui64 Log2OnesPerInventory_;
        ui32* Inventory_;
        TRank Rank_;
        bool OwnsData_;

    public:
        static const ui64 npos = static_cast<ui64>(-1);
        TRankSelect(const ui64* bits, ui64 num_bits);
        TRankSelect();
        TRankSelect(const TRankSelect&) = delete;
        TRankSelect& operator=(const TRankSelect&) = delete;
        TRankSelect(TRankSelect&&);
        TRankSelect& operator=(TRankSelect&&);
        ~TRankSelect();
        ui64 Rank(const ui64* bits, ui64 pos) const;
        ui64 Select(const ui64* bits, ui64 rank) const;
        void Save(IOutputStream* out) const;
        void SaveForReadonlyAccess(IOutputStream* out) const;
        void Load(IInputStream* inp);
        TBlob LoadFromBlob(const TBlob& blob);
        ui64 Space() const;

    private:
        void Save(IOutputStream* out, bool aligned) const;
    };

}
