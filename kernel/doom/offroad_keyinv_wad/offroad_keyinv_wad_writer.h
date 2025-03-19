#pragma once

#include <kernel/doom/wad/mega_wad_writer.h>

#include "key_data_factory.h"

namespace NDoom {


template <class KeyWriter, class HitWriter, class KeyDataAccessor>
class TOffroadKeyInvWadWriter {
    using TKeyWriter = KeyWriter;
    using THitWriter = HitWriter;
    using TDataFactory = TKeyDataFactory<KeyDataAccessor>;

    static_assert(THitWriter::Stages == TKeyWriter::Stages, "Expected same number of stages.");

public:
    using TKey = typename TKeyWriter::TKey;
    using TKeyRef = typename TKeyWriter::TKeyRef;
    using TKeyData = typename TKeyWriter::TKeyData;
    using TKeyModel = typename TKeyWriter::TModel;
    using TKeyTable = typename TKeyWriter::TTable;

    using THit = typename THitWriter::THit;
    using THitModel = typename THitWriter::TModel;
    using THitTable = typename THitWriter::TTable;
    using THitPosition = typename THitWriter::TPosition;

    enum {
        Stages = THitWriter::Stages,
        HitLayers = TDataFactory::Layers
    };

    TOffroadKeyInvWadWriter() {}

    template <class... Args>
    TOffroadKeyInvWadWriter(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    ~TOffroadKeyInvWadWriter() {
        Finish();
    }

    void Reset(const THitModel& hitModel, const TKeyModel& keyModel, TString path, ui32 keyBufferSize = 1 << 17, ui32 invBufferSize = 1 << 17) {
        if (!path.EndsWith('.')) {
            path.push_back('.');
        }

        LocalKeyWriter_.Reset(new TMegaWadWriter(path + "key.wad", keyBufferSize));
        LocalHitWriter_.Reset(new TMegaWadWriter(path + "inv.wad", invBufferSize));

        KeyWriter_.Reset(keyModel, LocalKeyWriter_.Get());
        HitWriter_.Reset(hitModel, LocalHitWriter_.Get());

        PrepareNextKey();
    }

    void Reset(const THitModel& hitModel, IWadWriter* hitWriter, const TKeyModel& keyModel, IWadWriter* keyWriter) {
        LocalKeyWriter_.Reset();
        LocalHitWriter_.Reset();

        KeyWriter_.Reset(keyModel, keyWriter);
        HitWriter_.Reset(hitModel, hitWriter);

        PrepareNextKey();
    }

    void WriteHit(const THit& hit) {
        HitWriter_.WriteHit(hit);
        DataFactory_.AddHit();
    }

    void WriteLayer() {
        HitWriter_.WriteSeekPoint();
        DataFactory_.WriteLayer(HitWriter_.Position());
    }

    void WriteKey(const TKeyRef& key) {
        DataFactory_.FinishLayers(HitWriter_.Position());
        if (DataFactory_.HasNonEmptyLayer()) {
            KeyWriter_.WriteKey(key, DataFactory_.Data());
        }
        PrepareNextKey();
    }

    bool IsFinished() const {
        return HitWriter_.IsFinished();
    }

    void Finish() {
        if (IsFinished()) {
            return;
        }

        HitWriter_.Finish();
        KeyWriter_.Finish();

        if (LocalKeyWriter_) {
            LocalKeyWriter_->Finish();
            LocalKeyWriter_.Reset();
        }
        if (LocalHitWriter_) {
            LocalHitWriter_->Finish();
            LocalHitWriter_.Reset();
        }
    }

private:
    void PrepareNextKey() {
        HitWriter_.WriteSeekPoint();
        DataFactory_.Reset(HitWriter_.Position());
    }

    THolder<IWadWriter> LocalKeyWriter_;
    THolder<IWadWriter> LocalHitWriter_;

    TKeyWriter KeyWriter_;
    THitWriter HitWriter_;

    TDataFactory DataFactory_;
};


} // namespace NDoom
