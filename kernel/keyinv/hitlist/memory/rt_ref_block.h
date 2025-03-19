#pragma once

#include "shortrefops.h"

#include <util/generic/mem_copy.h>
#include <util/generic/utility.h>

namespace NMemorySearch {

template<class TRefCountedBlock>
class TRTRefData {
private:
    const ui64* BaseData;

public:
    static const ui64 POINTER_MASK = 0x8C;
    static const ui64 INIT_MASK = 0xF8;

    TRTRefData(const ui64* baseData) noexcept
        : BaseData(baseData)
    { }
    // State indicators
    bool IsPointer() const noexcept {
        return (BaseData[0] & POINTER_MASK) == POINTER_MASK;
    }
    bool IsInitState() const noexcept {
        return BaseData[0] == INIT_MASK;
    }

    // Non-Init state
    size_t GetShift() const noexcept {
        return IsPointer() ? 2 : 0;
    }
    const ui64* GetBaseData() const noexcept {
        return BaseData;
    }
    const ui64* GetData() const noexcept {
        return BaseData + GetShift();
    }

    // Hits state
    ui32 GetStartHitsCount() const noexcept {
        return IsInitState() ? 0 : (BaseData[0] & RT_HITS_CODER_COUNT_MASK) + 1;
    }
    const ui64* GetDataEnd() const noexcept {
        return GetData() + 2;
    }

    // Pointer state
    TRefCountedBlock* GetPointer() const noexcept {
        return reinterpret_cast<TRefCountedBlock*>(BaseData[1]);
    }
    ui32 GetCount() const noexcept {
        ui64 blockHeader = *BaseData;
        ui32 part1 = blockHeader % 4;
        ui32 part2 = (blockHeader >> 2) & 0xC;
        ui32 part3 = (blockHeader >> 4) & ULL(0xFFFFFFF0);
        return part1 | part2 | part3;
    }
    ui32 GetLength() const noexcept {
        return BaseData[0] >> 36 << 4;
    }

    // All states
    ui32 SmartGetCount() const noexcept {
        return IsPointer() ? GetCount() : GetStartHitsCount();
    }
};

/*
 * Adds optional smart intrusive pointer, count, length and makes struct encoding compatible with hits encoding format.
 * Defines following packing of ref-blocks:
 *     ui64 CountPart1:2;
 *     ui64 MustBeOne1:2;
 *     ui64 CountPart2:2;
 *     ui64 Reserved:1;
 *     ui64 MustBeOne2:1;
 *     ui64 CountPart3:28;
 *     ui64 Top28OfLength:28;
 *     ui64 Pointer;
 *
 * WARNING: copy constructor is NOT excecuted for base class
 */
template<class TBaseBlock, class TRefCountedBlock, class TOps = TShortIntrusivePtrOps<TRefCountedBlock> >
class TRTRefBlock : public TBaseBlock {
private:
    typedef TBaseBlock TBase;
    typedef TRTRefData<TRefCountedBlock> TRefData;

    // Hiding
    using TBase::GetSize;
    using TBase::GetMutableData;
    using TBase::GetData;

    inline void DoClear() noexcept {
        ui64* data = GetMutableData();
        data[0] = TRefData::INIT_MASK;
        data[1] = 0;
    }
    inline void DoCopy(const ui64* blockData) noexcept {
        ui64* data = GetMutableData();
        data[0] = blockData[0];
        data[1] = blockData[1];
    }
    inline void DoRef(TRefCountedBlock& block, ui32 count, ui32 length) noexcept {
        ui64* data = GetMutableData();
        TOps::Ref(&block);
        Y_VERIFY(length % 16 == 0, "Internal error!");
        ui64 longCount = static_cast<ui64>(count);
        ui64 lengthMask = static_cast<ui64>(length) << 32;
        ui64 countMaskPart1 = longCount % 4;
        ui64 countMaskPart2 = (longCount & 0xC) << 2;
        ui64 countMaskPart3 = (longCount & ULL(0xFFFFFFF0)) << 4;
        data[0] = TRefData::POINTER_MASK | lengthMask |
            countMaskPart1 | countMaskPart2 | countMaskPart3;
        data[1] = reinterpret_cast<ui64>(&block);
    }
    inline void DoSmartCopy(const TRefData& block) noexcept {
        const ui64* blockData = block.GetBaseData();
        DoCopy(blockData);
        if (block.IsPointer()) {
            TOps::Ref(block.GetPointer());
        }
    }
    static inline void DoSmartSwap(TRTRefBlock& block1, TRTRefBlock& block2) noexcept {
        ui64* data1 = block1.GetMutableData();
        ui64* data2 = block2.GetMutableData();
        DoSwap(data1[0], data2[0]);
        DoSwap(data1[1], data2[1]);
    }
    inline void DoUnRef() const noexcept {
        TRefData tmp(GetData());
        if (tmp.IsPointer()) {
            TRefCountedBlock* block = tmp.GetPointer();
            if (block != nullptr) {
                TOps::UnRef(block);
            }
        }
    }

protected:
    TRTRefBlock() noexcept {
        DoClear();
    }
    TRTRefBlock(const TRefData& refData) noexcept {
        DoSmartCopy(refData);
    }
    TRTRefBlock(const TRefData& refData, const ui64* start, const ui64* end) noexcept {
        DoSmartCopy(refData);
        MemCopy(GetMutableData() + refData.GetShift(), start, end - start);
    }
    ~TRTRefBlock() {
        DoUnRef();
    }

    void Reset(const ui64* start, const ui64* end) {
        Y_VERIFY(end - start <= 2, "Internal error!");
        DoUnRef();
        if (end == start) {
            DoClear();
        } else {
            DoCopy(start);
        }
    }
    void Reset(TRefCountedBlock& block, ui32 count, ui32 length) noexcept {
        DoUnRef();
        DoRef(block, count, length);
    }

    // To be used only in TRTImmediateBlock
    void Swap(TRTRefBlock& block) noexcept {
        DoSmartSwap(*this, block);
    }

    TRefData GetRefData() const noexcept {
        return TRefData(GetData());
    }

    static size_t GetRequiredSize(const TRefData& data, const ui64* start, const ui64* end) noexcept {
        return TBase::GetSize() + data.GetShift() * sizeof(ui64) + (end - start) * sizeof(ui64);
    }
};

} // namespace NMemorySearch
