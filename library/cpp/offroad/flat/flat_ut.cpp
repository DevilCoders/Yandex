#include <library/cpp/testing/unittest/registar.h>

#include <random>
#include <algorithm>

#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/stream/buffer.h>

#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/test/test_md5.h>
#include <library/cpp/digest/md5/md5.h>

#include "flat_writer.h"
#include "flat_searcher.h"
#include "flat_ui32_searcher.h"
#include "flat_ui64_searcher.h"

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TFlatTest) {
    using TWriter = TFlatWriter<TTestData, TTestData, TTestDataVectorizer, TTestDataVectorizer>;
    using TSearcher = TFlatSearcher<TTestData, TTestData, TTestDataVectorizer, TTestDataVectorizer>;

    template <class Index, class Writer>
    void WriteIndex(const Index& index, Writer* writer) {
        for (const auto& data : index)
            writer->Write(data);
    }

    Y_UNIT_TEST(TestEmpty) {
        TBufferStream stream;

        TWriter writer(&stream);
        writer.Finish();

        UNIT_ASSERT_VALUES_EQUAL(stream.Buffer().Size(), 0);

        TSearcher searcher(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));

        UNIT_ASSERT_VALUES_EQUAL(searcher.Size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(searcher.LowerBound(1000), 0);
    }

    Y_UNIT_TEST(TestUninitialized) {
        TSearcher searcher;

        UNIT_ASSERT_VALUES_EQUAL(searcher.Size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(searcher.LowerBound(1000), 0);
    }

    Y_UNIT_TEST(TestSimple) {
        auto index0 = MakeTestData(10000, 5555);
        auto index1 = MakeTestData(10000, 1234);
        std::sort(index0.begin(), index0.end());

        TBufferStream stream;

        TWriter writer(&stream);
        for (size_t i = 0; i < index0.size(); i++)
            writer.Write(index0[i], index1[i]);
        writer.Finish();

        TSearcher searcher(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));

        TVector<size_t> indices = xrange(index0.size());
        std::minstd_rand random(112323);
        std::shuffle(indices.begin(), indices.end(), random);

        for (size_t i : indices) {
            TTestData d0 = searcher.ReadKey(i);
            TTestData d1 = searcher.ReadData(i);

            UNIT_ASSERT_EQUAL(index0[i], d0);
            UNIT_ASSERT_EQUAL(index1[i], d1);

            size_t index = searcher.LowerBound(d0);

            UNIT_ASSERT_VALUES_EQUAL(index, i);
        }

        for (size_t from : indices) {
            for (size_t to = from; to <= indices.size() && to - from < 32; ++to) {
                for (size_t elem = Max<size_t>(from - 10, 0); elem < Min<size_t>(to + 10, indices.size()); ++elem) {
                    TTestData d = searcher.ReadKey(elem);
                    const size_t index = searcher.LowerBound(d, from, to);
                    const size_t expected = LowerBound(index0.begin() + from, index0.begin() + to, d) - index0.begin();
                    UNIT_ASSERT_EQUAL(expected, index);
                }
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(stream.Buffer().Size(), 52503);
        UNIT_ASSERT_MD5_EQUAL(stream.Buffer(), "6d08a52515de4ae5804696f2c2860f2d");
    }

    Y_UNIT_TEST(TestQuick) {
        std::vector<ui32> index;
        for (size_t i = 0; i < 10000; i++)
            index.push_back(i * 961748941 % 179424691);

        TBufferStream stream;

        TFlatWriter<ui32, std::nullptr_t, TUi32Vectorizer, TNullVectorizer> writer(&stream);
        for (size_t i = 0; i < index.size(); i++)
            writer.Write(index[i], nullptr);
        writer.Finish();

        TBlob blob = TBlob::NoCopy(stream.Buffer().data(), stream.Buffer().size());

        TFlatUi32Searcher flatSearcher(blob);
        for (size_t i = 0; i < index.size(); i++) {
            ui32 value = flatSearcher.ReadKey(i);

            UNIT_ASSERT_VALUES_EQUAL(value, index[i]);
        }
    }

    Y_UNIT_TEST(TestQuick64) {
        TBufferStream stream;

        TVector<ui32> index32(10000);
        for (size_t i = 0; i < index32.size(); ++i) {
            index32[i] = i * 961748941 % 179424691;
        }

        TFlatWriter<ui64, std::nullptr_t, TUi64Vectorizer, TNullVectorizer> writer(&stream);
        for (size_t i = 0; i < index32.size(); ++i) {
            writer.Write(index32[i], nullptr);
        }
        writer.Finish();

        TBlob blob = TBlob::NoCopy(stream.Buffer().data(), stream.Buffer().size());
        TFlatUi64Searcher searcher(blob);
        UNIT_ASSERT_VALUES_EQUAL(index32.size(), searcher.Size());
        for (size_t i = 0; i < index32.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(index32[i], searcher.ReadKey(i));
        }

        stream.Buffer().Clear();
        writer.Reset(&stream);

        TVector<ui64> index64(12345);
        for (size_t i = 0; i < index64.size(); ++i) {
            index64[i] = (i * 9617489411337ULL) & ScalarMask(56);
        }

        for (size_t i = 0; i < index64.size(); ++i) {
            writer.Write(index64[i], nullptr);
        }
        writer.Finish();

        blob = TBlob::NoCopy(stream.Buffer().data(), stream.Buffer().size());
        searcher.Reset(blob);
        UNIT_ASSERT_VALUES_EQUAL(index64.size(), searcher.Size());
        for (size_t i = 0; i < index64.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(index64[i], searcher.ReadKey(i));
        }

        stream.Buffer().Clear();
        writer.Reset(&stream);

        for (size_t i = 0; i < index64.size(); ++i) {
            index64[i] = ((i * 9617489411337ULL) & ScalarMask(28)) | ((i * 9617489411337ULL) & (~ScalarMask(32)));
        }

        for (size_t i = 0; i < index64.size(); ++i) {
            writer.Write(index64[i], nullptr);
        }
        writer.Finish();

        blob = TBlob::NoCopy(stream.Buffer().data(), stream.Buffer().size());
        searcher.Reset(blob);
        UNIT_ASSERT_VALUES_EQUAL(index64.size(), searcher.Size());
        for (size_t i = 0; i < index64.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(index64[i], searcher.ReadKey(i));
        }
    }

    class TTestDataDumbVectorizer {
    public:
        enum {
            TupleSize = 1
        };

        template <class Slice>
        static void Gather(Slice&& slice, TTestData* tData) {
            *tData = ((ui64)slice[0]) << 32;
        }

        template <class Slice>
        static void Scatter(TTestData tData, Slice&& slice) {
            slice[0] = tData >> 32;
        }
    };

    Y_UNIT_TEST(TestUnique) {
        TVector<TTestData> index, index2;
        TBufferStream stream;
        TFlatWriter<TTestData, TTestData, TTestDataDumbVectorizer, TTestDataVectorizer, UniqueFlatType> writer(&stream);
        for (size_t i = 0; i < 10000; ++i) {
            ui64 tmp = i / 10;
            tmp <<= 32;
            tmp += i;
            index.push_back(tmp);
        }
        index2 = MakeTestData(10000, 1337);
        for (size_t i = 0; i < 10000; ++i) {
            writer.Write(index[i], index2[i]);
            UNIT_ASSERT_VALUES_EQUAL(writer.Size(), (i / 10) + 1);
        }
        writer.Finish();
        TFlatSearcher<TTestData, TTestData, TTestDataDumbVectorizer, TTestDataVectorizer> searcher(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        for (size_t i = 0; i < searcher.Size(); i++) {
            UNIT_ASSERT_VALUES_EQUAL(searcher.ReadKey(i), (index[10 * i + 9] >> 32) << 32);
            UNIT_ASSERT_VALUES_EQUAL(searcher.ReadData(i), index2[10 * i + 9]);
        }
    }

    /**
     * In TFlatSearcher::LoadBits was bug
     * ui64 LoadBits(const void* ptr, ui64 offset) {
     *     const ui64 result = *reinterpret_cast<const ui64*>(static_cast<const char*>(ptr) + offset / 8);
     *     return result >> (offset % 8);
     * }
     *
     * It's not correct because it's possible offset + 8 > size of data region of searcher
     * This test checks that new implementation works correctly
     */
    Y_UNIT_TEST(TestLoadBits) {
        TBufferStream stream;
        TFlatWriter<ui32, ui64, TUi32Vectorizer, TUi64Vectorizer> writer(&stream);
        UNIT_ASSERT_VALUES_EQUAL(3, TUi32Vectorizer::TupleSize + TUi64Vectorizer::TupleSize);
        writer.Write(1, 31);
        writer.Write((ui32(1) << 15) - 1, (((ui64(1) << 5) - 1) << 32) | 5);
        writer.Finish();
        UNIT_ASSERT_VALUES_EQUAL(9, stream.Buffer().Size());
        const ui64 header = *reinterpret_cast<const ui64*>(stream.Buffer().data());
        UNIT_ASSERT_VALUES_EQUAL(ui64(15) | (ui64(5) << 6) | (ui64(5) << 12), (header & ((ui64(1) << 18) - 1)));
        TFlatSearcher<ui32, ui64, TUi32Vectorizer, TUi64Vectorizer> searcher(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        UNIT_ASSERT_VALUES_EQUAL(2, searcher.Size());
        UNIT_ASSERT_VALUES_EQUAL(1, searcher.ReadKey(0));
        UNIT_ASSERT_VALUES_EQUAL(31, searcher.ReadData(0));
        UNIT_ASSERT_VALUES_EQUAL((ui32(1) << 15) - 1, searcher.ReadKey(1));
        UNIT_ASSERT_VALUES_EQUAL((((ui64(1) << 5) - 1) << 32) | 5, searcher.ReadData(1));
    }

    Y_UNIT_TEST(TestSmallHeader) {
        TBufferStream stream;
        TFlatWriter<ui32, std::nullptr_t, TUi32Vectorizer, TNullVectorizer> writer(&stream);
        writer.Write(1, nullptr);
        writer.Finish();
        UNIT_ASSERT_VALUES_EQUAL(2, stream.Buffer().Size());
        TFlatSearcher<ui32, std::nullptr_t, TUi32Vectorizer, TNullVectorizer> searcher(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        UNIT_ASSERT_VALUES_EQUAL(1, searcher.Size());
        UNIT_ASSERT_VALUES_EQUAL(1, searcher.ReadKey(0));
    }
}
