#pragma once

#include <kernel/doom/wad/mega_wad_writer.h>

#include <library/cpp/offroad/codec/encoder_64.h>
#include <library/cpp/offroad/codec/interleaved_encoder.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/flat/flat_writer.h>

#include "struct_diff_common.h"
#include "struct_diff_output_buffer.h"


namespace NDoom {


template <EWadIndexType indexType, class Key, class KeyVectorizer, class KeySubtractor, class KeyPrefixVectorizer, class Data>
class TOffroadStructDiffWadWriter {
    using TBuffer = TStructDiffOutputBuffer<NOffroad::TEncoder64::BlockSize, Key, KeyVectorizer, KeySubtractor, Data>;
public:
    enum {
        BlockSize = TBuffer::BlockSize,
        Stages = NOffroad::TEncoder64::Stages
    };

    using TKey = Key;
    using TData = Data;
    using TModel = TStructDiffModel<KeyVectorizer::TupleSize, sizeof(TData), NOffroad::TEncoder64::TTable::TModel>;
    using TTable = TStructDiffTable<KeyVectorizer::TupleSize, sizeof(TData), NOffroad::TEncoder64::TTable>;

    TOffroadStructDiffWadWriter(const IWad* modelWad, IWadWriter* writer, bool use64BitSubWriter = false) {
        Reset(modelWad, writer, use64BitSubWriter);
    }

    TOffroadStructDiffWadWriter(const TModel& model, IWadWriter* writer, bool use64BitSubWriter = false) {
        Reset(model, writer, use64BitSubWriter);
    }

    TOffroadStructDiffWadWriter(const TModel& model, const TString& path, bool use64BitSubWriter = false) {
        Reset(model, path, use64BitSubWriter);
    }

    void Reset(const IWad* modelWad, IWadWriter* writer, bool use64BitSubWriter = false) {
        LocalWadWriter_.Reset();

        TModel model;
        model.Load(modelWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructModel)));
        Reset(model, writer, use64BitSubWriter);
    }

    void Reset(const TModel& model, const TString& path, bool use64BitSubWriter = false) {
        LocalWadWriter_ = MakeHolder<TMegaWadWriter>(path);
        Reset(model, LocalWadWriter_.Get(), use64BitSubWriter);
    }

    void Reset(const TModel& model, IWadWriter* writer, bool use64BitSubWriter = false) {
        if (!LocalTable_)
            LocalTable_ = MakeHolder<TTable>();

        LocalTable_->Reset(model);

        WadWriter_ = writer;

        model.Save(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructModel)));

        AccumulatingSubOutput_.Reset();
        Use64BitSubWriter_ = use64BitSubWriter;
        if (Use64BitSubWriter_) {
            SubWriter64_.Reset(&AccumulatingSubOutput_);

            const ui32 offsetBytes = sizeof(ui64);
            writer->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel))->Write(&offsetBytes, sizeof(offsetBytes));
        } else {
            SubWriter32_.Reset(&AccumulatingSubOutput_);
        }

        Output_.Reset(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::Struct)));
        Encoder_.Reset(LocalTable_.Get(), &Output_);

        Buffer_.Reset();
        Finished_ = false;
    }

    void Write(const TKey& key, const TData* data) {
        Buffer_.Write(key, data);

        if (Buffer_.IsDone()) {
            Buffer_.Flush(&Encoder_);
            if (Use64BitSubWriter_) {
                SubWriter64_.Write(key, Output_.Position());
            } else {
                Y_ENSURE(Output_.Position() <= Max<ui32>());
                SubWriter32_.Write(key, Output_.Position());
            }
        }
    }

    bool IsFinished() const {
        return Finished_;
    }

    void Finish() {
        if (IsFinished()) {
            return;
        }

        Buffer_.Flush(&Encoder_);
        Output_.Finish();
        if (Use64BitSubWriter_) {
            SubWriter64_.Finish();
        } else {
            SubWriter32_.Finish();
        }
        const ui32 structSize = sizeof(Data);
        AccumulatingSubOutput_.Flush(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSub)));
        WadWriter_->WriteGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize), TArrayRef<const char>((const char*)&structSize, sizeof(structSize)));

        if (LocalWadWriter_) {
            LocalWadWriter_->Finish();
            LocalWadWriter_.Reset();
        }
        Finished_ = true;
    }

private:
    bool Finished_ = false;
    THolder<IWadWriter> LocalWadWriter_;
    THolder<TTable> LocalTable_;
    IWadWriter* WadWriter_ = nullptr;

    TAccumulatingOutput AccumulatingSubOutput_;
    NOffroad::TVecOutput Output_;
    NOffroad::TInterleavedEncoder<TTable::TupleSize, NOffroad::TEncoder64> Encoder_;
    bool Use64BitSubWriter_ = false;
    NOffroad::TFlatWriter<TKey, ui32, KeyPrefixVectorizer, NOffroad::TUi32Vectorizer> SubWriter32_;
    NOffroad::TFlatWriter<TKey, ui64, KeyPrefixVectorizer, NOffroad::TUi64Vectorizer> SubWriter64_;
    TBuffer Buffer_;
};


} // namespace NDoom
