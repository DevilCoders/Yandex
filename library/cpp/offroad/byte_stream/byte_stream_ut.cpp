#include <library/cpp/testing/unittest/registar.h>

#include "byte_input_stream.h"
#include "byte_output_stream.h"

#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/str.h>

#include <random>

using namespace NOffroad;
using namespace NOffroad::NPrivate;
using namespace std::string_view_literals;

Y_UNIT_TEST_SUITE(TOffroadByteStream) {
    TString GenerateData(ui32 size, EBufferType type) {
        TString data;
        data.reserve(size);
        std::mt19937 gen(42);
        auto randomChar = [&]() {
            char a = gen();
            while (type != PlainOldBuffer && !a)
                a = static_cast<char>(gen());
            return a;
        };
        generate_n(std::back_inserter(data), size, randomChar);
        return data;
    }

    template <class SampleStream, class OutputStream>
    typename SampleStream::TModel EncodeAllData(const TString& data, TBufferOutput* output) {
        typename SampleStream::TModel model;
        SampleStream sampleStream(&model);
        sampleStream << data;
        sampleStream.Finish();
        typename OutputStream::TTable table(model);
        OutputStream writeStream(&table, output);
        writeStream << data;
        writeStream.Finish();
        return model;
    }

    template <class SampleStream, class OutputStream>
    typename SampleStream::TModel EncodeDataSeqWrites(const TString& data, TBufferOutput* output, size_t len, bool random = false) {
        typename SampleStream::TModel model;
        TBuffer buffer(2048);
        std::mt19937 gen(42);
        if (random) {
            len = gen();
        }
        TStringStream dataStream(data);
        SampleStream sampleStream(&model);
        while (size_t count = dataStream.Read(buffer.data(), len)) {
            sampleStream.Write(buffer.data(), count);
        }

        sampleStream.Finish();
        typename OutputStream::TTable table(model);
        OutputStream writeStream(&table, output);
        dataStream = TStringStream(data);
        while (size_t count = dataStream.Read(buffer.data(), len)) {
            writeStream.Write(buffer.data(), count);
        }
        writeStream.Finish();
        return model;
    }

    template <class InputStream>
    void CheckDecode(const TBufferOutput& compressed, const TString& data, const typename InputStream::TModel& model) {
        typename InputStream::TTable table(model);
        InputStream input(&table, TArrayRef<const char>(compressed.Buffer().data(), compressed.Buffer().size()));
        TString decompressedData = input.ReadAll();
        if (InputStream::BufferType == PlainOldBuffer) {
            UNIT_ASSERT_STRINGS_EQUAL(data + TString(decompressedData.size() - data.size(), '\0'), decompressedData);
        } else
            UNIT_ASSERT_STRINGS_EQUAL(data, decompressedData);
    }

    template <class InputStream>
    void CheckDecodeSeqReads(const TBufferOutput& compressed, const TString& data, const typename InputStream::TModel& model, size_t len, bool random = false) {
        typename InputStream::TTable table(model);
        InputStream input(&table, TArrayRef<const char>(compressed.Buffer().data(), compressed.Buffer().size()));
        TStringStream decompressedStream;
        TBuffer buffer(2048);
        std::mt19937 gen(42);
        if (random) {
            len = gen();
        }
        size_t allBytes = 0;
        while (size_t count = input.Read(buffer.data(), len)) {
            decompressedStream.Write(buffer.data(), count);
            if (InputStream::BufferType == PlainOldBuffer) {
                size_t checkSize = Min(data.size(), allBytes + count);
                UNIT_ASSERT_STRINGS_EQUAL(data.substr(0, checkSize), decompressedStream.Str().substr(0, checkSize));
            } else {
                UNIT_ASSERT_STRINGS_EQUAL(data.substr(allBytes, count), decompressedStream.Str().substr(allBytes, count));
            }
            allBytes += count;
        }
        decompressedStream.Flush();
        if (InputStream::BufferType == PlainOldBuffer)
            UNIT_ASSERT_STRINGS_EQUAL(data + TString(decompressedStream.Str().size() - data.size(), '\0'), decompressedStream.Str());
        else
            UNIT_ASSERT_STRINGS_EQUAL(data, decompressedStream.Str());
    }

    template <class SampleStream, class OutputStream, class InputStream>
    void RunTest() {
        for (size_t i = 1; i < 257; ++i) {
            const TString data = GenerateData(i, SampleStream::BufferType);
            TBufferOutput output;
            typename SampleStream::TModel model = EncodeAllData<SampleStream, OutputStream>(data, &output);
            CheckDecode<InputStream>(output, data, model);
        }
    }

    template <class SampleStream, class OutputStream, class InputStream>
    void RunRandomWritesTest() {
        for (size_t i = 0; i < 257; ++i) {
            const TString data = GenerateData(i, SampleStream::BufferType);
            TBufferOutput output;
            typename SampleStream::TModel model = EncodeDataSeqWrites<SampleStream, OutputStream>(data, &output, true);
            CheckDecode<InputStream>(output, data, model);
        }
    }

    template <class SampleStream, class OutputStream, class InputStream>
    void RunRandomReadsWritesTest() {
        for (size_t i = 0; i < 257; ++i) {
            const TString data = GenerateData(i, SampleStream::BufferType);
            TBufferOutput output;
            typename SampleStream::TModel model = EncodeDataSeqWrites<SampleStream, OutputStream>(data, &output, true);
            CheckDecodeSeqReads<InputStream>(output, data, model, true);
        }
    }

    template <class SampleStream, class OutputStream, class InputStream>
    void RunRandomReadsTest() {
        for (size_t i = 0; i < 257; ++i) {
            const TString data = GenerateData(i, SampleStream::BufferType);
            TBufferOutput output;
            typename SampleStream::TModel model = EncodeAllData<SampleStream, OutputStream>(data, &output);
            CheckDecodeSeqReads<InputStream>(output, data, model, true);
        }
    }

    template <class SampleStream, class OutputStream, class InputStream>
    void RunSeqReadsTest(size_t len) {
        for (size_t i = 0; i < 257; ++i) {
            const TString data = GenerateData(i, SampleStream::BufferType);
            TBufferOutput output;
            typename SampleStream::TModel model = EncodeAllData<SampleStream, OutputStream>(data, &output);
            CheckDecodeSeqReads<InputStream>(output, data, model, len);
        }
    }

    template <class SampleStream, class OutputStream, class InputStream>
    void RunSeqWritesTest(size_t len) {
        for (size_t i = 0; i < 257; ++i) {
            const TString data = GenerateData(i, SampleStream::BufferType);
            TBufferOutput output;
            typename SampleStream::TModel model = EncodeDataSeqWrites<SampleStream, OutputStream>(data, &output, len);
            CheckDecode<InputStream>(output, data, model);
        }
    }

    template <class SampleStream, class OutputStream, class InputStream>
    void RunSeqReadsWritesTest(size_t len1, size_t len2) {
        for (size_t i = 0; i < 257; ++i) {
            const TString data = GenerateData(i, SampleStream::BufferType);
            TBufferOutput output;
            typename SampleStream::TModel model = EncodeDataSeqWrites<SampleStream, OutputStream>(data, &output, len1);
            CheckDecodeSeqReads<InputStream>(output, data, model, len2);
        }
    }

    Y_UNIT_TEST(PlainStream) {
        RunTest<TByteSampleStream, TByteOutputStream, TByteInputStream>();
    }

    Y_UNIT_TEST(EofStream) {
        RunTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>();
    }

    Y_UNIT_TEST(PlainRandomReads) {
        RunRandomReadsTest<TByteSampleStream, TByteOutputStream, TByteInputStream>();
    }

    Y_UNIT_TEST(EofRandomReads) {
        RunRandomReadsTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>();
    }

    Y_UNIT_TEST(PlainRandomReadsWrites) {
        RunRandomReadsWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>();
    }

    Y_UNIT_TEST(EofRandomReadsWrites) {
        RunRandomReadsWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>();
    }

    Y_UNIT_TEST(PlainRandomWrites) {
        RunRandomWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>();
    }

    Y_UNIT_TEST(EofRandomWrites) {
        RunRandomWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>();
    }

    Y_UNIT_TEST(PlainSeqReads) {
        RunSeqReadsTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(1);
        RunSeqReadsTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(2);
        RunSeqReadsTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(3);
        RunSeqReadsTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(64);
        RunSeqReadsTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(63);
        RunSeqReadsTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(128);
    }

    Y_UNIT_TEST(EofSeqReads) {
        RunSeqReadsTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(1);
        RunSeqReadsTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(2);
        RunSeqReadsTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(3);
        RunSeqReadsTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(64);
        RunSeqReadsTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(63);
        RunSeqReadsTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(128);
    }

    Y_UNIT_TEST(PlainSeqReadsWrites) {
        RunSeqReadsWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(1, 1);
        RunSeqReadsWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(1, 2);
        RunSeqReadsWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(2, 2);
        RunSeqReadsWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(2, 1);
    }

    Y_UNIT_TEST(EofSeqReadsWrites) {
        RunSeqReadsWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(1, 1);
        RunSeqReadsWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(1, 2);
        RunSeqReadsWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(2, 2);
        RunSeqReadsWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(2, 1);
    }

    Y_UNIT_TEST(PlainSeqWrites) {
        RunSeqWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(1);
        RunSeqWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(2);
        RunSeqWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(3);
        RunSeqWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(64);
        RunSeqWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(63);
        RunSeqWritesTest<TByteSampleStream, TByteOutputStream, TByteInputStream>(128);
    }

    Y_UNIT_TEST(EofSeqWrites) {
        RunSeqWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(1);
        RunSeqWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(2);
        RunSeqWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(3);
        RunSeqWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(64);
        RunSeqWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(63);
        RunSeqWritesTest<TByteSampleStreamEof, TByteOutputStreamEof, TByteInputStreamEof>(128);
    }

    Y_UNIT_TEST(CheckAutoZeroesInput) {
        constexpr TStringBuf data = "abcdefg\0asdasf"sv;
        typename TByteSampleStreamEof::TModel model;
        TByteSampleStreamEof sampleStream(&model);
        UNIT_ASSERT_EXCEPTION(sampleStream << data, yexception);
    }
    Y_UNIT_TEST(CheckZeroesInput) {
        constexpr TStringBuf data = "abcdefg\0asdasf"sv;
        typename TByteSampleStream::TModel model;
        TByteSampleStream sampleStream(&model);
        UNIT_ASSERT_NO_EXCEPTION(sampleStream << data);
    }

    template <class WriteStream, class Model>
    void WriteAndCheck(const Model& model, const TVector<TVector<unsigned char>>& dataToWrite, WriteStream& writeStream, TBufferStream& stream) {
        TVector<TDataOffset> seekPoints;
        for (const auto& chunk : dataToWrite) {
            seekPoints.push_back(writeStream.Position());
            writeStream.Write(chunk.data(), chunk.size());
        }
        writeStream.Finish();

        typename TByteInputStream::TTable table(model);
        TByteInputStream inStream(&table, TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        for (size_t i = 0; i < seekPoints.size(); ++i) {
            UNIT_ASSERT(inStream.Seek(seekPoints[i]));
            const auto& buf = dataToWrite[i];
            TVector<unsigned char> temp(buf.size());
            inStream.Read(temp.data(), buf.size());
            UNIT_ASSERT_VALUES_EQUAL(buf, temp);
        }
    }

    Y_UNIT_TEST(CheckOutputPositionOffset) {
        using TSampleStream = TByteSampleStream;
        using IOutputStream = TByteOutputStream;

        static constexpr size_t MIN = 1, MAX = 16, LEN = 4;
        size_t arr[LEN] = {MIN, MIN, MIN, MIN};
        auto genNext = [&arr]() {
            for (size_t i = LEN; i--;) {
                if (arr[i] < MAX) {
                    ++arr[i];
                    for (size_t j = i + 1; j < LEN; ++j)
                        arr[j] = MIN;
                    return true;
                }
            }
            return false;
        };

        std::mt19937 gen(1408);

        typename TSampleStream::TModel model;
        TSampleStream sampleStream(&model);

        for (size_t i = 0; i < 1000; ++i) {
            size_t len = gen() & 127 + 50;
            TVector<unsigned char> temp(len);
            temp.resize(len);
            for (size_t j = 0; j < len; ++j) {
                temp[j] = gen() & 255;
            }
            sampleStream.Write(temp.data(), len);
        }
        sampleStream.Finish();
        typename IOutputStream::TTable table(model);

        do {
            TVector<TVector<unsigned char>> data;
            TBufferStream stream;
            IOutputStream writeStream(&table, &stream);
            for (size_t i = 0; i < LEN; ++i) {
                size_t len = arr[i];
                TVector<unsigned char> tmp(len);
                for (size_t j = 0; j < len; ++j) {
                    tmp[j] = gen() & 255;
                }
                data.emplace_back(std::move(tmp));
            }
            WriteAndCheck(model, data, writeStream, stream);
        } while (genNext());
    }

    Y_UNIT_TEST(CheckSeekInput) {
        using TSampleStream = TByteSampleStream;
        using IOutputStream = TByteOutputStream;
        std::mt19937 gen(123);

        for (size_t testSize = 1; testSize < 250; ++testSize) {
            typename TSampleStream::TModel model;
            TSampleStream sampleStream(&model);
            TVector<TVector<unsigned char>> data;
            for (size_t i = 0; i < testSize; ++i) {
                size_t len = gen() % 100 + 10;
                TVector<unsigned char> temp(len);
                for (size_t j = 0; j < len; ++j) {
                    temp[j] = gen() & 255;
                }
                data.emplace_back(temp);
            }

            for (const auto& chunk : data) {
                sampleStream.Write(chunk.data(), chunk.size());
            }
            sampleStream.Finish();

            typename IOutputStream::TTable table(model);
            TBufferStream stream;
            IOutputStream writeStream(&table, &stream);
            WriteAndCheck(model, data, writeStream, stream);
        }
    }
}
