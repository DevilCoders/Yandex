#pragma once

#include "part_location.h"
#include "wad_writer.h"

#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/mega_wad_common.h>
#include <kernel/doom/wad/deduplicator.h>

#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>

#include <util/generic/maybe.h>
#include <util/stream/mem.h>

namespace NDoom {

    class TErasureLocationResolver {
    private:
        using TLocationSearcher = NOffroad::TFlatSearcher<std::nullptr_t, TErasureBlobLocation, NOffroad::TNullVectorizer, TErasureBlobLocationVectorizer>;

    public:
        TErasureLocationResolver() = default;

        TErasureLocationResolver(IWad* wad) {
            Reset(wad);
        }

        void Reset() {
            Searcher_.Reset();
            Size_ = 0;
        }

        void Reset(IWad* wad) {
            Searcher_.Reset(wad->LoadGlobalLump({EWadIndexType::ErasurePartLocations, EWadLumpRole::Hits}));
            TMegaWadInfo info = LoadMegaWadInfo(wad->LoadGlobalLump({EWadIndexType::ErasurePartLocations, EWadLumpRole::HitSub}));
            Size_ = info.DocCount;
        }

        TMaybe<TErasureBlobLocation> Resolve(size_t blobId) const {
            if (blobId >= Size_) {
                return {};
            } else {
                return Searcher_.ReadData(blobId);
            }
        }

        ui32 Size() const {
            return Size_;
        }

    private:
        TLocationSearcher Searcher_;
        ui32 Size_ = 0;
    };

    struct TDecoderTableFactory {
        static THolder<TOffsetsMappingSampler::TModel::TDecoderTable> Create(const TBlob& data) {
            TMemoryInput input(data.Data(), data.Size());
            TOffsetsMappingSampler::TModel model;
            model.Load(&input);
            return MakeHolder<TOffsetsMappingSampler::TModel::TDecoderTable>(model);
        }
    };

    inline void EnableErasureDecoderTablesDeduplication() {
        Deduplicator<TDecoderTableFactory>().DeduplicationEnabled = true;
    }

    class TErasureLocationResolverByPartIndex {
    private:
        using TBlockReader = NOffroad::TFlatSearcher<ui64, ui64, NOffroad::TUi64Vectorizer, NOffroad::TUi64Vectorizer>;
        using TOffsetReader = NOffroad::TTupleReader<ui64, NOffroad::TUi64Vectorizer, NOffroad::TD2Subtractor, NOffroad::TDecoder64, 1, NOffroad::EBufferType::PlainOldBuffer>;

    public:
        TErasureLocationResolverByPartIndex() = default;

        TErasureLocationResolverByPartIndex(IWad* wad, const ui32 index) {
            Reset(wad, index);
        }

        void Reset(IWad* wad, const ui32 index) {
            {
                TBlob data = wad->LoadGlobalLump({ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitSub });
                TMemoryInput input(data.AsCharPtr(), data.Size());
                ::Load(&input, IndexCount_);
            }

            Y_ENSURE(index < IndexCount_);
            Index_ = index;


            {
                TBlob data = wad->LoadGlobalLump({ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitsModel });
                OffsetReaderModelTable_ = Deduplicator<TDecoderTableFactory>().GetOrCreate(data);
            }

            std::array<size_t, 2> mapping;
            wad->MapDocLumps({
                { EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::Hits },
                { EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitSub }
                }, mapping);

            bool hasSizeLump = false;
            std::array<size_t, 1> sizeMapping;

            const TVector<TWadLumpId> docLumps = wad->DocLumps();
            if (Find(docLumps, TWadLumpId{EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitsModel}) != docLumps.end()) {
                hasSizeLump = true;
                wad->MapDocLumps({{ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitsModel }}, sizeMapping);
            }

            for (ui32 part = 0; ; ++part) {
                std::array<TArrayRef<const char>, 2> regions;
                PartDataBlobs_.push_back(wad->LoadDocLumps(part, mapping, regions));

                if (regions[0].empty() || regions[1].empty()) {
                    break;
                }

                OffsetReaderData_.push_back(regions[0]);
                BlockReaderByPart_.emplace_back(regions[1]);

                if (hasSizeLump) {
                    std::array<TArrayRef<const char>, 1> sizeRegions;
                    TBlob data = wad->LoadDocLumps(part, sizeMapping, sizeRegions);
                    TMemoryInput input(sizeRegions[0].begin(), sizeRegions[0].size());
                    ui32 size;
                    ::Load(&input, size);
                    Sizes_.push_back(size);
                } else {
                    Sizes_.push_back(Max<ui32>());
                }
            }
        }

