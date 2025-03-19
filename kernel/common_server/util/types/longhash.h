#pragma once

#include <util/string/hex.h>
#include <util/system/defaults.h>
#include <util/str_stl.h>

template <size_t N>
struct TFixedSizeHashable {
public:
    static_assert(N > sizeof(ui64), "size must be greater than 8 bytes");
    static_assert(N % sizeof(ui64) == 0, "size must be a multiple of 8");

    static constexpr size_t Size = N / sizeof(ui8);
public:
    explicit TFixedSizeHashable(bool zeroed = true) {
        if (zeroed)
            memset(Data, 0, Size);
    }
    explicit TFixedSizeHashable(const void* data) {
        memcpy(Data, data, Size);
    }

    ui8* Ptr() {
        return Data;
    }

    template <class T>
    T Head() const {
        return *((T*)Ptr());
    }

    const ui8* Ptr() const {
        return Data;
    }

    void operator=(const TFixedSizeHashable<N>& o) {
        memcpy(Data, o.Data, Size);
    }

    bool operator<(const TFixedSizeHashable<N>& o) const {
        return memcmp(Data, o.Data, Size) < 0;
    }

    bool operator==(const TFixedSizeHashable<N>& o) const {
        return memcmp(Data, o.Data, Size) == 0;
    }

    bool operator!=(const TFixedSizeHashable<N>& o) const {
        return memcmp(Data, o.Data, Size) != 0;
    }

    TString Quote() const {
        return HexEncode(Ptr(), N);
    }
private:
    ui8 Data[Size];
};

template<size_t N>
struct hash<TFixedSizeHashable<N>> {
    inline size_t operator() (const TFixedSizeHashable<N>& t) const noexcept {
        return (size_t&)(*t.Ptr());
    }
};
