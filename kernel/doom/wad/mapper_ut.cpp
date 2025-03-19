#include "mapper.h"
#include "multi_mapper.h"
#include "doc_lump_writer.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NDoom;

class TSingleWadLoader final : public IDocLumpLoader {
public:
    TSingleWadLoader() = default;

    TSingleWadLoader(TBlob blob, const TMegaWadCommon& common)
        : Blob_(blob)
        , Common_(common)
    {
    }

    bool HasDocLump(size_t docLumpId) const override {
        return Common_.HasDocLump(docLumpId);
    }

    void LoadDocRegions(TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> regions) const override {
        Common_.LoadDocLumpRegions(Blob_, mapping, regions);
    }

    TBlob DataHolder() const override {
        return Blob_;
    }

private:
    TBlob Blob_;
    TMegaWadCommon Common_;
};

class TSingleWadMapper : public IDocLumpMapper {
public:
    TSingleWadMapper() = default;

    TSingleWadMapper(TMegaWadInfo info) {
        Reset(info);
    }

    void Reset(TMegaWadInfo info) {
        Common_.Reset(info);
    }

    TVector<TWadLumpId> DocLumps() const override {
        auto lumps = Common_.Info().DocLumps;
        std::random_shuffle(lumps.begin(), lumps.end());

        TVector<TWadLumpId> result;
        result.reserve(lumps.size());
        for (TStringBuf lump : lumps) {
            result.push_back(FromString(lump));
        }
        return result;
    }

    void MapDocLumps(const TArrayRef<const NDoom::TWadLumpId>& ids, TArrayRef<size_t> mapping) const override {
        Common_.MapDocLumps(ids, mapping);
    }

    TSingleWadLoader GetLoader(TBlob blob) {
        return TSingleWadLoader(std::move(blob), Common_);
    }

private:
    TMegaWadCommon Common_;
};

Y_UNIT_TEST_SUITE(TestMultiMapper) {

    void TestRemapping(TVector<TVector<NDoom::TWadLumpId>> docLumps, TVector<NDoom::TWadLumpId> allLumps) {
        TVector<NDoom::TMegaWadInfo> infos;
        TVector<TSingleWadMapper> mappers(docLumps.size());
        TVector<const TSingleWadMapper*> mapperPtrs(docLumps.size());
        for (size_t i = 0; i < docLumps.size(); ++i) {
            SortBy(docLumps[i], [](TWadLumpId id) {
                return ToString(id);
            });
            infos.emplace_back();
            for (TWadLumpId id : docLumps[i]) {
                infos.back().DocLumps.push_back(ToString(id));
            }
            mappers[i].Reset(infos.back());
            mapperPtrs[i] = &mappers[i];
        }


        TVector<TSingleWadLoader> loaders;
        THashMap<TWadLumpId, TBlob> blobs;
        for (size_t writeChunk = 0; writeChunk < docLumps.size(); ++writeChunk) {
            NDoom::TMegaWadInfo info;
            NDoom::TDocLumpWriter writer(&info);
            for (auto lump : docLumps[writeChunk]) {
                writer.RegisterDocLumpType(lump);
                blobs[lump] = TBlob::FromString(ToString(lump));
            }
            for (size_t i = 0; i < docLumps[writeChunk].size(); ++i) {
                TBlob blob = blobs[docLumps[writeChunk][i]];
                writer.StartDocLump(docLumps[writeChunk][i])->Write(blob.Data(), blob.Size());
            }

            TBuffer result;
            TBufferOutput out(result);
            writer.FinishDoc(&out);
            loaders.push_back(mappers[writeChunk].GetLoader(TBlob::FromBuffer(result)));
        }
        TMultiDocLumpMapper<TSingleWadMapper> multiMapper(mapperPtrs);


        TVector<size_t> mapping(allLumps.size());
        multiMapper.MapDocLumps(allLumps, mapping);
        auto multiLoader = multiMapper.GetLoader<TSingleWadLoader>(loaders);
        TVector<TConstArrayRef<char>> regions(allLumps.size());
        multiLoader.LoadDocRegions(mapping, regions);
        for (size_t i = 0; i < allLumps.size(); ++i) {
            TBlob src = blobs[allLumps[i]];
            UNIT_ASSERT_VALUES_EQUAL(TStringBuf(src.AsCharPtr(), src.Size()), TStringBuf(regions[i].data(), regions[i].size()));
        }
    }


    Y_UNIT_TEST(TestMultiLoader) {
        TVector<NDoom::TWadLumpId> ids = {
            {EWadIndexType::ArcIndexType, EWadLumpRole::HitSub},
            {EWadIndexType::ErfIndexType, EWadLumpRole::Hits},
            {EWadIndexType::CategToNameIndexType, EWadLumpRole::HitsModel},
            {EWadIndexType::DocAttrsIndexType, EWadLumpRole::KeysModel},
            {EWadIndexType::FactorAnnDataIndexType, EWadLumpRole::Keys},
            {EWadIndexType::HnswDocsMappingPerFormulaType, EWadLumpRole::Struct},
            {EWadIndexType::LinkAnnArcIndexType, EWadLumpRole::StructModel},
            {EWadIndexType::OmniDssmAnnXfDtShowOneEmbeddingsType, EWadLumpRole::StructSize}
        };
        TVector<TVector<NDoom::TWadLumpId>> chunks;
        for (size_t i = 1; i < (1 << ids.size()); i += 2) {
            chunks.clear();
            for (size_t j = 0; j < ids.size(); ++j) {
                if ((1 << j) & i) {
                    chunks.emplace_back();
                }
                chunks.back().push_back(ids[j]);
            }
            TestRemapping(chunks, ids);
        }
    }
}
