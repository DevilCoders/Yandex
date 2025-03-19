#pragma once

#include <util/system/defaults.h>

namespace NMemorySearch {

// Allows extensions with predefined meaning of several ui64.
template<class TBaseBlock, size_t extensionSize>
class TRTBlockExtension : public TBaseBlock {
private:
    typedef TBaseBlock TBase;

protected:
    TRTBlockExtension() noexcept {
    }
    ~TRTBlockExtension() {
    }

    ui64& MutableData(size_t i) noexcept {
        return TBase::GetMutableData()[i];
    }
    const ui64& Data(size_t i) const noexcept {
        return TBase::GetData()[i];
    }
    static size_t GetSize() noexcept {
        return TBase::GetSize() + sizeof(ui64) * extensionSize;
    }

public:
    ui64* GetMutableData() noexcept {
        return TBase::GetMutableData() + extensionSize;
    }
    const ui64* GetData() const noexcept {
        return TBase::GetData() + extensionSize;
    }
};

} // namespace NMemorySearch
