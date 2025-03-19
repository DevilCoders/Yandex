#pragma once

#include <util/digest/numeric.h>
#include <util/digest/murmur.h>
#include <util/generic/strbuf.h>

namespace NQueryData {

    class TInferiorityClass {
    public:
        ui64 Hash = 0;
        ui64 Inferiority = 0;

        void AddSubkey(TStringBuf k) {
            Hash = CombineHashes(Hash, MurmurHash<ui64>(k.data(), k.size()));
        }

        void AddSubinferiority(ui8 p) {
            Inferiority = (Inferiority << 8) | p;
        }

        ui32 GetPriority32() const {
            return Max<ui32>() & GetPriority64();
        }

        ui64 GetPriority64() const {
            return Max<i64>() - (Inferiority & Max<i64>());
        }

        bool operator<(const TInferiorityClass& c) const {
            return Hash < c.Hash || (Hash == c.Hash && Inferiority < c.Inferiority);
        }

        bool InferiorTo(const TInferiorityClass& c) const {
            return Hash == c.Hash && Inferiority > c.Inferiority;
        }

        static const ui32 MaxPrioritizedSubkeys32;
        static const ui32 MaxPrioritizedSubkeys64;
        static const ui32 MaxHierarchyDepth;
    };

}
