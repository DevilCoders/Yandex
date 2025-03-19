#include <library/cpp/testing/unittest/registar.h>
#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/mega_wad.h>
#include <kernel/doom/chunked_wad/single_chunked_wad.h>
#include <kernel/doom/doc_lump_fetcher/chain.h>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestChunkedWadFetchers) {
    TWadLumpId LUMPS[] = {
        TWadLumpId(FactorAnnDataIndexType,  EWadLumpRole::HitsModel),
        TWadLumpId(FactorAnnDataIndexType,  EWadLumpRole::Hits),
        TWadLumpId(FactorAnnDataIndexType,  EWadLumpRole::HitSub),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::HitsModel),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::KeysModel),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::Keys),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::KeyFat),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::KeyIdx),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::Hits),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::HitSub),
    };

    TString LumpContent(size_t doc, TWadLumpId id) {
        return "$" + ToString(doc) + ":" + ToString(id) + "^";
    }

    Y_UNIT_TEST(SingleWad) {
        constexpr size_t Docs = 1000;

        NDoom::TMegaWadWriter writer{ "test.wad" };
        writer.RegisterDocLumpTypes(LUMPS);
        for (size_t i = 0; i < Docs; ++i) {
            for (auto id : LUMPS) {
                writer.StartDocLump(i, id)->Write(LumpContent(i, id));
            }
        }
        writer.Finish();

        THolder<TSingleChunkedWad> chunkedWad = THolder(new TSingleChunkedWad(MakeHolder<NDoom::TMegaWad>("test.wad", false)));

        TVector<ui32> docIds;
        for (size_t i = 0; i < Docs * 2; ++i) {
            docIds.push_back(i);
        }

        TVector<size_t> mapping(std::size(LUMPS));
        TVector<size_t> fetcherMapping(std::size(LUMPS));
        chunkedWad->MapDocLumps(LUMPS, mapping);
        chunkedWad->Mapper().MapDocLumps(LUMPS, fetcherMapping);

        std::random_shuffle(docIds.begin(), docIds.end());

        chunkedWad->Fetch(
            docIds,
            [&](size_t i, auto* loader) {
                UNIT_ASSERT(loader);
                TVector<TArrayRef<const char>> fetcherRegions(std::size(LUMPS));
                loader->LoadDocRegions(fetcherMapping, fetcherRegions);

                TVector<TArrayRef<const char>> regions(mapping.size());
                TBlob blob = chunkedWad->LoadDocLumps(docIds[i], mapping, regions);

                for (size_t j = 0; j < fetcherRegions.size(); ++j) {
                    auto region1 = fetcherRegions[j];
                    auto region2 = regions[j];
                    UNIT_ASSERT_VALUES_EQUAL(TStringBuf(region1.data(), region1.size()), TStringBuf(region2.data(), region2.size()));
                }
            });
    }

    Y_UNIT_TEST(SingleWadsChain) {
        constexpr size_t Docs = 1000;
        size_t index = 0;
        for (size_t i = 0; i < std::size(LUMPS); i += 2) {
            TMegaWadWriter writer(ToString(index) + ".test.wad");
            ++index;
            for (size_t j = i; j < std::size(LUMPS) && j < i + 2; ++j) {
                writer.RegisterDocLumpType(LUMPS[j]);
            }

            for (size_t doc = 0; doc < Docs; ++doc) {
                for (size_t j = i; j < std::size(LUMPS) && j < i + 2; ++j) {
                    writer.StartDocLump(doc, LUMPS[j])->Write(LumpContent(doc, LUMPS[j]));
                }
            }

            writer.Finish();
        }

        TChainedFetchersWrapper<TChunkedWadDocLumpLoader> wrapper, subwrapper;
        TVector<THolder<IDocLumpFetcher<TChunkedWadDocLumpLoader>>> fetchers;
        for (size_t i = 0; i < index; ++i) {
            fetchers.push_back(MakeHolder<TSingleChunkedWad>(MakeHolder<TMegaWad>(ToString(i) + ".test.wad", false)));
        }
        subwrapper.AddFetcher(*fetchers[0]).AddFetcher(*fetchers[1]).Init();

        for (size_t i = 2; i < index; ++i) {
            wrapper.AddFetcher(*fetchers[i]);
        }
        wrapper.SetMultiFetcher(subwrapper, subwrapper.MultiMapper()).Init();

        TVector<ui32> docIds;
        for (size_t i = 0; i < Docs * 2; ++i) {
            docIds.push_back(i);
        }
        std::random_shuffle(docIds.begin(), docIds.end());

        TVector<ui32> called;
        TVector<size_t> mapping(std::size(LUMPS));
        wrapper.Mapper().MapDocLumps(LUMPS, mapping);
        wrapper.Fetch(
            docIds,
            [&](size_t i, auto* loader) {
                called.push_back(docIds[i]);
                UNIT_ASSERT(loader);
                TVector<TArrayRef<const char>> regions(std::size(LUMPS));
                loader->LoadDocRegions(mapping, regions);

                for (size_t j = 0; j < regions.size(); ++j) {
                    auto region1 = regions[j];
                    auto region2 = LumpContent(docIds[i], LUMPS[j]);
                    if (docIds[i] < Docs) {
                        UNIT_ASSERT_VALUES_EQUAL(TStringBuf(region1.data(), region1.size()), TStringBuf(region2.data(), region2.size()));
                    } else {
                        UNIT_ASSERT_VALUES_EQUAL(TStringBuf(region1.data(), region1.size()), TStringBuf{});
                    }
                }
            });
        Sort(docIds);
        Sort(called);
        UNIT_ASSERT(docIds == called);
    }
}
