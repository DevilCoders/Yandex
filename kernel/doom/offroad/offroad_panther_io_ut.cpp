#include <library/cpp/testing/unittest/registar.h>

#include <random>

#include <util/generic/algorithm.h>
#include <util/stream/str.h>

#include <library/cpp/digest/md5/md5.h>

#include <kernel/doom/algorithm/transfer.h>
#include <kernel/doom/simple_map/simple_map_reader.h>
#include <kernel/doom/simple_map/simple_map_writer.h>

#include "offroad_panther_io.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TOffroadPantherTest) {

    using TMemoryIndex = TMap<TString, TVector<TPantherHit>>;

    TMemoryIndex MakeIndex(size_t keyCount, size_t hitCount, size_t seed) {
        TMemoryIndex result;

        ui64 state = seed;

        for (size_t i = 0; i < keyCount; i++) {
            TVector<TPantherHit> hits;
            for (size_t j = 0; j < hitCount; j++) {
                ui32 doc = state % 1000;
                state = state * 982445797 + 1;
                ui32 relev = state % 10000;
                state = state * 982445797 + 1;

                hits.push_back(TPantherHit(doc, relev));
            }
            std::sort(hits.begin(), hits.end());

            result[ToString(i)] = std::move(hits);
        }

        return result;
    }

    Y_UNIT_TEST(TestSimple) {
        TMemoryIndex srcIndex = MakeIndex(10000, 1000, 31333);
        TMemoryIndex dstIndex;

        TSimpleMapReader<TPantherHit> src(srcIndex);
        TSimpleMapWriter<TPantherHit> dst(dstIndex);

        TOffroadPantherIo::THitSampler sampler0;
        TransferIndex(&src, &sampler0);
        auto hitModel = sampler0.Finish();
        src.Restart();

        TOffroadPantherIo::TKeySampler sampler1(hitModel);
        TransferIndex(&src, &sampler1);
        auto keyModel = sampler1.Finish();
        src.Restart();

        TOffroadPantherIo::TWriter writer(hitModel, keyModel, "indexpanther.");
        TransferIndex(&src, &writer);
        writer.Finish();
        src.Restart();

        TOffroadPantherIo::TReader reader("indexpanther.");
        TransferIndex(&reader, &dst);

        UNIT_ASSERT_EQUAL(srcIndex, dstIndex);

        TStringStream stream;
        for (const auto& pair : srcIndex) {
            stream << pair.first;
            for (const auto& hit : pair.second)
                stream << hit.DocId() << hit.Relevance();
        }

#if 0
        Cerr << MD5::Calc(stream.Str()) << Endl;
        Cerr << MD5::File("indexpanther.inv.model") << Endl;
        Cerr << MD5::File("indexpanther.inv") << Endl;
        Cerr << MD5::File("indexpanther.key.model") << Endl;
        Cerr << MD5::File("indexpanther.key") << Endl;
        Cerr << MD5::File("indexpanther.fat") << Endl;
        Cerr << MD5::File("indexpanther.fat.idx") << Endl;
#endif

        UNIT_ASSERT_VALUES_EQUAL(MD5::Calc(stream.Str()),               "9551b05845d4d828f03cf6f8a8085c62");
        UNIT_ASSERT_VALUES_EQUAL(MD5::File("indexpanther.inv.model"),   "6c3d774647145801a50fa7798ad2896e");
        UNIT_ASSERT_VALUES_EQUAL(MD5::File("indexpanther.inv"),         "93a9efc582838fa20b68799c250baa31");
        UNIT_ASSERT_VALUES_EQUAL(MD5::File("indexpanther.key.model"),   "d7a90b20ef0a9e6a8af040a091f5df44");
        UNIT_ASSERT_VALUES_EQUAL(MD5::File("indexpanther.key"),         "b02eeb96e18d7ce7d54cb1bdb298d0a2");
        UNIT_ASSERT_VALUES_EQUAL(MD5::File("indexpanther.fat"),         "d350f6a4a118964b1d281a5f4e1f527b");
        UNIT_ASSERT_VALUES_EQUAL(MD5::File("indexpanther.fat.idx"),     "bf35308c082b4ac67e00a1d6d7df3322");
    }
}
