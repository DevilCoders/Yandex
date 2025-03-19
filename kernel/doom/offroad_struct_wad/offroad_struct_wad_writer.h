#pragma once

#include "compression_type.h"
#include "struct_type.h"
#include "struct_writer.h"

#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/byte_stream/byte_output_stream.h>
#include <library/cpp/offroad/byte_stream/raw_output_stream.h>


namespace NDoom {

/*
 * @param Serializer must implement either:
        - ui32 Serializer::Serialize(const TData data, IOutputStream*) // used in (VariableSizeStructType, OffroadCompressionType) and FixedSizeStructType
        - void Serializer::Serialize(const TData data, IOutputStream*) // used in other cases
 */

template <EWadIndexType indexType, class Data, class Serializer, EStructType structType, ECompressionType compressionType>
class TOffroadStructWadWriter {
    using TWriter = TStructWriter<indexType, Data, Serializer, structType, compressionType>;

public:
    using THit = Data;
    using TTable = typename TWriter::TTable;
    using TModel = typename TTable::TModel;

    TOffroadStructWadWriter() {}

    template <class... Args>
    TOffroadStructWadWriter(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const IWad* modelWad, IWadWriter* writer) {
        LocalWadWriter_.Reset();

        TModel model;
        model.Load(modelWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructModel)));

        Reset(model, writer);
    }

    void Reset(const TModel& model, const TString& path) {
        LocalWadWriter_ = MakeHolder<TMegaWadWriter>(path);
        Reset(model, LocalWadWriter_.Get());
    }

    void Reset(const TModel& model, IWadWriter* writer) {
        if (!LocalTable_)
            LocalTable_ = MakeHolder<TTable>();

        /* Write out models right away. */
        model.Save(writer->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructModel)));

        LocalTable_->Reset(model);
        Reset(LocalTable_.Get(), writer);
    }

    void Reset(const TTable* table, IWadWriter* writer) {
        Table_ = table;

        WadWriter_ = writer;
        WadWriter_->RegisterDocLumpType(TWadLumpId(indexType, EWadLumpRole::Struct));

        Finished_ = false;
        AccumulatingOutput_.Reset();

        Writer_.Reset(Table_);
    }

    void WriteDoc(ui32 docId) {
        AccumulatingOutput_.Flush(WadWriter_->StartDocLump(docId, TWadLumpId(indexType, EWadLumpRole::Struct)));
    }

    void WriteHit(const THit& data) {
        Writer_.Write(data, &AccumulatingOutput_);
    }

    void Write(ui32 docId, const THit& data) {
        WriteHit(data);
        WriteDoc(docId);
    }

    void Finish() {
        if (IsFinished()) {
            return;
        }

        Writer_.Finish(WadWriter_);
        if (LocalWadWriter_) {
            LocalWadWriter_->Finish();
            LocalWadWriter_.Reset();
        }
        Finished_ = true;
    }

    bool IsFinished() const {
        return Finished_;
    }

private:
    THolder<IWadWriter> LocalWadWriter_;
    THolder<TTable> LocalTable_;

    const TTable* Table_ = nullptr;
    IWadWriter* WadWriter_;

    TAccumulatingOutput AccumulatingOutput_;

    TWriter Writer_;

    bool Finished_ = false;
};


} // namespace NDoom
