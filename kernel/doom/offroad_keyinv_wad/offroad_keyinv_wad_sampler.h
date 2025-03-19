#pragma once

#include <kernel/doom/wad/mega_wad_writer.h>

#include "key_data_factory.h"

#include <util/stream/null.h>

namespace NDoom {


template <class KeySampler, class HitSampler, class HitWriter, class KeyDataAccessor>
class TOffroadKeyInvWadSampler {
    using TKeySampler = KeySampler;
    using THitSampler = HitSampler;
    using THitWriter = HitWriter;
    using TDataFactory = TKeyDataFactory<KeyDataAccessor>;

public:
    using TKey = typename TKeySampler::TKey;
    using TKeyRef = typename TKeySampler::TKeyRef;
    using TKeyData = typename TKeySampler::TKeyData;
    using TKeyModel = typename TKeySampler::TModel;

    using THit = typename THitSampler::THit;
    using THitModel = typename THitSampler::TModel;
    using THitPosition = typename THitWriter::TPosition;

    enum {
        Stages = THitSampler::Stages + TKeySampler::Stages,
        HitLayers = TDataFactory::Layers
    };

    TOffroadKeyInvWadSampler() {}

    void Reset() {
        Stage_ = 0;
        HitSampler_.Reset();
    }

    void WriteHit(const THit& hit) {
        if (Stage_ < THitSampler::Stages) {
            HitSampler_.WriteHit(hit);
        } else {
            HitWriter_.WriteHit(hit);
            DataFactory_.AddHit();
        }
    }

    void WriteLayer() {
        if (Stage_ < THitSampler::Stages) {
            HitSampler_.WriteSeekPoint();
        } else {
            HitWriter_.WriteSeekPoint();
            DataFactory_.WriteLayer(HitWriter_.Position());
        }
    }

    void WriteKey(const TKeyRef& key) {
        if (Stage_ < THitSampler::Stages) {
            HitSampler_.WriteSeekPoint();
        } else {
            DataFactory_.FinishLayers(HitWriter_.Position());
            if (DataFactory_.HasNonEmptyLayer()) {
                KeySampler_.WriteKey(key, DataFactory_.Data());
            }
            PrepareNextKey();
        }
    }

    bool IsFinished() const {
        return Stage_ == Stages;
    }

    std::pair<THitModel, TKeyModel> Finish() {
        if (IsFinished()) {
            return std::make_pair(THitModel(), TKeyModel());
        }

        if (Stage_ < THitSampler::Stages) {
            HitModel_ = HitSampler_.Finish();
            ++Stage_;
            if (Stage_ == THitSampler::Stages) {
                KeySampler_.Reset();
                PrepareNextKeyStage();
            }
        } else {
            KeyModel_ = KeySampler_.Finish();
            ++Stage_;
            if (Stage_ == Stages) {
                return std::make_pair(std::move(HitModel_), std::move(KeyModel_));
            }
            PrepareNextKeyStage();
        }

        return std::make_pair(THitModel(), TKeyModel());
    }

private:
    void PrepareNextKey() {
        HitWriter_.WriteSeekPoint();
        DataFactory_.Reset(HitWriter_.Position());
    }

    void PrepareNextKeyStage() {
        MegaWadWriter_.Reset(&Cnull);
        HitWriter_.Reset(HitModel_, &MegaWadWriter_);
        PrepareNextKey();
    }


    THitSampler HitSampler_;
    THitModel HitModel_;
    TMegaWadWriter MegaWadWriter_;
    THitWriter HitWriter_;

    TKeySampler KeySampler_;
    TKeyModel KeyModel_;
    TDataFactory DataFactory_;

    ui32 Stage_ = 0;
};


} // namespace NDoom
