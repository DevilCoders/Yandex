#pragma once

#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/fat/fat_searcher.h>
#include <library/cpp/offroad/key/key_reader.h>

#include <util/generic/string.h>

#include <utility>

namespace NDoom {

template <
    EWadIndexType indexType,
    size_t blockSize>
class TOffroadBlockStringWadReader {
    using TSubSearcher = NOffroad::TFatSearcher<std::nullptr_t, NOffroad::TNullSerializer>;
    using TKeyReader = NOffroad::TKeyReader<std::nullptr_t, NOffroad::TNullVectorizer, NOffroad::TINSubtractor>;
public:
    using THash = TStringBuf;
    using TModel = typename TKeyReader::TModel;
    using TTable = typename TKeyReader::TTable;

    TOffroadBlockStringWadReader() = default;

    template <typename... Args>
    TOffroadBlockStringWadReader(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& subPath, const TString& blockPath) {
        LocalSubWad_ = IWad::Open(subPath);
        LocalBlockWad_ = IWad::Open(blockPath);
        Reset(LocalSubWad_.Get(), LocalBlockWad_.Get());
    }

    void Reset(const IWad* subWad, const IWad* blockWad) {
        BlockId_ = 0;

        SubWad_ = subWad;

        SubSearcher_.Reset(
            SubWad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyFat)),
            SubWad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyIdx))
        );

        BlockWad_ = blockWad;
        TModel model;
        model.Load(BlockWad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel)));

        if (!LocalTable_) {
            LocalTable_ = new TTable();
        }

        LocalTable_->Reset(model);
        BlockWad_->MapDocLumps({{TWadLumpId(indexType, EWadLumpRole::Keys)}}, DocLumpsMapping_);

        NextReader();
    }

    bool ReadHash(THash* hash) {
        while (true) {
            if (KeyReader_.ReadKey(hash, nullptr)) {
                return true;
            }
            if (!NextReader()) {
                return false;
            }
        }
        return false;
    }

private:
    bool NextReader() {
        while (BlockId_ + 1 < SubSearcher_.Size()) {
            KeyBlob_ = BlockWad_->LoadDocLumps(BlockId_++, DocLumpsMapping_, Regions_);
            if (!Regions_[0].empty()) {
                KeyReader_.Reset(LocalTable_.Get(), Regions_[0]);
                return true;
            }
        }
        return false;
    }

    ui32 BlockId_ = 0;

    const IWad* SubWad_ = nullptr;
    THolder<IWad> LocalSubWad_;
    TSubSearcher SubSearcher_;

    THolder<IWad> LocalBlockWad_;
    const IWad* BlockWad_ = nullptr;
    THolder<TKeyReader::TTable> LocalTable_;

    std::array<size_t, 1> DocLumpsMapping_;

    TKeyReader KeyReader_;
    TBlob KeyBlob_;
    std::array<TArrayRef<const char>, 1> Regions_;
};

} // namespace NDoom
