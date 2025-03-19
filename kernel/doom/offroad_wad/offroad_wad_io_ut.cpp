#include <library/cpp/testing/unittest/registar.h>

#include <random>

#include <util/generic/algorithm.h>
#include <util/stream/str.h>
#include <util/stream/buffer.h>

#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/test/test_md5.h>
#include <library/cpp/digest/md5/md5.h>

#include <kernel/doom/algorithm/transfer.h>
#include <kernel/doom/simple_map/simple_map_reader.h>
#include <kernel/doom/simple_map/simple_map_writer.h>
#include <kernel/doom/wad/wad.h>

#include "offroad_ann_wad_io.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TOffroadWadTest) {

    using TMemoryIndex = TMap<TString, TVector<TReqBundleHit>>;

    using TSampler = TOffroadAnnWadIoSortedMultiKeys::TUniSampler;
    using TWriter = TOffroadAnnWadIoSortedMultiKeys::TWriter;
    using TSearcher = TOffroadAnnWadIoSortedMultiKeys::TSearcher;
    using TIterator = TSearcher::TIterator;

    TMemoryIndex MakeIndex(size_t keyCount, size_t hitCount, size_t seed) {
        TMemoryIndex result;

        ui64 state = seed;

        for (size_t i = 0; i < keyCount; i++) {
            TVector<TReqBundleHit> hits;
            for (size_t j = 0; j < hitCount; j++) {
                ui32 doc = state % 100;
                state = state * 982445797 + 1;
                ui32 relev = state % 10000;
                state = state * 982445797 + 1;

                hits.push_back(TReqBundleHit(doc, relev, 0, 0, 0, 0));
            }
            std::sort(hits.begin(), hits.end());

            result[ToString(i)] = std::move(hits);
        }

        return result;
    }

    Y_UNIT_TEST(TestSimple) {
        TMemoryIndex srcIndex = MakeIndex(100, 10, 31333);
        TMemoryIndex dstIndex;

        TSimpleMapReader<TReqBundleHit> src(srcIndex);
        TSimpleMapWriter<TReqBundleHit> dst(dstIndex);

        TSampler sampler;
        TransferIndex(&src, &sampler);
        auto models = sampler.Finish();
        src.Restart();

        TBufferStream memory;

        TWriter writer(models.first, models.second, &memory);
        TransferIndex(&src, &writer);
        writer.Finish();
        src.Restart();

        THolder<IWad> iwad(IWad::Open(TArrayRef<const char>(memory.Buffer().data(), memory.Buffer().size())));
        TSearcher searcher(iwad.Get());
        TIterator iterator;

        for (const auto& pair : srcIndex) {
            ui32 docId = -1;
            for (const TReqBundleHit& hit : pair.second) {
                TReqBundleHit hit0;
                bool success = false;

                if (hit.DocId() != docId) {
                    docId = hit.DocId();

                    success = iterator.ReadHit(&hit0);
                    UNIT_ASSERT(!success);

                    TVector<TOffroadWadKey> keys;
                    success = searcher.FindTerms(pair.first, &iterator, &keys);
                    UNIT_ASSERT(success);
                    UNIT_ASSERT_VALUES_EQUAL(keys.size(), 1);

                    success = searcher.Find(docId, keys[0].Id, &iterator);
                    UNIT_ASSERT(success);
                }

                success = iterator.ReadHit(&hit0);
                UNIT_ASSERT(success);
                UNIT_ASSERT_VALUES_EQUAL(hit0.Break(), hit.Break());
            }
        }

        UNIT_ASSERT_MD5_EQUAL(memory.Buffer(), "51e25a5d59ceef0f09b6c26fc94d7728");

        TBufferStream modelStream;
        models.first.Save(&modelStream);
        UNIT_ASSERT_MD5_EQUAL(modelStream.Buffer(), "52add842c721a55853357d638ec7c1f5");
        modelStream.Buffer().Clear();
        models.second.Save(&modelStream);
        UNIT_ASSERT_MD5_EQUAL(modelStream.Buffer(), "ca13d6ef16c4982a890f97647af630f7");
    }
}