        TMaybe<TErasureBlobLocation> Resolve(const ui32 part, ui32 partPosition) const {
            if (part >= BlockReaderByPart_.size()) {
                return Nothing();
            }

            partPosition = partPosition * IndexCount_ + Index_;

            if (partPosition + 1 >= Sizes_[part]) {
                return Nothing();
            }

            ui64 blockStart = 0;
            ui64 blockOffset = 0;
            const ui32 blockNum = (partPosition >> 6);
            if (blockNum != 0) {
                if (blockNum > BlockReaderByPart_[part].Size()) {
                    return Nothing();
                }

                blockStart = BlockReaderByPart_[part].ReadKey(blockNum - 1);
                blockOffset = BlockReaderByPart_[part].ReadData(blockNum - 1);
            }

            TOffsetReader offsetReader(OffsetReaderModelTable_.Get(), OffsetReaderData_[part]);

            if (!offsetReader.Seek(NOffroad::TDataOffset::FromEncoded((blockOffset << 6) + (partPosition & 63)), blockStart)) {
                return Nothing();
            }

            TErasureBlobLocation location;
            location.Part = part;

            if (!offsetReader.ReadHit(&location.Offset)) {
                return Nothing();
            }

            if (!offsetReader.ReadHit(&location.Size)) {
                return Nothing();
            }

            location.Size -= location.Offset;

            // Remove when all chunks will contain {EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitsModel} lump
            if (location.Size == 0) {
                return Nothing();
            }

            return location;
        }

    private:
        TVector<TBlob> PartDataBlobs_;
        TVector<TBlockReader> BlockReaderByPart_;
        TVector<TArrayRef<const char>> OffsetReaderData_;
        TVector<ui32> Sizes_;
        TAtomicSharedPtr<const TOffsetsMappingSampler::TModel::TDecoderTable> OffsetReaderModelTable_;
        ui32 Index_;
        ui32 IndexCount_;
    };

    class TPartIndexResolverByChunkIndex {
    private:
        using TLocationSearcher = NOffroad::TFlatSearcher<std::nullptr_t, TDocInPartLocation, NOffroad::TNullVectorizer, TDocInPartLocationVectorizer>;

    public:
        TPartIndexResolverByChunkIndex() = default;

        TPartIndexResolverByChunkIndex(IWad* wad) {
            Reset(wad);
        }

        void Reset() {
            Searcher_.Reset();
        }

        void Reset(IWad* wad) {
            Searcher_.Reset(wad->LoadGlobalLump({ EWadIndexType::ErasureDocInPartLocation, EWadLumpRole::Hits }));
        }

        TMaybe<TDocInPartLocation> Resolve(const ui32 chunkPosition) const {
            if (chunkPosition < Searcher_.Size()) {
                return Searcher_.ReadData(chunkPosition);
            } else {
                return Nothing();
            }
        }

    private:
        TLocationSearcher Searcher_;
    };

    class TPartOptimizedErasureLocationResolver {
    public:
        template<typename... Args>
        TPartOptimizedErasureLocationResolver(Args&&... args) {
            Reset(std::forward<Args>(args)...);
        }

        void Reset() {
            PartResolverHolder_.Destroy();
            OffsetResolverHolder_.Destroy();

            PartResolver_ = nullptr;
            OffsetResolver_ = nullptr;
        }

        void Reset(IWad* offsetWad, const ui32 index, IWad* partWad) {
            Reset();

            Y_ENSURE(offsetWad);
            OffsetResolverHolder_ = MakeHolder<TErasureLocationResolverByPartIndex>(offsetWad, index);

            if (partWad) {
                PartResolverHolder_ = MakeHolder<TPartIndexResolverByChunkIndex>(partWad);
            }

            PartResolver_ = PartResolverHolder_.Get();
            OffsetResolver_ = OffsetResolverHolder_.Get();
        }

        void Reset(TErasureLocationResolverByPartIndex* offsetResolver, TPartIndexResolverByChunkIndex* partResolver) {
            Reset();

            PartResolver_ = partResolver;
            OffsetResolver_ = offsetResolver;
        }

        TMaybe<TErasureBlobLocation> Resolve(const ui32 chunkPosition) const {
            TDocInPartLocation partLocation;

            if (PartResolver_) {
                if (auto partLocationMaybe = PartResolver_->Resolve(chunkPosition)) {
                    partLocation = *partLocationMaybe;
                } else {
                    return Nothing();
                }
            } else {
                partLocation = {0, chunkPosition};
            }

            return OffsetResolver_->Resolve(partLocation.Part, partLocation.PartPosition);
        }

    private:
        THolder<TPartIndexResolverByChunkIndex> PartResolverHolder_;
        THolder<TErasureLocationResolverByPartIndex> OffsetResolverHolder_;

        TPartIndexResolverByChunkIndex* PartResolver_ = nullptr;
        TErasureLocationResolverByPartIndex* OffsetResolver_ = nullptr;
    };

}
