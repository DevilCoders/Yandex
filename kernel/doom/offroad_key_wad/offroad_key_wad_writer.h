#pragma once

#include <library/cpp/offroad/key/key_writer.h>
#include <library/cpp/offroad/key/fat_key_writer.h>

#include <kernel/doom/offroad_common/accumulating_output.h>
#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/wad_writer.h>

namespace NDoom {


template <EWadIndexType indexType, class KeyData, class Vectorizer, class Subtractor, class Serializer>
class TOffroadKeyWadWriter {
private:
    using TKeyWriter = NOffroad::TFatKeyWriter<
        NOffroad::TFatOffsetDataWriter<KeyData, Serializer>,
        NOffroad::TKeyWriter<KeyData, Vectorizer, Subtractor>
    >;

public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = KeyData;

    using TTable = typename TKeyWriter::TTable;
    using TModel = typename TKeyWriter::TModel;

    enum {
        Stages = TKeyWriter::Stages
    };

    TOffroadKeyWadWriter() {}

    TOffroadKeyWadWriter(const TModel& model, IWadWriter* writer) {
        Reset(model, writer);
    }

    TOffroadKeyWadWriter(const IWad* modelWad, IWadWriter* writer) {
        Reset(modelWad, writer);
    }

    void Reset(const IWad* modelWad, IWadWriter* writer) {
        TModel model;
        model.Load(modelWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel)));

        Reset(model, writer);
    }

    void Reset(const TModel& model, IWadWriter* writer) {
        if (!LocalTable_)
            LocalTable_ = MakeHolder<TTable>();

        LocalTable_->Reset(model);

        WadWriter_ = writer;

        /* Write out model right away. */
        model.Save(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel)));

        /* Set up key i/o, key data will be written next. */
        FatKeyOutput_.Reset();
        FatKeySubOutput_.Reset();
        IOutputStream* keyOutput = WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::Keys));
        KeyWriter_.Reset(&FatKeyOutput_, &FatKeySubOutput_, LocalTable_.Get(), keyOutput);
    }

    void WriteKey(const TKeyRef& key, const TKeyData& data) {
        KeyWriter_.WriteKey(key, data);
    }

    void Finish() {
        KeyWriter_.Finish();
        FatKeyOutput_.Flush(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyFat)));
        FatKeySubOutput_.Flush(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyIdx)));
    }

    bool IsFinished() const {
        return KeyWriter_.IsFinished();
    }

private:
    THolder<TTable> LocalTable_;
    IWadWriter* WadWriter_ = nullptr;
    TAccumulatingOutput FatKeyOutput_;
    TAccumulatingOutput FatKeySubOutput_;
    TKeyWriter KeyWriter_;
};


} // namespace NDoom
