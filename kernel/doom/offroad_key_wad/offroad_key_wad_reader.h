#pragma once

#include <kernel/doom/wad/wad_lump_id.h>
#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/key/key_reader.h>

#include "combined_key_data_reader.h"

namespace NDoom {


template <EWadIndexType indexType, class KeyData, class Vectorizer, class Subtractor, class Combiner>
class TOffroadKeyWadReader {
    using TKeyReader = TCombinedKeyDataReader<Combiner, NOffroad::TKeyReader<KeyData, Vectorizer, Subtractor>>;

public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = KeyData;
    using TModel = typename TKeyReader::TModel;
    using TTable = typename TKeyReader::TTable;

    enum {
        HasLowerBound = false
    };

    TOffroadKeyWadReader() {

    }

    TOffroadKeyWadReader(const IWad* wad) {
        Reset(wad);
    }

    void Reset(const IWad* wad) {
        if (!LocalTable_) {
            LocalTable_ = MakeHolder<TTable>();
        }

        TModel model;
        model.Load(wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel)));
        LocalTable_->Reset(model);

        KeysBlob_ = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::Keys));
        KeyReader_.Reset(LocalTable_.Get(), KeysBlob_);
    }

    bool ReadKey(TKeyRef* key, TKeyData* data) {
        return KeyReader_.ReadKey(key, data);
    }

    void LowerBound(const TKeyRef&) {
        Y_VERIFY(false, "LowerBound unsupported, check HasLowerBound property.");
    }

private:
    THolder<TTable> LocalTable_;
    TBlob KeysBlob_;
    TKeyReader KeyReader_;
};


} // namespace NDoom
