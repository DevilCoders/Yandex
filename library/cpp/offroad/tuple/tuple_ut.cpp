#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/stream/buffer.h>

#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/codec/bit_decoder_16.h>
#include <library/cpp/offroad/codec/bit_encoder_16.h>
#include <library/cpp/offroad/codec/bit_sampler_16.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/streams/bit_input.h>
#include <library/cpp/offroad/streams/bit_output.h>
#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/test/test_md5.h>
#include <library/cpp/digest/md5/md5.h>

#include "adaptive_tuple_reader.h"
#include "adaptive_tuple_writer.h"
#include "tuple_reader.h"
#include "tuple_writer.h"
#include "tuple_sampler.h"

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TTupleTest) {
    template <class Reader, class Writer, class Sampler>
    void DoTestSimple2(const TString& streamMd5, const TString& modelMd5) {
        TVector<ui64> hits = MakeTestData(1024, 0);

        Sampler sampler;
        for (ui64 hit : hits)
            sampler.WriteHit(hit);
        auto model = sampler.Finish();
        auto tables = NewMultiTable(model);

        TBufferStream stream;
        Writer writer(tables.Get(), &stream);

        for (ui64 hit : hits)
            writer.WriteHit(hit);
        writer.Finish();

        Reader reader(tables.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        for (size_t i = 0; i < hits.size(); i++) {
            ui64 hit = 0;
            bool success = reader.ReadHit(&hit);

            UNIT_ASSERT(success);
            UNIT_ASSERT_VALUES_EQUAL(hit, hits[i]);
        }

        UNIT_ASSERT_MD5_EQUAL(stream.Buffer(), streamMd5);

        TBufferStream modelStream;
        model.Save(&modelStream);
        UNIT_ASSERT_MD5_EQUAL(modelStream.Buffer(), modelMd5);
    }

    template <class Reader>
    void DoTestEmptyRead() {
        Reader reader0;
        for (size_t i = 0; i < 10; i++) {
            ui64 data;
            bool success = reader0.ReadHit(&data);
            UNIT_ASSERT(!success);
        }

        Reader reader1;
        for (size_t i = 0; i < 10; i++) {
            bool success = reader1.ReadHits([](ui64) { return true; });
            UNIT_ASSERT(!success);
        }
    }

    Y_UNIT_TEST(TestEmptyReadWithBlockIo) {
        DoTestEmptyRead<NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor>>();
    }

    Y_UNIT_TEST(TestEmptyReadWithBitIo) {
        DoTestEmptyRead<NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor, TBitDecoder16>>();
    }

    Y_UNIT_TEST(TestWithBlockIo) {
        DoTestSimple2<
            NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor>,
            NOffroad::TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor>,
            NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor>>("6fbe1e8a8fa7b691229cf7ea2efcb672", "51746a3d624a5a1d7d143cb4a17d0cdd");
    }

    Y_UNIT_TEST(TestWithBitIo) {
        DoTestSimple2<
            NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor, TBitDecoder16>,
            NOffroad::TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor, TBitEncoder16>,
            NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor, TBitSampler16>>("bb1235e2ff74de4e7967021cd70c5137", "51746a3d624a5a1d7d143cb4a17d0cdd");
    }

    Y_UNIT_TEST(TestWithMultiBitIo) {
        DoTestSimple2<
            NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor, TBitDecoder16, 13>,
            NOffroad::TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor, TBitEncoder16, 7>,
            NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor, TBitSampler16>>("bb1235e2ff74de4e7967021cd70c5137", "51746a3d624a5a1d7d143cb4a17d0cdd");
    }

    Y_UNIT_TEST(TestPositions) {
        using TWriter = NOffroad::TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor>;
        using TReader = NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor>;
        using TSampler = NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor>;

        TVector<ui64> hits = MakeTestData(1024, 0);

        TSampler sampler;
        for (ui64 hit : hits)
            sampler.WriteHit(hit);
        auto table = NewMultiTable(sampler.Finish());

        TBufferStream stream;
        TWriter writer(table.Get(), &stream);

        TVector<std::pair<TDataOffset, ui64>> poshits;

        for (ui64 hit : hits) {
            poshits.push_back({writer.Position(), hit});
            writer.WriteHit(hit);
        }
        writer.Finish();

        TReader reader(table.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));

        /* Sequential. */
        for (const auto& poshit : poshits) {
            ui64 decompressedHit = 0;
            bool success = reader.ReadHit(&decompressedHit);

            UNIT_ASSERT(success);
            UNIT_ASSERT_VALUES_EQUAL(poshit.second, decompressedHit);
        }

        /* With skips. */
        for (size_t s = 0; s < 2; s++) {
            size_t skip = s == 0 ? 13 : 277;

            TReader reader2(table.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));

            for (size_t i = 0; i < poshits.size(); i += skip) {
                const auto& poshit = poshits[i];

                bool success1 = reader2.Seek(poshit.first, 0, TSeekPointSeek());
                UNIT_ASSERT(success1);

                ui64 decompressedHit = 0;
                bool success2 = reader2.ReadHit(&decompressedHit);
                UNIT_ASSERT(success2);

                UNIT_ASSERT_VALUES_EQUAL(poshit.second, decompressedHit);
            }
        }
    }

    Y_UNIT_TEST(TestBlockMultiplier) {
        using TReader = NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor, TBitDecoder16, 4>;
        using TWriter = NOffroad::TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor, TBitEncoder16, 4>;
        using TSampler = NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor, TBitSampler16>;

        const size_t testSize = 200;

        std::vector<ui64> testData;
        for (size_t i = 0; i < testSize; i++)
            testData.push_back(static_cast<ui64>(i + 1) | (static_cast<ui64>(i + 1) << 32));

        TSampler sampler;
        for (ui64 value : testData)
            sampler.WriteHit(value);
        auto table = NewMultiTable(sampler.Finish());

        TBufferStream stream;

        for (size_t size = 0; size < 200; size++) {
            stream.Buffer().Clear();

            TWriter writer(table.Get(), &stream);
            for (size_t i = 0; i < size; i++)
                writer.WriteHit(testData[i]);
            writer.Finish();

            TReader reader(table.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
            size_t i = 0;
            while (true) {
                ui64 value;
                if (!reader.ReadHit(&value))
                    break;

                UNIT_ASSERT(i < size);
                UNIT_ASSERT_VALUES_EQUAL(testData[i], value);
                i++;
            }
            UNIT_ASSERT_VALUES_EQUAL(i, size);
        }
    }

    Y_UNIT_TEST(TestFarSeek) {
        using TWriter = NOffroad::TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor>;
        using TReader = NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor>;
        using TSampler = NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor>;

        std::vector<ui64> testData;
        for (size_t i = 0; i < 8; i++)
            testData.push_back(i + 1);

        TSampler sampler;
        for (ui64 value : testData)
            sampler.WriteHit(value);
        auto table = NewMultiTable(sampler.Finish());

        TBufferStream stream;

        TWriter writer(table.Get(), &stream);
        for (ui64 value : testData)
            writer.WriteHit(value);
        writer.Finish();

        TReader reader(table.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));

        for (size_t k = 0; k < 2; k++) {
            ui64 tmp;
            bool success = false;
            success = reader.Seek(TDataOffset(0, 0), 0);
            UNIT_ASSERT(success);

            success = reader.ReadHit(&tmp);
            UNIT_ASSERT(success);
            UNIT_ASSERT_VALUES_EQUAL(tmp, 1);

            success = reader.Seek(TDataOffset(0, 8), 0);
            UNIT_ASSERT(success); /* Seek-at-end. */

            for (size_t i = 0; i < 100; i++) {
                success = reader.ReadHit(&tmp);
                UNIT_ASSERT(!success);
            }

            for (size_t i = 9; i < 63; i++) {
                success = reader.Seek(TDataOffset(0, i), 0);
                UNIT_ASSERT(!success);
            }
        }
    }

    Y_UNIT_TEST(TestPlainOldBuffer) {
        TTupleOutputBuffer<ui64, TTestDataVectorizer, TINSubtractor, 128, PlainOldBuffer> buffer;

        UNIT_ASSERT_NO_EXCEPTION(buffer.WriteHit(0));
        UNIT_ASSERT_NO_EXCEPTION(buffer.WriteHit(0));
        UNIT_ASSERT_NO_EXCEPTION(buffer.WriteHit(0));
        UNIT_ASSERT_NO_EXCEPTION(buffer.WriteHit(0));

        UNIT_ASSERT_VALUES_EQUAL(buffer.BlockPosition(), 4);
    }

    Y_UNIT_TEST(TestPlainOldBufferTuple) {
        using TReader = NOffroad::TTupleReader<ui64, TTestDataVectorizer, TINSubtractor, TBitDecoder16, 4, PlainOldBuffer>;
        using TWriter = NOffroad::TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor, TBitEncoder16, 4, PlainOldBuffer>;
        using TSampler = NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor, TBitSampler16, PlainOldBuffer>;

        TSampler sampler;
        for (size_t i = 0; i < 100; i++)
            for (size_t j = 0; j < 16; j++)
                sampler.WriteHit(i * 3);

        TBufferStream source;
        auto model = sampler.Finish();
        auto table = NewMultiTable(model);

        TWriter writer(table.Get(), &source);
        for (size_t i = 0; i < 100; i++)
            for (size_t j = 0; j < 16; j++)
                writer.WriteHit(i * 3);
        writer.Finish();

        TReader reader(table.Get(), TArrayRef<const char>(source.Buffer().data(), source.Buffer().size()));
        ui64 data;
        for (size_t i = 0; i < 100; i++) {
            for (size_t j = 0; j < 16; j++) {
                reader.ReadHit(&data);
                UNIT_ASSERT_VALUES_EQUAL(data, i * 3);
            }
        }

        while (reader.ReadHit(&data))
            UNIT_ASSERT_VALUES_EQUAL(data, 0);
    }

    Y_UNIT_TEST(TestAdaptiveTupleNoPrefix) {
        using TReader = NOffroad::TAdaptiveTupleReader<ui64, TTestDataVectorizer, TINSubtractor>;
        using TWriter = NOffroad::TAdaptiveTupleWriter<ui64, TTestDataVectorizer, TINSubtractor>;
        using TSampler = NOffroad::TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor>;

        for (size_t hitsCount = 0; hitsCount <= 128; ++hitsCount) {
            TSampler sampler;
            for (size_t i = 0; i < hitsCount; ++i) {
                sampler.WriteHit(i + 1);
            }

            TBufferStream source;
            auto model = sampler.Finish();
            auto table = NewMultiTable(model);

            TWriter writer(table.Get(), &source);
            for (size_t i = 0; i < hitsCount; ++i) {
                writer.WriteHit(i + 1);
            }
            writer.Finish();

            TReader reader(table.Get(), TArrayRef<const char>(source.Buffer().data(), source.Buffer().size()));
            ui64 data;
            for (size_t i = 0; i < hitsCount; ++i) {
                UNIT_ASSERT(reader.ReadHit(&data));
                UNIT_ASSERT_VALUES_EQUAL(data, i + 1);
            }

            UNIT_ASSERT(!reader.ReadHit(&data));
        }
    }
}
