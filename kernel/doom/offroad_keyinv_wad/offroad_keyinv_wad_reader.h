#pragma once

#include <kernel/doom/progress/progress.h>
#include <kernel/doom/wad/wad.h>

namespace NDoom {


template <class KeyReader, class HitReader, class KeyDataAccessor>
class TOffroadKeyInvWadReader {
    using TKeyReader = KeyReader;
    using THitReader = HitReader;

public:
    using TKey = typename TKeyReader::TKey;
    using TKeyRef = typename TKeyReader::TKeyRef;
    using TKeyData = typename TKeyReader::TKeyData;
    using TKeyModel = typename TKeyReader::TModel;
    using TKeyTable = typename TKeyReader::TTable;

    using THit = typename THitReader::THit;
    using THitModel = typename THitReader::TModel;
    using THitTable = typename THitReader::TTable;

    enum {
        HasLowerBound = false,
        HitLayers = KeyDataAccessor::Layers
    };

    TOffroadKeyInvWadReader() {

    }

    TOffroadKeyInvWadReader(const TString& path) {
        Reset(path);
    }

    TOffroadKeyInvWadReader(const IWad* hitWad, const IWad* keyWad) {
        Reset(hitWad, keyWad);
    }

    void Reset(TString path) {
        if (!path.EndsWith('.')) {
            path.push_back('.');
        }

        LocalKeyWad_ = IWad::Open(path + "key.wad");
        LocalHitWad_ = IWad::Open(path + "inv.wad");

        KeyReader_.Reset(LocalKeyWad_.Get());
        HitReader_.Reset(LocalHitWad_.Get());
    }

    void Reset(const IWad* hitWad, const IWad* keyWad) {
        LocalKeyWad_.Reset();
        LocalHitWad_.Reset();

        KeyReader_.Reset(keyWad);
        HitReader_.Reset(hitWad);
    }

    bool ReadKey(TKeyRef* key, TKeyData* data = nullptr) {
        if (!KeyReader_.ReadKey(key, &KeyData_)) {
            return false;
        }
        ExpectFirstHit_ = true;
        ChangeLayer(0);
        if (data) {
            *data = KeyData_;
        }
        return true;
    }

    bool NextLayer() {
        if (ExpectFirstHit_) {
            ExpectFirstHit_ = false;
            return true;
        }
        if (Layer_ + 1 == HitLayers) {
            return false;
        }

        ChangeLayer(Layer_ + 1);
        return true;
    }

    bool ReadHit(THit* hit) {
        ExpectFirstHit_ = false;

        if (LayerIsEmpty_) {
            return false;
        }

        return HitReader_.ReadHit(hit);
    }

    TProgress Progress() const {
        return TProgress();
    }

    void LowerBound(const TKeyRef&) {
        Y_VERIFY(false, "LowerBound unsupported, check HasLowerBound property.");
    }

private:
    void ChangeLayer(const size_t layer) {
        Y_ENSURE(layer < HitLayers);
        Layer_ = layer;
        LayerIsEmpty_ = KeyDataAccessor::Position(KeyData_, Layer_) == KeyDataAccessor::Position(KeyData_, Layer_ + 1);
        if (!LayerIsEmpty_) {
            const bool success = HitReader_.Seek(KeyDataAccessor::Position(KeyData_, Layer_), KeyDataAccessor::Position(KeyData_, Layer_ + 1));
            Y_ENSURE(success);
        }
    }

    THolder<IWad> LocalKeyWad_;
    THolder<IWad> LocalHitWad_;

    TKeyReader KeyReader_;
    THitReader HitReader_;

    TKeyData KeyData_;
    bool ExpectFirstHit_ = false;
    bool LayerIsEmpty_ = false;
    size_t Layer_ = 0;
};


} // namespace NDoom
