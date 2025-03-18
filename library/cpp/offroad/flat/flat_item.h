#pragma once

#include <array>

#include <util/system/types.h>
#include <util/system/yassert.h>
#include <util/system/unaligned_mem.h>

namespace NOffroad {
    template <size_t keySize, size_t dataSize>
    class TFlatItem: public std::array<ui32, keySize + dataSize> {
        using TBase = std::array<ui32, keySize + dataSize>;

    public:
        using TKeyArray = std::array<ui32, keySize>;
        using TDataArray = std::array<ui32, dataSize>;

        TKeyArray& Key() {
            return *reinterpret_cast<TKeyArray*>(TBase::data());
        }

        const TKeyArray& Key() const {
            return *reinterpret_cast<const TKeyArray*>(TBase::data());
        }

        ui32 Key(size_t i) const {
            Y_ASSERT(i < keySize);

            return *(TBase::data() + i);
        }

        size_t KeySize() const {
            return keySize;
        }

        TDataArray& Data() {
            return *reinterpret_cast<TDataArray*>(TBase::data() + keySize);
        }

        const TDataArray& Data() const {
            return *reinterpret_cast<const TDataArray*>(TBase::data() + keySize);
        }

        ui32 Data(size_t i) const {
            Y_ASSERT(i < dataSize);

            return *(TBase::data() + keySize + i);
        }

        size_t DataSize() const {
            return dataSize;
        }
    };

}
