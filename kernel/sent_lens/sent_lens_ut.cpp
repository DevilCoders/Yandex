#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/buffer.h>

#include "sent_lens.h"
#include "sent_lens_writer.h"

Y_UNIT_TEST_SUITE(TSentenceLengthsTest) {
    static void TestPack(const TSentenceLengths& lengths0) {
        TBufferStream memOut;
        {
            TSentenceLengthsWriter writer(&memOut);
            writer.Add(0, lengths0);
        }
        {
            TBlob data = TBlob::FromStreamSingleThreaded(memOut);
            TSentenceLengthsReader reader(data);
            TSentenceLengths lengths;
            reader.Get(0, &lengths);
            UNIT_ASSERT_EQUAL(lengths0.size(), lengths.size());
            for (size_t i = 0; i < lengths0.size(); ++i)
                UNIT_ASSERT_EQUAL(lengths0[i], lengths[i]);

            TSentenceOffsets offsets;
            reader.GetOffsets(0, &offsets);
            UNIT_ASSERT_EQUAL(lengths0.size() + 1, offsets.size());
            ui32 sum = 0;
            for (size_t i = 0; i < offsets.size(); ++i) {
                UNIT_ASSERT_EQUAL(offsets[i], sum);
                if (i < +lengths)
                    sum += lengths[i];
            }
        }
    }

    Y_UNIT_TEST(SimpleTest) {
        TSentenceLengths lengths0;
        for (size_t i = 0; i < 64; ++i)
            lengths0.push_back(i);
        TestPack(lengths0);
    }

    Y_UNIT_TEST(ZeroTest) {
        TSentenceLengths lengths0;
        for (size_t i = 0; i < 6400; ++i)
            lengths0.push_back(0);
        TestPack(lengths0);
    }

    Y_UNIT_TEST(OnesTest) {
        TSentenceLengths lengths0;
        for (size_t i = 0; i < 6400; ++i)
            lengths0.push_back(1);
        TestPack(lengths0);
    }

    Y_UNIT_TEST(EmptyTest) {
        TSentenceLengths lengths0;
        TestPack(lengths0);
    }

    Y_UNIT_TEST(RandomTest) {
        size_t len = (rand() + 45335) % 10000;
        TSentenceLengths lengths0;
        for (size_t i = 0; i < len; ++i)
            lengths0.push_back(rand() % 64);
        TestPack(lengths0);
    }

    Y_UNIT_TEST(BatchRandomTest) {
        for (size_t j = 0; j < 100; ++j) {
            size_t len = (rand() + 45335) % 10000;
            TSentenceLengths lengths0;
            for (size_t i = 0; i < len; ++i)
                lengths0.push_back(rand() % 5);
            TestPack(lengths0);
        }
    }

    Y_UNIT_TEST(BatchRandomTest12) {
        for (size_t j = 0; j < 100; ++j) {
            size_t len = (rand() + 45335) % 10000;
            TSentenceLengths lengths0;
            for (size_t i = 0; i < len; ++i)
                lengths0.push_back((rand() % 2) + 1);
            TestPack(lengths0);
        }
    }

    Y_UNIT_TEST(BatchRandomTest12Small) {
        for (size_t j = 0; j < 1000; ++j) {
            size_t len = (rand() + 45335) % 3;
            TSentenceLengths lengths0;
            for (size_t i = 0; i < len; ++i)
                lengths0.push_back((rand() % 3));
            TestPack(lengths0);
        }
    }

    Y_UNIT_TEST(BatchTest) {
        size_t len = (rand() + 1000) % 10000;
        TSentenceLengths lengths0;
        for (size_t i = 0; i < len; ++i)
            lengths0.push_back(rand() % 64);

        const size_t N = 100;
        TBufferStream memOut;
        {
            TSentenceLengthsWriter writer(&memOut);
            for (size_t i = 0; i < N; ++i)
                writer.Add(i, lengths0);
        }
        {
            TBlob data = TBlob::FromStreamSingleThreaded(memOut);
            TSentenceLengthsReader reader(data);
            for (size_t i = 0; i < N; ++i) {
                TSentenceLengths lengths;
                reader.Get(i, &lengths);
                UNIT_ASSERT_EQUAL(lengths0.size(), lengths.size());
                for (size_t i = 0; i < 64; ++i)
                    UNIT_ASSERT_EQUAL(lengths0[i], lengths[i]);
            }
        }
    }
}
