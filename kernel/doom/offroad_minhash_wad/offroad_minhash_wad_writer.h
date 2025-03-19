#pragma once

#include "flat_key_storage.h"

#include <kernel/doom/wad/mega_wad_writer.h>

#include <library/cpp/offroad/flat/flat_writer.h>
#include <library/cpp/offroad/minhash/writer.h>

#include <util/stream/input.h>

namespace NDoom {

template <typename T, typename Key, typename Data>
concept MinHashKeyValueWriter = requires (T& t, const Key& key, const Data& data, IWadWriter* wadWriter) {
    t.Write(key, data);
    t.Finish();
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeySerializer, MinHashKeyValueWriter<Key, Data> KeyWriter>
class TOffroadMinHashWadWriter {
public:
    inline static constexpr EWadIndexType IndexType = indexType;

    using TMinHashBuilder = typename NOffroad::NMinHash::TMinHashBuilder<Key, KeySerializer, /* HasCheckSum */ false>;
    using TMinHashWriter = typename NOffroad::NMinHash::TMinHashWriter;
    using TKeyWriter = KeyWriter;
    using TKeyWriterModel = typename KeyWriter::TKeyModel;

public:
    TOffroadMinHashWadWriter(TKeyWriterModel model, IWadWriter* wadWriter) {
        Reset(std::move(model), wadWriter);
    }

    void Reset(TKeyWriterModel model, IWadWriter* wadWriter) {
        KeyWriterModel_ = std::move(model);
        WadWriter_ = wadWriter;
    }

    void Reserve(size_t numKeys) {
        Keys_.reserve(numKeys);
        Values_.reserve(numKeys);
    }

    void WriteKey(const Key& key, const Data& data) {
        Keys_.push_back(key);
        Values_.push_back(data);
    }

    void Finish(NOffroad::NMinHash::TMinHashParams params = {}) {
        params.NumKeys = Keys_.size();

        TMinHashBuilder builder{ params };
        builder.Build(Keys_);

        WriteMinHash(builder);
        WriteKeys(builder);
    }

private:
    void WriteMinHash(const TMinHashBuilder& builder) {
        TAccumulatingOutput headerOutput;
        TAccumulatingOutput emptySlotsOutput;

        NOffroad::NMinHash::TOutputStreams minHashOutput;
        minHashOutput.Buckets = WadWriter_->StartGlobalLump(LumpId(EWadLumpRole::Hits));
        minHashOutput.EmptySlots = &emptySlotsOutput;
        minHashOutput.Header = &headerOutput;

        TMinHashWriter writer{ minHashOutput };
        builder.Finish(&writer);

        emptySlotsOutput.Flush(WadWriter_->StartGlobalLump(LumpId(EWadLumpRole::Struct)));
        headerOutput.Flush(WadWriter_->StartGlobalLump(LumpId(EWadLumpRole::StructSub)));
    }

    void WriteKeys(const TMinHashBuilder& builder) {
        TVector<ui32> keyByPos(builder.NumPositions(), Max<ui32>());
        for (auto [i, item] : Enumerate(builder.Items())) {
            ui32 pos = item.Slot - builder.NumHolesBeforeSlot(item.Slot);
            Y_ASSERT(keyByPos[pos] == Max<ui32>());
            keyByPos[pos] = item.KeyId;

#ifndef NDEBUG
            TMaybe<ui32> expected = builder.GetItemPos(i);
            Y_ASSERT(expected == pos);
#endif
        }

        TKeyWriter writer{KeyWriterModel_, WadWriter_};
        for (ui32 i : keyByPos) {
            writer.Write(Keys_[i], Values_[i]);
        }
        writer.Finish();
    }

private:
    static TWadLumpId LumpId(EWadLumpRole role) {
        return TWadLumpId{ IndexType, role };
    }

private:
    IWadWriter* WadWriter_ = nullptr;
    TKeyWriterModel KeyWriterModel_;

    TVector<Key> Keys_;
    TVector<Data> Values_;
};

} // namespace NDoom
