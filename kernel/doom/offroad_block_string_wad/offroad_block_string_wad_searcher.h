#pragma once

#include <kernel/doom/wad/mega_wad.h>

#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/fat/fat_searcher.h>
#include <library/cpp/offroad/key/key_reader.h>

#include <util/generic/algorithm.h>
#include <util/memory/blob.h>

#include <cstddef>
#include <type_traits>

namespace NDoom {

template <
    EWadIndexType indexType,
    size_t blockSize>
class TOffroadBlockStringWadSearcher {
    using TSubSearcher = NOffroad::TFatSearcher<std::nullptr_t, NOffroad::TNullSerializer>;
    using TKeyReader = NOffroad::TKeyReader<std::nullptr_t, NOffroad::TNullVectorizer, NOffroad::TINSubtractor>;
public:
    using THash = TStringBuf;
    using TModel = TKeyReader::TModel;
    using TTable = TKeyReader::TTable;

    TOffroadBlockStringWadSearcher() = default;

    TOffroadBlockStringWadSearcher(const IWad* subWad, const IWad* blockWad) {
        Reset(subWad, blockWad);
    }

    void Reset(const IWad* subWad, const IWad* blockWad) {
        SubSearcher_.Reset(
            subWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyFat)),
            subWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyIdx))
        );

        BlockWad_ = blockWad;

        TModel model;
        model.Load(BlockWad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel)));

        if (!LocalTable_) {
            LocalTable_ = new TTable();
        }

        LocalTable_->Reset(model);
        BlockWad_->MapDocLumps({{TWadLumpId(indexType, EWadLumpRole::Keys)}}, DocLumpsMapping_);

        if (BlockWad_->HasGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize))) {
            TBlob sizeBlob = BlockWad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize));
            Y_ENSURE(sizeBlob.Size() == sizeof(ui32));
            Size_ = ReadUnaligned<ui32>(sizeBlob.Data());
        }
    }

    bool Find(THash hash, ui32* hashId) const {
        return FindHashId(hash, hashId);
    }

    TVector<bool> Find(const TArrayRef<const THash>& hashes, TArrayRef<ui32> hashIds) const {
        return FindInternal(hashes, hashIds);
    }

    inline ui32 Size() const {
        return Size_;
    }

private:

    inline ui32 FindBlock(THash hash) const {
        return SubSearcher_.LowerBound(hash) - 1;
    }

    inline bool FindHashId(THash hash, ui32* hashId) const {
        ui32 blockId = FindBlock(hash);
        std::array<TArrayRef<const char>, 1> regions;
        TBlob keyBlob = BlockWad_->LoadDocLumps(blockId, DocLumpsMapping_, regions);
        if (regions[0].empty()) {
            return false;
        }
        ui32 index = 0;
        TKeyReader reader;
        reader.Reset(LocalTable_.Get(), regions[0]);

        TKeyReader::TKeyRef firstHashRef;

        if (!reader.ReadKey(&firstHashRef, nullptr)) {
            return false;
        }
        while (firstHashRef < hash) {
            if (!reader.ReadKey(&firstHashRef, nullptr)) {
                return false;
            }
            ++index;
        }
        if (hash != firstHashRef) {
            return false;
        }

        size_t foundHashId = index + blockSize * static_cast<size_t>(blockId);
        // unlikely scenario, but can happen
        if (Y_UNLIKELY(foundHashId > static_cast<size_t>(Max<ui32>()))) {
            return false;
        }

        *hashId = static_cast<ui32>(foundHashId);


        return true;
    }

    /*
        Hashes must be sorted
    */
    TVector<bool> FindInternal(const TArrayRef<const THash>& hashes, TArrayRef<ui32> hashIds) const {
        const size_t hashesSize = hashes.size();
        TVector<ui32> blocks(hashesSize);
        TVector<bool> found(hashesSize);
        for (size_t i = 0; i < hashesSize; ++i) {
            blocks[i] = FindBlock(hashes[i]);
        }
        Y_ASSERT(IsSorted(blocks.begin(), blocks.end()));
        size_t i = 0;
        while (i < hashesSize) {
            ui32 index = 0;
            ui32 blockId = blocks[i];
            TKeyReader reader;
            TKeyReader::TKeyRef firstHashRef;

            bool blockIsEmpty = true;
            std::array<TArrayRef<const char>, 1> regions;
            TBlob keyBlob = BlockWad_->LoadDocLumps(blockId, DocLumpsMapping_, regions);
            if (!regions[0].empty()) {
                reader.Reset(LocalTable_.Get(), regions[0]);
                if (reader.ReadKey(&firstHashRef, nullptr)) {
                    blockIsEmpty = false;
                }
            }
            while (i < hashesSize && blocks[i] == blockId) {
                if (!blockIsEmpty) {
                    const THash& hash = hashes[i];
                    while (firstHashRef < hash) {
                        if (!reader.ReadKey(&firstHashRef, nullptr)) {
                            blockIsEmpty = true;
                            break;
                        }
                        ++index;
                    }
                    if (blockIsEmpty) {
                        continue;
                    }
                    if (hash == firstHashRef) {
                        size_t foundHashId = index + blockSize * static_cast<size_t>(blockId);
                        // unlikely scenario, but can happen
                        if (Y_UNLIKELY(foundHashId > static_cast<size_t>(Max<ui32>()))) {
                            continue;
                        }

                        hashIds[i] = static_cast<ui32>(foundHashId);
                        found[i] = true;
                    }
                }
                ++i;
            }
        }
        return found;
    }

    TSubSearcher SubSearcher_;
    const IWad* BlockWad_ = nullptr;
    THolder<TKeyReader::TTable> LocalTable_;
    std::array<size_t, 1> DocLumpsMapping_;
    ui32 Size_ = 0;
};

} // namespace NDoom
