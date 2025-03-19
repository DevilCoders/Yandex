#pragma once

#include <util/generic/noncopyable.h>

namespace NMemorySearch {

// Container for data. Hides data layout.
class TRTRawBlock : private TNonCopyable {
private:
    ui64 Data[2]; // Minimum size is 16 bytes.

protected:
    TRTRawBlock() noexcept {
    }
    ~TRTRawBlock() {
    }

    static size_t GetSize() noexcept {
        TRTRawBlock tmp; // I hope compiler will erase this.
        return (char*)&tmp.Data[0] - (char*)&tmp;
    }

public:
    ui64* GetMutableData() noexcept {
        return &Data[0];
    }
    const ui64* GetData() const noexcept {
        return &Data[0];
    }
};

} // namespace NMemorySearch
