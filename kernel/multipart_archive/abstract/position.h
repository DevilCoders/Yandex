#pragma once

#include <library/cpp/logger/global/global.h>
#include <util/system/types.h>


namespace NRTYArchive {
    class TPosition {
    public:
        static const ui64 PositionBits = sizeof(ui64) * 8;
        static const ui64 PartBits = 24;
        static const ui64 OffsetBits = PositionBits - PartBits;

        static constexpr ui64 InvalidOffset = Max<ui64>(); // offset size is 40 bits only so Max<ui64>() cannot be a valid offset

    private:
        static const ui64 PartMask = ~(Max<ui64>() << (PartBits - 1));
        static const ui64 RemovedMask = 1 << (PartBits - 1);
        static const ui64 OffsetMask = Max<ui64>() << PartBits;

        static_assert(PartMask == 0x00000000007FFFFF, "");
        static_assert(RemovedMask == 0x0000000000800000, "");
        static_assert(OffsetMask == 0xFFFFFFFFFF000000, "");

    public:
        TPosition()
            : Data(0) {
            SetRemoved();
        }

        explicit TPosition(ui64 offset, ui32 part)
            : Data(0) {
            SetOffset(offset).SetPart(part);
        }

        TPosition(const TPosition&) = default;

        TPosition& operator=(const TPosition&) = default;

        static TPosition Removed() {
            return TPosition(0, 0).SetRemoved();
        }

        ui32 GetPart() const {
            return (Data & PartMask);
        }

        ui64 GetOffset() const {
            return ((Data & OffsetMask) >> PartBits);
        }

        bool IsRemoved() const {
            return Data & RemovedMask;
        }

        TPosition& SetPart(ui64 part) {
            CHECK_WITH_LOG(!IsRemoved());
            Data &= ~PartMask;
            Data |= (part & PartMask);
            return (*this);
        }

        TPosition& SetOffset(ui64 offset) {
            CHECK_WITH_LOG(!IsRemoved());
            Data &= ~OffsetMask;
            Data |= (offset << PartBits) & OffsetMask;
            return (*this);
        }

        TPosition& SetRemoved() {
            Data |= RemovedMask;
            return (*this);
        }

        ui64 Repr() const { return Data; }

        static TPosition FromRepr(ui64 repr) {
            TPosition pos;
            pos.Data = repr;
            return pos;
        }

    private:
        ui64 Data;
    };

    static_assert(sizeof(TPosition) == 8, "");
}
