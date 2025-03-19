#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_writer.h>
#include <kernel/doom/wad/mega_wad_writer.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/flat/flat_writer.h>

#include <util/generic/maybe.h>

#include <cstddef>

#include <utility>

namespace NDoom {

template <
    EWadIndexType indexType,
    class Hash,
    class HashVectorizer,
    class HashSubtractor,
    EOffroadDocCodec codec,
    size_t blockSize>
class TOffroadBlockWadWriter {
    using TBlockWriter = TOffroadDocWadWriter<indexType, Hash, HashVectorizer, HashSubtractor, NOffroad::TNullVectorizer, codec>;
public:
    using THash = Hash;
    using TModel = typename TBlockWriter::TModel;

    TOffroadBlockWadWriter() = default;

    template<class... Args>
    TOffroadBlockWadWriter(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& subPath, const TModel& blockModel, const TString& blockPath) {
        LocalSubWadWriter_ = MakeHolder<TMegaWadWriter>(subPath);
        LocalBlockWadWriter_ = MakeHolder<TMegaWadWriter>(blockPath);
        Reset(LocalSubWadWriter_.Get(), blockModel, LocalBlockWadWriter_.Get());
    }

    void Reset(IWadWriter* subWriter, const TModel& blockModel, IWadWriter* blockWriter) {
        ResetInternal();
        BlockWadWriter_ = blockWriter;
        SubWriter_.Reset(subWriter->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSub)));
        BlockWriter_.Reset(blockModel, blockWriter);
    }

    void WriteBlock(const THash& hash, ui32* hashId) {
        Y_ENSURE(!LastHash_.Defined() || hash > *LastHash_);
        LastHash_ = hash;
        BlockWriter_.WriteHit(hash);
        Flushed_ = false;
        if (HashId_ % blockSize == blockSize - 1) {
            SubWriter_.Write(nullptr, hash);
            Flushed_ = true;
            Y_ENSURE(BlockId_ != Max<ui32>()); // we don't want too many blocks
            BlockWriter_.WriteDoc(BlockId_++);
        }
        Y_ENSURE(HashId_ != Max<ui32>());
        *hashId = HashId_;
        ++HashId_;
    }

    bool IsFinished() const {
        return Finished_;
    }

    void Finish() {
        if (IsFinished()) {
            return;
        }
        if (!Flushed_ && LastHash_.Defined()) {
            BlockWriter_.WriteDoc(BlockId_);
            SubWriter_.Write(nullptr, *LastHash_);
        }

        BlockWriter_.Finish();
        SubWriter_.Finish();
        BlockWadWriter_->WriteGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize), TArrayRef<const char>(reinterpret_cast<const char*>(&HashId_), sizeof(HashId_)));
        if (LocalBlockWadWriter_) {
            LocalBlockWadWriter_->Finish();
        }
        if (LocalSubWadWriter_) {
            LocalSubWadWriter_->Finish();
        }
        Finished_ = true;
    }

private:

    void ResetInternal() {
        LastHash_.Clear();
        HashId_ = 0;
        BlockId_ = 0;
        Finished_ = false;
        Flushed_ = false;
    }

    ui32 HashId_ = 0;
    ui32 BlockId_ = 0;
    TMaybe<THash> LastHash_;
    NOffroad::TFlatWriter<std::nullptr_t, Hash, NOffroad::TNullVectorizer, HashVectorizer> SubWriter_;
    THolder<IWadWriter> LocalSubWadWriter_;
    THolder<IWadWriter> LocalBlockWadWriter_;
    IWadWriter* BlockWadWriter_ = nullptr;
    TBlockWriter BlockWriter_;
    bool Finished_ = false;
    bool Flushed_ = false;
};

} // namespace NDoom
