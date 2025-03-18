#pragma once

#include <library/cpp/digest/old_crc/crc.h>
#include <util/digest/fnv.h>

// Template selectors for CRC constants
namespace NCRC {
    template <typename T>
    struct TInit {};

    template <>
    struct TInit<ui32> {
        static const ui32 Val = CRC32INIT;
    };

    template <>
    struct TInit<ui64> {
        static const ui64 Val = CRC64INIT;
    };
}

template <typename T>
struct TCRCHash;

template <>
struct TCRCHash<ui32> {
    typedef ui32 TValue;
    static const ui32 Init = NCRC::TInit<ui32>::Val;
    static inline ui32 Do(ui32 val, ui32 init = Init) {
        extern const ui32* crctab32;
        ui32 crc = init ^ 0xFFFFFFFF;
        for (size_t i = 0; i < 8 * sizeof(ui32); i += 8)
            crc = (crc >> 8) ^ crctab32[(crc ^ static_cast<ui8>(val >> i)) & 0xFF];
        return crc ^ 0xFFFFFFFF;
    }
};

template <>
struct TCRCHash<ui64> {
    typedef ui64 TValue;
    static const ui64 Init = NCRC::TInit<ui64>::Val;
    static inline ui64 Do(ui64 val, ui64 init = Init) {
        extern const ui64* crctab64;
        ui64 crc = init;
        for (size_t i = 0; i < 8 * sizeof(ui64); i += 8)
            crc = crctab64[((crc >> 56) ^ static_cast<ui8>(val >> i))] ^ (crc << 8);
        return crc;
    }
};

// Template selectors for FNV constants
namespace NFNV {
    template <typename T>
    struct TInit {};

    template <>
    struct TInit<ui32> {
        static const ui32 Val = FNV32INIT;
    };

    template <>
    struct TInit<ui64> {
        static const ui64 Val = FNV64INIT;
    };

    template <typename T>
    struct TPrime {};

    template <>
    struct TPrime<ui32> {
        static const ui32 Val = FNV32PRIME;
    };

    template <>
    struct TPrime<ui64> {
        static const ui64 Val = FNV64PRIME;
    };

    template <typename T, size_t N>
    struct TPrimePower {};

    template <>
    struct TPrimePower<ui64, 0> {
        static const ui64 Val = 1;
    };

    template <>
    struct TPrimePower<ui64, 1> {
        static const ui64 Val = FNV64PRIME;
    };

    template <>
    struct TPrimePower<ui64, 2> {
        static const ui64 Val = FNV64PRIME * FNV64PRIME;
    };

    template <>
    struct TPrimePower<ui64, 3> {
        static const ui64 Val = ULL(624165263380053675);
    };

    template <>
    struct TPrimePower<ui64, 4> {
        static const ui64 Val = ULL(11527715348014283921);
    };

    template <>
    struct TPrimePower<ui64, 5> {
        static const ui64 Val = ULL(913917546033277539);
    };

    template <>
    struct TPrimePower<ui64, 6> {
        static const ui64 Val = ULL(15895002104753931833);
    };

    template <>
    struct TPrimePower<ui64, 7> {
        static const ui64 Val = ULL(14218562807570617051);
    };

    template <>
    struct TPrimePower<ui64, 8> {
        static const ui64 Val = ULL(2232315406967589409);
    };

    template <>
    struct TPrimePower<ui64, 9> {
        static const ui64 Val = ULL(10622396531520239123);
    };
}

template <typename T>
struct TFNVHash {
    typedef T TValue;
    static const T Init = NFNV::TInit<T>::Val;
    static const T Prime = NFNV::TPrime<T>::Val;

    static inline T Do(T val, T init = Init) {
        return (init * NFNV::TPrime<T>::Val) ^ val;
    }
};

template <typename T>
struct TSGIHash {
    typedef T TValue;
    static const T Init = sizeof(T);
    static inline T Do(T val, T init = Init) {
        return 5 * init + val;
    }
};
