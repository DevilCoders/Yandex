#include <library/cpp/testing/unittest/registar.h>

#include <random>

#include <util/generic/algorithm.h>
#include <util/stream/buffer.h>

#include <kernel/doom/wad/mega_wad_buffer_writer.h>
#include <kernel/doom/offroad_ann_data_wad/offroad_ann_data_wad_io.h>
#include <kernel/indexann/interface/reader.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/test/test_md5.h>


#include "offroad_ann_data_wad_accessor.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TOffroadAnnDataWadAccessorTest) {

    struct TAnnDataQuery {
        ui32 DocId = 0;
        ui32 BreakId = 0;
    };

    // count is approximate here
    static TMap<ui32, TVector<TAnnDataHit>> CoverEdgeCase(size_t count, size_t seed) {
        TMap<ui32, TVector<TAnnDataHit>> result;

        std::mt19937 random(seed);

        ui32 docId = 0;
        for (size_t i = 0; i < count;) {
            docId += 1 + random() % 3;

            size_t num_hits = 15 + random() % 3; // 15, 16 or 17 hits strictly
            for (size_t j = 0; j < num_hits; ++j) {
                TAnnDataHit hit;

                hit.SetDocId(docId);
                hit.SetBreak(random() % 5);
                hit.SetRegion(random() % 5);
                hit.SetStream(random() % 256);
                hit.SetValue(random() % 256);

                result[docId].push_back(hit);
            }

            i += num_hits;
        }

        for (auto& pair_data : result) {
            Sort(pair_data.second.begin(), pair_data.second.end());
        }

        return result;
    }

    static TMap<ui32, TVector<TAnnDataHit>> MakeHits(size_t count, size_t seed) {
        TMap<ui32, TVector<TAnnDataHit>> result;

        std::mt19937 random(seed);

        for (size_t i = 0; i < count; i++) {
            TAnnDataHit hit;

            ui32 docId = 0;
            if (random() % 2) {
                docId = random() % 1000000;
            } else {
                docId = random() % 10000;
                docId = docId * docId / 10000;
            }

            hit.SetDocId(docId);
            hit.SetBreak(random() % 5);
            hit.SetRegion(random() % 5);
            hit.SetStream(random() % 256);
            hit.SetValue(random() % 256);

            result[docId].push_back(hit);
        }

        for (auto& pair_data : result) {
            Sort(pair_data.second.begin(), pair_data.second.end());
        }
        return result;
    }

    static TMap<ui32, TVector<TAnnDataHit>> MakeRepeatedHits(size_t count) {
        TMap<ui32, TVector<TAnnDataHit>> result;

        for (size_t i = 0; i < count; i++)
            result[i].push_back(TAnnDataHit(i, 1, 1, 1, 1));

        return result;
    }

    static TVector<TAnnDataQuery> MakeQueries(size_t count, size_t seed, const TVector<TAnnDataHit>& hits) {
        TVector<TAnnDataQuery> result;

        std::mt19937 random(seed);

        for (size_t i = 0; i < count; i++) {
            TAnnDataQuery query;

            size_t index = random() % hits.size();
            query.DocId = hits[index].DocId();
            query.BreakId = hits[index].Break();

            if (random() % 3 == 0)
                query.BreakId++;

            result.push_back(query);
        }

        return result;
    }

    static void SimpleTestTemplate(const TMap<ui32, TVector<TAnnDataHit>>& pairedHits) {
        using TIo = TOffroadFactorAnnDataDocWadIo;
        TIo::TSampler sampler;
        auto model = sampler.Finish();
        TBuffer buffer;
        TMegaWadBufferWriter wadWriter(&buffer);
        TIo::TWriter writer(model, &wadWriter);
        for (const auto& pair_data : pairedHits) {
            for (TAnnDataHit hit : pair_data.second) {
                writer.WriteHit(hit);
            }
            writer.WriteDoc(pair_data.first);
        }
        wadWriter.Finish();

        THolder<IWad> wad = IWad::Open(std::move(buffer));
        THolder<NIndexAnn::IDocDataIndex> index = NewOffroadAnnDataWadIndex(wad.Get(), wad.Get());
        THolder<NIndexAnn::IDocDataIterator> iterator = index->CreateIterator();

        TVector<TAnnDataHit> hits;
        for (const auto& pair_data : pairedHits) {
            for (TAnnDataHit hit : pair_data.second) {
                hits.push_back(hit);
            }
        }
        for (const auto& query: MakeQueries(10000, 125423, hits)) {
            size_t j = std::lower_bound(hits.begin(), hits.end(), TAnnDataHit(query.DocId, query.BreakId, 0, 0, 0)) - hits.begin();

            iterator->Restart(NIndexAnn::THitMask(query.DocId, query.BreakId));

            const TAnnDataHit* hit = iterator->Valid() ? &iterator->Current() : nullptr;
            while(true) {
                if (j < hits.size() && query.DocId == hits[j].DocId() && query.BreakId == hits[j].Break()) {
                    UNIT_ASSERT(iterator->Valid());
                    UNIT_ASSERT(hit);
                    UNIT_ASSERT_EQUAL(iterator->Current(), hits[j]);
                } else {
                    UNIT_ASSERT(!iterator->Valid());
                    UNIT_ASSERT(!hit);
                    break;
                }

                //Cerr << ToDebugString(iterator->Current()) << Endl;

                hit = iterator->Next();
                j++;
            }
        }
    }

    Y_UNIT_TEST(TestSimple) {
        SimpleTestTemplate(MakeHits(100000, 1233354));
    }

    Y_UNIT_TEST(TestEdgeCase) {
        SimpleTestTemplate(CoverEdgeCase(100, 1233354));
    }

    Y_UNIT_TEST(TestRepeats) {
        SimpleTestTemplate(MakeRepeatedHits(5));
    }
}
