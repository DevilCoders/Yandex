#pragma once

#include "rt_headered_block.h"
#include "rt_hits_coders_internal.h"
#include "rt_ref_block.h"

#include <util/system/sys_alloc.h>

#define MAX_RT_HITS_BLOCKS 3

namespace NMemorySearch {

class TRTRefVariableBlock : public TRTRefBlock<TRTHeaderedBlock, TRTRefVariableBlock> {
private:
    typedef TRTRefBlock<TRTHeaderedBlock, TRTRefVariableBlock> TBase;
    typedef TRTRefData<TRTRefVariableBlock> TRefData;

    TRTRefVariableBlock(const TRefData& refData, const ui64* start, const ui64* end) noexcept
        : TBase(refData, start, end)
    { }

public:
    static TRTRefVariableBlock* CreateBlock(const TRefData& refData, const ui64* start, const ui64* end) {
        size_t blockSize = TBase::GetRequiredSize(refData, start, end);
        void* raw = y_allocate(blockSize);
        new(raw) TRTRefVariableBlock(refData, start, end);
        return static_cast<TRTRefVariableBlock*>(raw);
    }

    static bool Next(TRefData& refData) noexcept {
        if (refData.IsPointer()) {
            refData = refData.GetPointer()->GetRefData();
            return true;
        }
        return false;
    }
};

class TRTImmediateBlock : public TRTRefBlock<TRTRawBlock, TRTRefVariableBlock> {
private:
    typedef TRTRefBlock<TRTRawBlock, TRTRefVariableBlock> TBase;

protected:
    TRTImmediateBlock() noexcept
        : TBase()
    { }
    TRTImmediateBlock(const TRTImmediateBlock& block) noexcept
        : TBase(block.GetRefData())
    { }

public:
    typedef TRTRefData<TRTRefVariableBlock> TRefData;
    using TBase::GetRefData;

    void SwitchTo(const TRefData& refData, const ui64* start, const ui64* end, ui32 count) {
        if (!refData.IsPointer() && end - start <= 2) {
            Reset(start, end);
            return;
        }
        Y_VERIFY(end != start, "Internal bug!");
        TRTRefVariableBlock* newBlock = TRTRefVariableBlock::CreateBlock(refData, start, end);
        ui32 length = (end - start) * sizeof(ui64);
        if (refData.IsPointer()) {
            count += refData.GetCount();
            length += refData.GetLength();
        }
        Reset(*newBlock, count, length);
    }

    void Swap(TRTImmediateBlock& block) noexcept {
        TBase::Swap(static_cast<TBase&>(block));
    }
};

} // namespace NMemorySearch
