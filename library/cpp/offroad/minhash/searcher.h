#pragma once

#include "common.h"
#include "header.h"
#include "hit.h"
#include "io.h"

#include <library/cpp/offroad/flat/flat_writer.h>

#include <util/generic/buffer.h>
#include <util/generic/maybe.h>
#include <util/stream/mem.h>

namespace NOffroad::NMinHash {

struct TMinHashStorage {
    TBlob Header;
    TBlob Buckets;
    TBlob EmptySlots;
};

template <typename Key, typename KeySerializer, bool HasCheckSum>
class TMinHashSearcher {
public:
    using TBucketsSearcher = TMinHashIo::TArithmeticProgressionSearcher;
    using TEmptySlotsSearcher = TMinHashIo::TEmptySlotsSearcher;

    using THash = TBaseHashValue<HasCheckSum>;
    using TItem = TItem<HasCheckSum>;

public:
    TMinHashSearcher() = default;

    TMinHashSearcher(const TMinHashStorage& storage) {
        Reset(storage);
    }

    void Reset(const TMinHashStorage& storage) {
        THeader header;
        TMemoryInput input{ storage.Header.Data(), storage.Header.Size() };
        header.Load(&input);

        NumSlots_ = header.NumSlots;
        Hash_.Reset(header.Seed, header.NumSlots, header.NumBuckets);
        BucketsSearcher_.Reset(storage.Buckets);
        EmptySlotsSearcher_.Reset(storage.EmptySlots);
    }

    ui32 Find(const Key& key) const {
        TTempBuf buf{ KeySerializer::MaxSize };
        size_t len = KeySerializer::Serialize(key, TArrayRef<char>{ buf.Data(), buf.Size() });

        return FindRaw(TStringBuf{ buf.Data(), len });
    }

    TArithmeticProgressionHit GetBucket(ui32 index) const {
        Y_ASSERT(index < BucketsSearcher_.Size());
        return BucketsSearcher_.ReadKey(index);
    }

    ui32 NumHolesBeforeSlot(ui32 slot) const {
        auto getSlot = [this](size_t i) {
            return EmptySlotsSearcher_.ReadKey(i);
        };
        auto range = xrange<size_t>(0u, EmptySlotsSearcher_.Size());

        return *LowerBoundBy(range.begin(), range.end(), slot, getSlot);
    }

    ui32 FindRaw(TStringBuf serializedKeyRef) const {
        TBaseHashValue hash = Hash_(serializedKeyRef);

        TArithmeticProgressionHit hit = GetBucket(hash.BucketId);
        TArithmeticProgression<ui32> progression{ .Init = hit.Init, .Diff = hit.Diff };

        ui32 slot = CalculateItemSlot(NumSlots_, hash.Item, progression);

        if (EmptySlotsSearcher_.Size() > 0) {
            ui32 numHolesBeforeSlot = NumHolesBeforeSlot(slot);
            slot -= numHolesBeforeSlot;
        }

        if (slot > Size()) {
            return 0;
        }
        return slot;
    }

    ui32 Size() const {
        return NumSlots_ - EmptySlotsSearcher_.Size();
    }

private:
    ui32 NumSlots_ = 0;
    TBaseHash<HasCheckSum> Hash_;

    TBucketsSearcher BucketsSearcher_;
    TEmptySlotsSearcher EmptySlotsSearcher_;
};

}
