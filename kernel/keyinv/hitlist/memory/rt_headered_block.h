#pragma once

#include "rt_ext_block.h"
#include "rt_raw_block.h"

#include <util/system/yassert.h>

namespace NMemorySearch {

// Adds ref counting.
class TRTHeaderedBlock : public TRTBlockExtension<TRTRawBlock, /*extensionSize=*/1> {
protected:
    TRTHeaderedBlock() noexcept {
        MutableData(0) = 0;
    }
    ~TRTHeaderedBlock() {
        Y_ASSERT(Data(0) == 0);
    }

public:
    ui64 GetRefCount() const noexcept {
        return Data(0);
    }
    void Ref() noexcept {
        ++MutableData(0);
    }
    void UnRef() noexcept {
        --MutableData(0);
    }
};

} // namespace NMemorySearch
