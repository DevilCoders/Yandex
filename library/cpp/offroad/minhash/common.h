#pragma once

#include <util/digest/city.h>
#include <util/generic/array_ref.h>
#include <util/system/types.h>

namespace NOffroad::NMinHash {

struct TItemHash {
    ui32 Lo = 0;
    ui32 Hi = 0;

    ui64 AsUi64() const {
        return (ui64{Hi} << 32) | Lo;
    }
};

template <bool HasCheckSum>
struct TItem; 

template <>
struct TItem</* HasCheckSum */ false> : public TItemHash {
};

template <>
struct TItem</* HasCheckSum */ true> : public TItemHash {
    ui32 CheckSum = 0;
};

template <bool HasCheckSum>
struct TBaseHashValue {
    TItem<HasCheckSum> Item;
    ui32 BucketId = Max<ui32>();
};

template <bool HasCheckSum>
class TBaseHash {
public:
    using TResult = TBaseHashValue<HasCheckSum>;

    TBaseHash() = default;

    TBaseHash(ui64 seed, ui32 numSlots, ui32 numBuckets) {
        Reset(seed, numSlots, numBuckets);
    }

    void Reset(ui64 seed, ui32 numSlots, ui32 numBuckets) {
        Seed_ = seed;
        NumSlots_ = numSlots;
        NumBuckets_ = numBuckets;
    }

    TResult operator()(TConstArrayRef<char> value) const {
        auto [lo, hi] = CityHash128WithSeed(value.data(), value.size(), { Seed_, 0 });
        TResult result;
        result.BucketId = SelectBits<0, 32>(lo) % NumBuckets_;
        result.Item.Lo = SelectBits<32, 32>(lo) % NumSlots_;
        result.Item.Hi = SelectBits<0, 32>(hi) % (NumSlots_ - 1) + 1;

        if constexpr (HasCheckSum) {
            result.Item.CheckSum = SelectBits<32, 32>(hi);
        }

        return result;
    }

private:
    ui64 Seed_ = 0;
    ui32 NumSlots_ = 0;
    ui32 NumBuckets_ = 0;
};

template <typename T>
struct TArithmeticProgression {
    T Init = T{};
    T Diff = T{};

    template <typename U>
    auto operator()(U x) {
        return Init + std::common_type_t<U, T>(x) * Diff;
    }
};

Y_FORCE_INLINE ui32 CalculateItemSlot(ui32 numSlots, const TItemHash& item, TArithmeticProgression<ui32> progression) {
    return static_cast<ui32>((item.Lo + progression(static_cast<ui64>(item.Hi))) % numSlots);
}

} // namespace NOffroad::NMinHash
