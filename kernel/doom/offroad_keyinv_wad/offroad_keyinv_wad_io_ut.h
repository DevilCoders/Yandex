#pragma once

#include "offroad_keyinv_wad_io.h"

#include <kernel/doom/offroad/panther_hit_adaptors.h>
#include <kernel/doom/offroad_inv_wad/offroad_inv_wad_io.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_io.h>

#include <library/cpp/offroad/offset/data_offset.h>

#include <util/generic/hash_set.h>

#include <random>

using namespace NDoom;
using namespace NOffroad;

struct TPantherHitsRange {
    TDataOffset Start;
    TDataOffset End;

    TPantherHitsRange() = default;

    TPantherHitsRange(const TDataOffset& start, const TDataOffset& end)
        : Start(start)
        , End(end)
    {
    }
};

struct TPantherHitsRangeVectorizer {
    enum {
        TupleSize = 2
    };

    template <class Slice>
    Y_FORCE_INLINE static void Scatter(const TPantherHitsRange& range, Slice&& slice) {
        const ui64 end = range.End.ToEncoded();
        slice[0] = (end >> 32);
        slice[1] = static_cast<ui32>(end);
    }

    template <class Slice>
    Y_FORCE_INLINE static void Gather(Slice&& slice, TPantherHitsRange* range) {
        const ui64 end = (static_cast<ui64>(slice[0]) << 32) | slice[1];
        range->End = TDataOffset::FromEncoded(end);
    }
};

using TPantherHitsRangeSubtractor = NOffroad::TD1I1Subtractor;

struct TPantherHitsRangeSerializer {
    enum {
        MaxSize = TUi64VarintSerializer::MaxSize
    };

    Y_FORCE_INLINE static size_t Serialize(const TPantherHitsRange& range, ui8* dst) {
        return TUi64VarintSerializer::Serialize(range.End.ToEncoded(), dst);
    }

    Y_FORCE_INLINE static size_t Deserialize(const ui8* src, TPantherHitsRange* range) {
        ui64 tmp;
        size_t size = TUi64VarintSerializer::Deserialize(src, &tmp);
        range->End = TDataOffset::FromEncoded(tmp);
        return size;
    }
};

struct TPantherHitsRangeCombiner {
    static constexpr bool IsIdentity = false;

    Y_FORCE_INLINE static void Combine(const TPantherHitsRange& prev, const TPantherHitsRange& next, TPantherHitsRange* res) {
        *res = TPantherHitsRange(prev.End, next.End);
    }
};

struct TPantherHitsRangeAccessor {
    using TData = TPantherHitsRange;
    using TPosition = TDataOffset;

    enum {
        Layers = 1
    };

    Y_FORCE_INLINE static void AddHit(size_t, TPantherHitsRange*) {

    }

    Y_FORCE_INLINE static TDataOffset Position(const TPantherHitsRange& range, size_t layer) {
        if (layer == 0) {
            return range.Start;
        } else {
            return range.End;
        }
    }

    Y_FORCE_INLINE static void SetPosition(size_t layer, const TDataOffset& pos, TPantherHitsRange* range) {
        if (layer == 0) {
            range->Start = pos;
        } else {
            range->End = pos;
        }
    }
};

TVector<std::pair<TString, TVector<TPantherHit>>> Generate() {
    std::minstd_rand rand(42);

    const size_t size = (1 << 10);
    THashSet<TString> keysSet;
    while (keysSet.size() < size) {
        const size_t len = 1 + rand() % 8;
        TString current;
        current.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            current.push_back('a' + rand() % 3);
        }
        keysSet.insert(std::move(current));
    }

    TVector<TString> keys(keysSet.begin(), keysSet.end());
    Sort(keys);

    TVector<std::pair<TString, TVector<TPantherHit>>> res(size);
    for (size_t i = 0; i < size; ++i) {
        res[i].first = std::move(keys[i]);
        TVector<TPantherHit>& hits = res[i].second;
        const size_t count = rand() % 100;
        for (size_t j = 0; j < count; ++j) {
            const ui32 docId = rand() % 42;
            const ui32 relevance = rand() % 42;
            hits.emplace_back(docId, relevance);
        }
        Sort(hits);
        hits.erase(Unique(hits.begin(), hits.end()), hits.end());
    }

    return res;
}

class TIndexGenerator {
public:
    using TKeyIo = TOffroadKeyWadIo<PantherIndexType, TPantherHitsRange, TPantherHitsRangeVectorizer, TPantherHitsRangeSubtractor, TPantherHitsRangeSerializer, TPantherHitsRangeCombiner, NoStandardIoModel>;
    using THitIo = TOffroadInvWadIo<PantherIndexType, TPantherHit, TPantherHitVectorizer, TPantherHitSubtractor, TNullVectorizer, NoStandardIoModel>;
    using TIo = TOffroadKeyInvWadIo<TKeyIo, THitIo, TPantherHitsRangeAccessor>;
    using TSampler = TIo::TSampler;
    using TWriter = TIo::TWriter;
    using TReader = TIo::TReader;
    using THitModel = TIo::THitModel;
    using TKeyModel = TIo::TKeyModel;
    using TIndex = TVector<std::pair<TString, TVector<TPantherHit>>>;

public:
    TIndexGenerator() {
        Index_ = Generate();

        TSampler sampler;
        THitModel hitModel;
        TKeyModel keyModel;

        for (ui32 stage = 0; stage < TSampler::Stages; ++stage) {
            for (const auto& entry : Index_) {
                for (const TPantherHit& hit : entry.second) {
                    sampler.WriteHit(hit);
                }
                sampler.WriteKey(entry.first);
            }
            std::pair<THitModel, TKeyModel> model = sampler.Finish();
            hitModel = std::move(model.first);
            keyModel = std::move(model.second);
        }

        TMegaWadWriter keyWriter(&KeyStream_);
        TMegaWadWriter hitWriter(&HitStream_);
        TWriter writer(hitModel, &hitWriter, keyModel, &keyWriter);
        static_assert(TWriter::Stages == 1, "Expected one stage.");

        for (const auto& entry : Index_) {
            for (const TPantherHit& hit : entry.second) {
                writer.WriteHit(hit);
            }
            writer.WriteKey(entry.first);
        }
        writer.Finish();
        keyWriter.Finish();
        hitWriter.Finish();

        KeyWad_.Reset(IWad::Open(TArrayRef<const char>(KeyStream_.Buffer().Data(), KeyStream_.Buffer().Size())));
        HitWad_.Reset(IWad::Open(TArrayRef<const char>(HitStream_.Buffer().Data(), HitStream_.Buffer().Size())));
    }

    Y_FORCE_INLINE TReader GetReader() const {
        return TReader(HitWad_.Get(), KeyWad_.Get());
    }

    Y_FORCE_INLINE const TIndex& GetIndex() const {
        return Index_;
    }

private:
    TIndex Index_;

    TBufferStream KeyStream_;
    TBufferStream HitStream_;
    THolder<NDoom::IWad> KeyWad_;
    THolder<NDoom::IWad> HitWad_;
};

