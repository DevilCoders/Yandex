#pragma once

#include "doc_chunk_mapping.h"

#include <kernel/doom/wad/mega_wad_writer.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/flat/flat_writer.h>


namespace NDoom {


class TDocChunkMappingWriter {
    using TWriter = NOffroad::TFlatWriter<TDocChunkMapping, std::nullptr_t, TDocChunkMappingVectorizer, NOffroad::TNullVectorizer>;
public:
    TDocChunkMappingWriter() = default;

    TDocChunkMappingWriter(const TString& path) {
        Reset(path);
    }

    TDocChunkMappingWriter(TMegaWadWriter* writer) {
        Reset(writer);
    }

    void Reset(const TString& path) {
        LocalMegaWadWriter_.Reset(new TMegaWadWriter(path));
        ResetInternal(LocalMegaWadWriter_.Get());
    }

    void Reset(TMegaWadWriter* writer) {
        LocalMegaWadWriter_.Reset();
        ResetInternal(writer);
    }

    void Write(const TDocChunkMapping& docChunkMapping) {
        Writer_.Write(docChunkMapping, nullptr);
    }

    bool IsFinished() const {
        return Writer_.IsFinished();
    }

    void Finish() {
        if (IsFinished()) {
            return;
        }
        Writer_.Finish();
        if (LocalMegaWadWriter_) {
            LocalMegaWadWriter_->Finish();
            LocalMegaWadWriter_.Reset();
        }
    }

private:
    void ResetInternal(TMegaWadWriter* writer) {
        Writer_.Reset(writer->StartGlobalLump(TWadLumpId(ChunkMappingIndexType, EWadLumpRole::Hits)));
    }


    THolder<TMegaWadWriter> LocalMegaWadWriter_;
    TWriter Writer_;
};


} // namespace NDoom
