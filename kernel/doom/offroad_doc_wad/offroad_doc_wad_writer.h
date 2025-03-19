#pragma once

#include <library/cpp/offroad/tuple/adaptive_tuple_writer.h>

#include <kernel/doom/offroad_common/accumulating_output.h>
#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/wad.h>

#include <util/generic/size_literals.h>

#include "offroad_doc_wad_mapping.h"
#include "offroad_doc_codec.h"
#include "offroad_doc_common.h"

namespace NDoom {


template<EWadIndexType indexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer, EOffroadDocCodec codec = AdaptiveDocCodec>
class TOffroadDocWadWriter {
public:
    using TWriter = typename TOffroadDocCommon<codec, Hit, Vectorizer, Subtractor, PrefixVectorizer>::TWriter;
    using TMapping = TOffroadDocWadMapping<indexType, PrefixVectorizer>;
    using THit = Hit;
    using TTable = typename TWriter::TTable;
    using TModel = typename TWriter::TModel;

    enum {
        Stages = TWriter::Stages
    };

    TOffroadDocWadWriter() {}

    template <class... Args>
    TOffroadDocWadWriter(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const IWad* modelWad, IWadWriter* writer) {
        LocalWadWriter_.Reset();

        TModel model;
        model.Load(modelWad->LoadGlobalLump(Mapping_.ModelLump()));

        Reset(model, writer);
    }

    void Reset(const TModel& model, const TString& path, ui32 bufferSize = 128_KB) {
        LocalWadWriter_ = MakeHolder<TMegaWadWriter>(path, bufferSize);
        Reset(model, LocalWadWriter_.Get());
    }

    void Reset(const TModel& model, IWadWriter* writer) {
        if (!LocalTable_)
            LocalTable_ = MakeHolder<TTable>();
        LocalTable_->Reset(model);

        /* Write out models right away. */
        model.Save(writer->StartGlobalLump(Mapping_.ModelLump()));

        Reset(LocalTable_.Get(), writer);
    }

    void Reset(const TTable* table, IWadWriter* writer) {
        Table_ = table;

        DocLumps_ = Mapping_.DocLumps();
        for (size_t i = 0; i < DocOutputs_.size(); i++) {
            DocOutputs_[i].Reset();
            DocOutputPointers_[i] = &DocOutputs_[i];
        }

        WadWriter_ = writer;
        WadWriter_->RegisterDocLumpTypes(DocLumps_);

        ResetHitWriter();
        Finished_ = false;
    }

    void WriteHit(const THit& hit) {
        Writer_.WriteHit(hit);
    }

    void WriteDoc(ui32 docId) {
        Writer_.Finish();

        for (size_t i = 0; i < DocLumps_.size(); i++)
            DocOutputs_[i].Flush(WadWriter_->StartDocLump(docId, DocLumps_[i]));

        ResetHitWriter();
    }

    void Finish() {
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
    void ResetHitWriter() {
        Mapping_.ResetStream(&Writer_, Table_, DocOutputPointers_);
    }

private:
    THolder<IWadWriter> LocalWadWriter_;
    THolder<TTable> LocalTable_;
    TMapping Mapping_;

    const TTable* Table_ = nullptr;
    IWadWriter* WadWriter_;

    std::array<TWadLumpId, TMapping::DocLumpCount> DocLumps_;
    std::array<TAccumulatingOutput, TMapping::DocLumpCount> DocOutputs_;
    std::array<IOutputStream*, TMapping::DocLumpCount> DocOutputPointers_;
    TWriter Writer_;

    bool Finished_ = false;
};


} // namespace NDoom


