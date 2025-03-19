#include <library/cpp/testing/unittest/registar.h>

#include <random>

#include <util/generic/algorithm.h>
#include <util/stream/buffer.h>

#include <kernel/doom/wad/mega_wad_buffer_writer.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/test/test_md5.h>

#include "offroad_ann_data_wad_io.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TOffroadAnnDataWadTest) {

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

    template<class TWriter>
    void writeIo(const TMap<ui32, TVector<TAnnDataHit>>& pairedHits, TWriter& writer) {
        for (const auto& pair_data: pairedHits) {
            for (TAnnDataHit hit : pair_data.second) {
                writer.WriteHit(hit);
            }
            writer.WriteDoc(pair_data.first);
        }
    }

    static void SimpleTestTemplate(const TMap<ui32, TVector<TAnnDataHit>>& pairedHits, const TStringBuf& streamMD5, const TStringBuf& modelMD5) {
        using TIo = TOffroadFactorAnnDataDocWadIo;

        TIo::TSampler sampler;
        writeIo(pairedHits, sampler);
        auto model = sampler.Finish();

        TBuffer buffer;
        TMegaWadBufferWriter wadWriter(&buffer);
        TIo::TWriter writer(model, &wadWriter);
        writeIo(pairedHits, writer);
        wadWriter.Finish();

        UNIT_ASSERT_MD5_EQUAL(buffer, streamMD5);

        THolder<IWad> wad = IWad::Open(std::move(buffer));
        TIo::TSearcher reader(wad.Get());
        TIo::TSearcher::TIterator searchIterator;

        TVector<TAnnDataHit> hits;
        for (const auto& pair_data: pairedHits) {
            for (TAnnDataHit hit : pair_data.second) {
                hits.push_back(hit);
            }
        }
        for (const TAnnDataQuery& query: MakeQueries(10000, 125423, hits)) {
            size_t j = std::lower_bound(hits.begin(), hits.end(), TAnnDataHit(query.DocId, query.BreakId, 0, 0, 0)) - hits.begin();

            TAnnDataHit hit;
            bool success = (reader.Find(query.DocId, &searchIterator) && (searchIterator.LowerBound(TAnnDataHit(query.DocId, query.BreakId, 0, 0, 0), &hit)));

            UNIT_ASSERT(success == (j < hits.size() && query.DocId == hits[j].DocId()));

            if (!success)
                continue;

            size_t n = 0;
            while (searchIterator.ReadHit(&hit) && n < 100) {
                hit.SetDocId(query.DocId);
                UNIT_ASSERT(j < hits.size());
                UNIT_ASSERT_EQUAL(hit, hits[j]);
                j++;
                n++;
            }
        }

        TBufferStream modelStream;
        model.Save(&modelStream);
        UNIT_ASSERT_MD5_EQUAL(modelStream.Buffer(), modelMD5);
    }

    Y_UNIT_TEST(TestSimple) {
        SimpleTestTemplate(MakeHits(100000, 1233354), "26aa9253eaf585e9c167faeac8e7172f", "dce34e267de038be4631a0b63dd8eaac");
    }

    Y_UNIT_TEST(TestEdgeCase) {
        SimpleTestTemplate(CoverEdgeCase(100, 1233354), "069236fc937fa77d667923cef9608acf", "4ceec2b37aff65871843caaae30f65bf");
    }

    Y_UNIT_TEST(TestRepeats) {
        SimpleTestTemplate(MakeRepeatedHits(5), "42686fe89296217db4cdeb1132157923", "5fc6e7bd9c06d0dbf3cf2cf3d1d346aa");
    }
}
