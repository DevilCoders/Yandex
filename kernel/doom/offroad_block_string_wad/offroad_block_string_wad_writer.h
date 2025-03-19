#pragma once

#include <kernel/doom/wad/mega_wad_writer.h>

#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/fat/fat_writer.h>
#include <library/cpp/offroad/key/key_writer.h>

#include <util/generic/maybe.h>

#include <cstddef>
#include <utility>

namespace NDoom {

template <
    EWadIndexType indexType,
    size_t blockSize>
class TOffroadBlockStringWadWriter {
    using TKeyWriter = NOffroad::TKeyWriter<std::nullptr_t, NOffroad::TNullVectorizer, NOffroad::TINSubtractor>;
    using TTable = typename TKeyWriter::TTable;
public:
    using THash = TStringBuf;
    using TModel = typename TKeyWriter::TModel;

    TOffroadBlockStringWadWriter() = default;

    template<class... Args>
    TOffroadBlockStringWadWriter(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(IWadWriter* subWriter, const TModel& model, IWadWriter* blockWriter) {
        ResetInternal();
        SubWadWriter_ = subWriter;
        FatWriter_.Reset(&FatOutput_, &FatSubOutput_);

        WadWriter_ = blockWriter;
        if (!LocalTable_) {
            LocalTable_ = new TTable();
        }

        LocalTable_->Reset(model);
        WadWriter_->RegisterIndexType(indexType);
        WadWriter_->RegisterDocLumpType(TWadLumpId(indexType, EWadLumpRole::Keys));

        /* Write out model right away. */
        model.Save(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel)));
    }

    void Reset(const TString& subPath, const TModel& model, const TString& blockPath) {
        LocalSubWadWriter_ = new TMegaWadWriter(subPath);
        LocalWadWriter_ = new TMegaWadWriter(blockPath);
        Reset(LocalSubWadWriter_.Get(), model, LocalWadWriter_.Get());
    }

    void WriteBlock(THash hash, ui32* hashId) {
        Y_ENSURE(!LastHash_.Defined() || hash > *LastHash_);
        LastHash_ = hash;
        if (HashId_ % blockSize == 0) {
            KeyWriter_.Reset(LocalTable_.Get(), WadWriter_->StartDocLump(BlockId_, TWadLumpId(indexType, EWadLumpRole::Keys)));
        }
        KeyWriter_.WriteKey(hash, nullptr);
        Flushed_ = false;
        if (HashId_ % blockSize == blockSize - 1) {
            FatWriter_.WriteKey(hash, nullptr);
            KeyWriter_.Finish();
            Flushed_ = true;
            Y_ENSURE(BlockId_ != Max<ui32>()); // we don't want too many blocks
            ++BlockId_;
            KeyWriter_.Reset(LocalTable_.Get(), WadWriter_->StartDocLump(BlockId_, TWadLumpId(indexType, EWadLumpRole::Keys)));
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
            FatWriter_.WriteKey(*LastHash_, nullptr);
        }
        KeyWriter_.Finish();
        FatWriter_.Finish();

        FatOutput_.Flush(SubWadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyFat)));
        FatSubOutput_.Flush(SubWadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyIdx)));

        WadWriter_->WriteGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize), TArrayRef<const char>(reinterpret_cast<const char*>(&HashId_), sizeof(HashId_)));

        if (LocalSubWadWriter_) {
            LocalSubWadWriter_->Finish();
        }
        if (LocalWadWriter_) {
            LocalWadWriter_->Finish();
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

    NDoom::TAccumulatingOutput FatOutput_;
    NDoom::TAccumulatingOutput FatSubOutput_;
    IWadWriter* SubWadWriter_ = nullptr;
    THolder<IWadWriter> LocalSubWadWriter_;
    NOffroad::TFatWriter<std::nullptr_t, NOffroad::TNullSerializer> FatWriter_;

    IWadWriter* WadWriter_ = nullptr;
    THolder<IWadWriter> LocalWadWriter_;
    THolder<TKeyWriter::TTable> LocalTable_;

    TKeyWriter KeyWriter_;
    bool Finished_ = false;
    bool Flushed_ = false;
};

} // namespace NDoom
