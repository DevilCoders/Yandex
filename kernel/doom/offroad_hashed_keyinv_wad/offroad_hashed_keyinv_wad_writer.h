#pragma once

#include <kernel/doom/offroad_block_wad/offroad_block_wad_writer.h>
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
    class Hit,
    class Vectorizer,
    class Subtractor,
    EOffroadDocCodec codec,
    size_t blockSize>
class TOffroadHashedKeyInvWadWriter {
    using TBlockWriter = TOffroadBlockWadWriter<indexType, Hash, HashVectorizer, HashSubtractor, codec, blockSize>;
    using THitWriter = TOffroadDocWadWriter<indexType, Hit, Vectorizer, Subtractor, NOffroad::TNullVectorizer, codec>;
public:
    using THash = Hash;
    using TBlockModel = typename TBlockWriter::TModel;

    using THit = Hit;
    using THitModel = typename THitWriter::TModel;

    TOffroadHashedKeyInvWadWriter() = default;

    template<class... Args>
    TOffroadHashedKeyInvWadWriter(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& subPath, const TBlockModel& blockModel, const TString& blockPath, const THitModel& hitModel, const TString& hashPath) {
        BlockWriter_.Reset(subPath, blockModel, blockPath);
        HitWriter_.Reset(hitModel, hashPath);
    }

    void Reset(IWadWriter* subWriter, const TBlockModel& blockModel, IWadWriter* blockWriter, const THitModel& hitModel, IWadWriter* hashWriter) {
        BlockWriter_.Reset(subWriter, blockModel, blockWriter);
        HitWriter_.Reset(hitModel, hashWriter);
    }

    void WriteHit(const THit& hit) {
        HitWriter_.WriteHit(hit);
    }

    void WriteBlock(const THash& hash) {
        ui32 hashId = 0;
        BlockWriter_.WriteBlock(hash, &hashId);
        HitWriter_.WriteDoc(hashId);
    }

    bool IsFinished() const {
        return Finished_;
    }

    void Finish() {
        BlockWriter_.Finish();
        HitWriter_.Finish();
        Finished_ = true;
    }

private:

    TBlockWriter BlockWriter_;
    THitWriter HitWriter_;
    bool Finished_ = false;
    bool Flushed_ = false;
};

} // namespace NDoom
