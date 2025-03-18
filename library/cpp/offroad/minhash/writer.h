#pragma once

#include "common.h"
#include "header.h"
#include "hit.h"
#include "io.h"
#include "serializer.h"

#include <library/cpp/offroad/flat/flat_writer.h>

#include <library/cpp/containers/bitseq/bitvector.h>
#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/iterator/cartesian_product.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/minhash/prime.h>

#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/random/fast.h>
#include <util/system/types.h>

namespace NOffroad::NMinHash {

struct TOutputStreams {
    IOutputStream* Header = nullptr;
    IOutputStream* Buckets = nullptr;
    IOutputStream* EmptySlots = nullptr;
};

class TMinHashWriter {
public:
    TMinHashWriter() = default;

    TMinHashWriter(const TOutputStreams& streams) {
        Reset(streams);
    }

    void Reset(const TOutputStreams& streams) {
        Streams_ = streams;
        BucketsWriter_.Reset(streams.Buckets);
        EmptySlotsWriter_.Reset(streams.EmptySlots);
    }

    void WriteHeader(THeader header) {
        ::Save(Streams_.Header, header);
        Streams_.Header->Finish();
    }

    void WriteBucket(TArithmeticProgression<ui32> params) {
        TArithmeticProgressionHit hit{ params.Init, params.Diff };
        BucketsWriter_.Write(hit, nullptr);
    }

    void WriteEmptySlot(ui32 slotIndex) {
        EmptySlotsWriter_.Write(slotIndex, nullptr);
    }

    void Finish() {
        BucketsWriter_.Finish();
        EmptySlotsWriter_.Finish();
    }

private:
    TOutputStreams Streams_;

    TMinHashIo::TArithmeticProgressionWriter BucketsWriter_;
    TMinHashIo::TEmptySlotsWriter EmptySlotsWriter_;
};

struct TMinHashParams {
    ui32 NumKeys = 0;
    ui32 KeysPerBucket = 5;
    ui32 Seed = 0;
    double LoadFactor = 0.995;
};

template <typename Key, typename KeySerializer, bool HasCheckSum>
class TMinHashBuilder {
    inline static constexpr ui32 NumSeedSelectionIters = 1000;
    inline static constexpr ui32 NumIters = 1000;

    struct TBucket {
        ui32 Id = 0;
        ui32 Offset = 0;
        ui32 Size = 0;
    };

    using THash = TBaseHashValue<HasCheckSum>;

    struct TItem {
        THash Hash;
        ui32 KeyId = 0;
        ui32 Slot = 0;
    };

public:
    TMinHashBuilder(const TMinHashParams& params)
        : SeedRng_{ params.Seed }
        , NumKeys_{ params.NumKeys }
        , NumSlots_{ CalcNumSlots(params.NumKeys, params.LoadFactor) }
        , NumBuckets_{ params.NumKeys / params.KeysPerBucket + 1 }
        , MaxProbeIter_{ static_cast<ui32>((Log2(NumKeys_ + 2) / 20) * (1 << 20)) }
    {
    }

    template <typename C>
    void Build(const C& cont) {
        using std::begin;
        using std::end;
        Build(begin(cont), end(cont));
    }

    template <typename It>
    void Build(It begin, It end) {
        for (ui32 _ : xrange(NumIters)) {
            Y_UNUSED(_);

            if (TryBuild(begin, end)) {
                return;
            }
        }

        Y_ENSURE(false, "Failed to build minhash in required number of iterations");
    }

    void Finish(TMinHashWriter* writer) const {
        THeader header;
        header.NumSlots = NumSlots_;
        header.NumBuckets = NumBuckets_;
        header.Seed = Seed_;
        writer->WriteHeader(header);

        for (auto bucket : BucketParams_) {
            writer->WriteBucket(bucket);
        }

        for (ui32 slot : EmptySlots_) {
            writer->WriteEmptySlot(slot);
        }

        writer->Finish();
    }

    TConstArrayRef<TItem> Items() const {
        return Items_;
    }

    TConstArrayRef<ui32> EmptySlots() const {
        return EmptySlots_;
    }

    TConstArrayRef<TArithmeticProgression<ui32>> Buckets() const {
        return BucketParams_;
    }

    ui32 NumHolesBeforeSlot(ui32 slot) const {
        if (EmptySlots_.empty()) {
            return 0;
        }
        auto range = xrange(EmptySlots_.size());
        return *LowerBoundBy(range.begin(), range.end(), slot, [this](ui32 i) {
            return EmptySlots_[i];
        });
    }

    TMaybe<ui32> GetItemPos(size_t item) const {
        if (item >= Items_.size()) {
            return Nothing();
        }

        TItem hash = Items_[item];
        TArithmeticProgression progression = BucketParams_[hash.Hash.BucketId];
        ui32 slot = CalculateItemSlot(NumSlots_, hash.Hash.Item, progression);
        slot -= NumHolesBeforeSlot(slot);
        return slot;
    }

    ui32 NumPositions() const {
        return NumSlots_ - EmptySlots_.size();
    }

private:
    template <typename It>
    bool TryBuild(It begin, It end) {
        if (!PlaceElementsIntoBuckets(begin, end)) {
            Y_ENSURE(false, "Failed to place elements to buckets without collisions");
        }

        SortBucketsBySize();
        RemoveEmptyBuckets();

        if (!TryChooseArithmeticProgressionsParams()) {
            return false;
        }

        SelectEmptySlots();
        return true;
    }

