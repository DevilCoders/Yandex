#pragma once

#include <util/memory/blob.h>
#include <util/system/types.h>

#include <cstring>

class TBlobReader {
public:
    TBlobReader(const TBlob& blob);

    template <typename T>
    void ReadInteger(T* integer) {
        const auto size = sizeof(*integer);
        memcpy(integer, Cursor(), size);
        Read += size;
    }

    template <typename T>
    const T* ReadArray(size_t elemCount) {
        const auto* result = reinterpret_cast<const T*>(Cursor());
        Read += sizeof(T) * elemCount;
        return result;
    }

    const ui8* Cursor() const;
    TBlob Tail() const;

private:
    const TBlob& Blob;
    size_t Read;
};
