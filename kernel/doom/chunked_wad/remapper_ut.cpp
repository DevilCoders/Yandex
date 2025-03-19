#include "mapper.h"
#include <kernel/doom/wad/doc_lump_writer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestRemapper) {

    void TestRemapping(TVector<TVector<NDoom::TWadLumpId>> docLumps, TVector<NDoom::TWadLumpId> allLumps) {
        TVector<NDoom::TMegaWadInfo> infos;
        for (size_t i = 0; i < docLumps.size(); ++i) {
            SortBy(docLumps[i], [](TWadLumpId id) {
                return ToString(id);
            });
            infos.emplace_back();
            for (TWadLumpId id : docLumps[i]) {
                infos.back().DocLumps.push_back(ToString(id));
            }
        }
        TChunkedWadDocLumpMapper mapper(infos);
        for (size_t writeChunk = 0; writeChunk < docLumps.size(); ++writeChunk) {
            NDoom::TMegaWadInfo info;
            NDoom::TDocLumpWriter writer(&info);
            TVector<TBlob> blobs;
            for (auto lump : docLumps[writeChunk]) {
                writer.RegisterDocLumpType(lump);
                blobs.push_back(TBlob::FromString(ToString(lump)));
            }
            for (size_t i = 0; i < docLumps[writeChunk].size(); ++i) {
                writer.StartDocLump(docLumps[writeChunk][i])->Write(blobs[i].Data(), blobs[i].Size());
            }
            TBuffer result;
            TBufferOutput out(result);
            writer.FinishDoc(&out);

            TVector<size_t> mapping(allLumps.size());
            mapper.MapDocLumps(allLumps, mapping);
            auto loader = mapper.GetLoader(TBlob::FromBuffer(result), writeChunk);
            TVector<TConstArrayRef<char>> regions(allLumps.size());
            loader.LoadDocRegions(mapping, regions);

            for (size_t i = 0; i < allLumps.size(); ++i) {
                auto it = Find(info.DocLumps.begin(), info.DocLumps.end(), ToString(allLumps[i]));
                if (it != info.DocLumps.end()) {
                    size_t id = it - info.DocLumps.begin();
                    UNIT_ASSERT_VALUES_EQUAL(TStringBuf(blobs[id].AsCharPtr(), blobs[id].Size()), TStringBuf(regions[i].data(), regions[i].size()));
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(TStringBuf(regions[i].data(), regions[i].size()), "");
                }
            }
        }
    }


    Y_UNIT_TEST(TestChunks) {
        TVector<NDoom::TWadLumpId> ids = {
            {EWadIndexType::ArcIndexType, EWadLumpRole::HitSub},
            {EWadIndexType::ErfIndexType, EWadLumpRole::Hits},
            {EWadIndexType::CategToNameIndexType, EWadLumpRole::HitsModel},
            {EWadIndexType::DocAttrsIndexType, EWadLumpRole::KeysModel},
            {EWadIndexType::FactorAnnDataIndexType, EWadLumpRole::Keys},
            {EWadIndexType::HnswDocsMappingPerFormulaType, EWadLumpRole::Struct},
            {EWadIndexType::LinkAnnArcIndexType, EWadLumpRole::StructModel},
            {EWadIndexType::OmniDssmAnnXfDtShowOneEmbeddingsType, EWadLumpRole::StructSize},
        };
        TVector<TVector<NDoom::TWadLumpId>> chunks;
        TVector<size_t> order;
        for (size_t i = 0; i < ids.size(); ++i) {
            order.push_back(i);
        }
        for (size_t i = 1; i < (1 << ids.size()); ++i) {
            chunks.emplace_back();
            for (size_t j = 0; j < ids.size(); ++j) {
                if ((1 << j) & i) {
                    chunks.back().push_back(ids[j]);
                }
            }
        }
        TestRemapping(chunks, ids);
    }
}