    template <typename It>
    bool PlaceElementsIntoBuckets(It begin, It end) {
        for (ui32 _ : xrange(NumSeedSelectionIters)) {
            Y_UNUSED(_);

            Reset();

            TVector<TItem> items = CalculateElementsHashes(begin, end);
            BuildBuckets();

            if (!TryAssignElementsToBuckets(items)) {
                continue;
            }
            return true;
        }

        return false;
    }

    void Reset() {
        Seed_ = SelectNextSeed();

        Buckets_.assign(NumBuckets_, TBucket{});
        BucketParams_.assign(NumBuckets_, TArithmeticProgression<ui32>{});
        Items_.assign(NumKeys_, TItem{});
    }

    ui64 SelectNextSeed() {
        return SeedRng_.GenRand64();
    }

    template <typename It>
    TVector<TItem> CalculateElementsHashes(It begin, It end) {
        TBuffer keyBuffer{KeySerializer::MaxSize};

        TBaseHash<HasCheckSum> hasher(Seed_, NumSlots_, NumBuckets_);
        TVector<TItem> hashes(Reserve(NumKeys_));

        for (It it = begin; it != end; ++it) {
            size_t len = KeySerializer::Serialize(*it, keyBuffer);

            TBaseHashValue hash = hasher(TStringBuf{ keyBuffer.data(), len });
            hashes.push_back(TItem{hash, static_cast<ui32>(it - begin), 0});
            ++Buckets_[hash.BucketId].Size;
        }

        Y_ENSURE(hashes.size() == NumKeys_);

        return hashes;
    }

    void BuildBuckets() {
        ui32 offset = 0;
        for (auto [i, bucket] : Enumerate(Buckets_)) {
            bucket.Id = i;
            bucket.Offset = offset;
            offset += bucket.Size;
            bucket.Size = 0;
        }
    }

    // Try to place elements to corresponding buckets
    // Checks that there is no collisions in buckets
    bool TryAssignElementsToBuckets(TConstArrayRef<TItem> items) {
        for (const TItem item : items) {
            const THash hash = item.Hash;
            TBucket& bucket = Buckets_[hash.BucketId];

            for (ui32 i = 0; i < bucket.Size; ++i) {
                if (Items_[bucket.Offset + i].Hash.Item.AsUi64() == hash.Item.AsUi64()) {
                    return false;
                }
            }

            Items_[bucket.Offset + bucket.Size] = item;
            ++bucket.Size;
        }

        return true;
    }

    void SortBucketsBySize() {
        Sort(Buckets_, [](TBucket lhs, TBucket rhs) {
            return lhs.Size > rhs.Size;
        });
    }

    void RemoveEmptyBuckets() {
        EraseIf(Buckets_, [](TBucket bucket) {
            return bucket.Size == 0;
        });
    }

    // Try to select arithmetic progressions for each bucket
    bool TryChooseArithmeticProgressionsParams() {
        TBitVector<ui64>(NumSlots_).Swap(UsedSlots_);
        return AllOf(Buckets_, [this](TBucket bucket) {
            return TrySelectParamsForOneBucket(bucket);
        });
    }

    bool TrySelectParamsForOneBucket(TBucket bucket) {
        TSmallVec<ui32> slots(bucket.Size);

        for (auto [i, ab] : Enumerate(CartesianProduct(xrange(NumSlots_), xrange(NumSlots_)))) {
            if (i >= MaxProbeIter_) {
                break;
            }

            auto [a, b] = ab;
            TArithmeticProgression<ui32> progression{a, b};
            if (TrySetParamsForBucket(bucket, progression, slots)) {
                BucketParams_[bucket.Id] = progression;
                return true;
            }
        }

        return false;
    }

    bool TrySetParamsForBucket(TBucket bucket, TArithmeticProgression<ui32> progression, TArrayRef<ui32> slots) {
        for (ui32 i : xrange(bucket.Size)) {
            ui32 slot = CalculateItemSlot(NumSlots_, Items_[bucket.Offset + i].Hash.Item, progression);
            // Fast read-only pass
            if (UsedSlots_.Test(slot)) {
                return false;
            }
            slots[i] = slot;
        }

        for (ui32 i : xrange(bucket.Size)) {
            ui32 slot = slots[i];
            Items_[bucket.Offset + i].Slot = slot;
            if (!UsedSlots_.Set(slot)) {
                // Rollback previously placed items for that bucket
                for (ui32 j : xrange(i)) {
                    UsedSlots_.Reset(slots[j]);
                }
                return false;
            }
        }

        return true;
    }

    void SelectEmptySlots() {
        for (ui32 slot : xrange(NumSlots_)) {
            if (!UsedSlots_.Test(slot)) {
                EmptySlots_.push_back(slot);
            }
        }
    }

    static ui32 CalcNumSlots(ui32 numKeys, double loadFactor) {
        ui32 n = static_cast<ui32>(numKeys / loadFactor) + 1;
        n = IsPrime(n) ? n : NextPrime(n);
        return n;
    }

private:
    TReallyFastRng32 SeedRng_;

    ui64 Seed_ = 0;
    ui32 NumKeys_ = 0;
    ui32 NumSlots_ = 0;
    ui32 NumBuckets_ = 0;
    ui32 MaxProbeIter_ = 0;
    ui32 MaxBucketSize_ = 0;

    TVector<TItem> Items_;
    TVector<TBucket> Buckets_;
    TBitVector<ui64> UsedSlots_;

    TVector<ui32> EmptySlots_;
    TVector<TArithmeticProgression<ui32>> BucketParams_;
};

} // namespace NOffroad::NMinHash
