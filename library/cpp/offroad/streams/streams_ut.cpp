#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

#include "vec_input.h"
#include "vec_output.h"

#include "bit_input.h"
#include "bit_output.h"

#include "vec_input.h"
#include "vec_memory_input.h"
#include "vec_output.h"

using namespace NOffroad;
using namespace NOffroad::NPrivate;

Y_UNIT_TEST_SUITE(TScalarBlockTest) {
    Y_UNIT_TEST(TestBits) {
        ui64 v = 0;
        for (size_t bits = 0; bits < 65; ++bits) {
            for (size_t i = 0; i < 100; ++i) {
                TBufferStream stream;
                TBitOutput inStream(&stream);
                std::vector<ui64> elements;
                ui64 mask = ScalarMasks.Values[bits];
                for (size_t j = 0; j < i; ++j) {
                    v = v * 5 + 1;
                    elements.push_back(v & mask);
                    inStream.Write(v & mask, bits);
                }
                inStream.Finish();
                TBitInput outStream(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
                for (size_t j = 0; j < i; ++j) {
                    outStream.Seek(j * bits);
                    UNIT_ASSERT_VALUES_EQUAL(bits, outStream.Read(&v, bits));
                    UNIT_ASSERT_VALUES_EQUAL(v & mask, elements[j]);
                }
            }
        }
    }

    Y_UNIT_TEST(TestAlign64) {
        TBufferStream stream;
        TBitOutput out(&stream);
        out.Write(42, 64);
        out.Write(42, 64);
        out.Finish();
        TBitInput in(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        ui64 x;

        UNIT_ASSERT_EQUAL(64, in.Read(&x, 64));
        UNIT_ASSERT_EQUAL(42, x);
        UNIT_ASSERT_EQUAL(64, in.Read(&x, 64));
        UNIT_ASSERT_EQUAL(42, x);
    }

    Y_UNIT_TEST(TestBitssss) {
        ui64 r = 0;
        for (size_t bits = 0; bits < 33; ++bits) {
            for (size_t i = 0; i < 100; ++i) {
                TBufferStream stream;
                TBitOutput inStream(&stream);
                std::vector<TVec4u> elements;
                TVec4u mask = VectorMasks.Values[bits];
                for (size_t j = 0; j < i; ++j) {
                    r = r * 5 + 1;
                    TVec4u v(r, r * 3, r * 5, r * 7);
                    elements.push_back(v & mask);
                    inStream.Write(v & mask, bits);
                }
                inStream.Finish();
                TBitInput outStream(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
                for (size_t j = 0; j < i; ++j) {
                    outStream.Seek(j * bits * 4);
                    TVec4u v;
                    UNIT_ASSERT_VALUES_EQUAL(bits * 4, outStream.Read(&v, bits));
                    UNIT_ASSERT_VALUES_EQUAL(v & mask, elements[j]);
                }
            }
        }
    }

    Y_UNIT_TEST(TestBitsSeek) {
        ui64 r = 42424243ULL;
        for (size_t fullBlocks = 0; fullBlocks < 3; ++fullBlocks) {
            for (size_t remainder = 0; remainder < 65; ++remainder) {
                size_t bufferSize = fullBlocks * 64 + remainder;
                while (bufferSize % 8 != 0) {
                    ++bufferSize;
                }
                TBufferStream stream;
                TBitOutput inStream(&stream);
                TVector<ui8> bitsSeq;
                bitsSeq.reserve(bufferSize);
                for (size_t i = 0; i < fullBlocks; ++i) {
                    r = r * 424243ULL + 4243ULL;
                    inStream.Write(r, 64);
                    for (size_t j = 0; j < 64; ++j) {
                        bitsSeq.push_back((r >> j) & 1);
                    }
                }
                r = r * 424243ULL + 4243ULL;
                inStream.Write(r & ScalarMasks.Values[remainder], remainder);
                for (size_t i = 0; i < remainder; ++i) {
                    bitsSeq.push_back((r >> i) & 1);
                }
                while (bitsSeq.size() < bufferSize) {
                    bitsSeq.push_back(0);
                }
                inStream.Finish();
                TBitInput outStream(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
                for (size_t i = 0; i < bufferSize + 10; ++i) {
                    for (size_t j = 0; j < 65; ++j) {
                        UNIT_ASSERT_VALUES_EQUAL((i <= bufferSize), outStream.Seek(i));
                        ui64 value;
                        size_t bits = j;
                        if (i + bits > bufferSize) {
                            bits = (i < bufferSize ? bufferSize - i : 0);
                        }
                        UNIT_ASSERT_VALUES_EQUAL(bits, outStream.Read(&value, j));
                        for (size_t z = 0; z < bits; ++z) {
                            UNIT_ASSERT_VALUES_EQUAL((value >> z) & 1, bitsSeq[i + z]);
                        }
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(TestOne) {
        TBufferStream stream;

        TVecOutput output(&stream);
        TVec4u src(0xDEADBEEFu, 0x0DEADBEEu, 0xDEADF00Du, 0x0BADBEEFu);
        output.Write(src, 32);
        output.Finish();

        TVec4u dst;

        TVecInput input(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        size_t bits = input.Read(&dst, 32);
        UNIT_ASSERT_VALUES_EQUAL(bits, 32);
        UNIT_ASSERT_VALUES_EQUAL(src, dst);

        TVecMemoryInput input2(TArrayRef<const char>(stream.Buffer().Data(), stream.Buffer().Size()));
        dst = input2.Read();
        UNIT_ASSERT_VALUES_EQUAL(src, dst);
    }

    Y_UNIT_TEST(TestFixed32) {
        TVector<TVec4u> data;
        for (size_t i = 0; i < 2048; i++)
            data.push_back(TVec4u(i * 982374293, i * 982375417, i * 982372289, i * 982372213));

        TBufferStream stream;

        TVecOutput output(&stream);
        for (size_t i = 0; i < 2048; i++)
            output.Write(data[i], 32);
        output.Finish();

        TVecInput input(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        TVecMemoryInput input2(TArrayRef<const char>(stream.Buffer().Data(), stream.Buffer().Size()));
        TVec4u tmp;
        for (size_t i = 0; i < 2048; i++) {
            size_t bits = input.Read(&tmp, 32);

            UNIT_ASSERT_VALUES_EQUAL(bits, 32);
            UNIT_ASSERT_VALUES_EQUAL(tmp, data[i]);

            tmp = input2.Read();

            UNIT_ASSERT_VALUES_EQUAL(tmp, data[i]);
        }
    }

    Y_UNIT_TEST(TestInterleaved) {
        TVector<TVec4u> data;
        for (size_t i = 0; i < 2048; i++)
            data.push_back(TVec4u(i * 982448081, i * 982445797, i * 982430893, i * 982405651) & VectorMask(i % 32));

        TBufferStream stream;

        TVecOutput output(&stream);
        for (size_t i = 0; i < 2048; i++)
            output.Write(data[i], i % 32);
        output.Finish();

        TVecInput input(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        TVecMemoryInput input2(TArrayRef<const char>(stream.Buffer().Data(), stream.Buffer().Size()));
        TVec4u tmp;
        for (size_t i = 0; i < 2048; i++) {
            size_t bits = input.Read(&tmp, i % 32);
            tmp &= VectorMask(i % 32);

            UNIT_ASSERT_VALUES_EQUAL(bits, i % 32);
            UNIT_ASSERT_VALUES_EQUAL(tmp, data[i]);
        }
    }

    Y_UNIT_TEST(TestSkip) {
        TVector<TVec4u> data;
        for (size_t i = 0; i < 2048; i++)
            data.push_back(TVec4u(i * 982374293, i * 982375417, i * 982372289, i * 982372213));

        TBufferStream stream;

        TVecOutput output(&stream);
        output.Write(data[0], 16);
        for (size_t i = 0; i < 2048; i++)
            output.Write(data[i], 32);
        output.Finish();

        TVecInput input(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        TVecMemoryInput input2(TArrayRef<const char>(stream.Buffer().Data(), stream.Buffer().Size()));
        TVec4u tmp;
        input.Read(&tmp, 16);
        for (size_t i = 0; i < 2048; i++) {
            size_t bits = input.Read(&tmp, 32);

            UNIT_ASSERT_VALUES_EQUAL(bits, 32);
            UNIT_ASSERT_VALUES_EQUAL(tmp, data[i]);

            input.Seek(input.Position() + 1);
            input.Seek(input.Position() + 3200);
            input.Seek(input.Position() + 31);

            i += 101;
        }
    }

    Y_UNIT_TEST(TestSeek) {
        TVector<TVec4u> data;
        TVector<ui64> offsets;
        for (size_t i = 0; i < 2048; i++)
            data.push_back(TVec4u(i * 982448081, i * 982445797, i * 982430893, i * 982405651) & VectorMask(i % 32));

        TBufferStream stream;

        TVecOutput output(&stream);
        for (size_t i = 0; i < 2048; i++) {
            offsets.push_back(output.Position());
            output.Write(data[i], i % 32);
        }
        output.Finish();

        TVecInput input2(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        TVec4u tmp;
        for (size_t i = 0; i < 2048; i += 3) {
            input2.Seek(offsets[i]);

            size_t bits = input2.Read(&tmp, i % 32);
            tmp &= VectorMask(i % 32);

            UNIT_ASSERT_VALUES_EQUAL(bits, i % 32);
            UNIT_ASSERT_VALUES_EQUAL(tmp, data[i]);
        }
    }

    Y_UNIT_TEST(TestSeek2) {
        TVector<TVec4u> data;
        TVector<ui64> offsets;
        for (size_t i = 0; i < 2048; i++)
            data.push_back(TVec4u(i * 982448081, i * 982445797, i * 982430893, i * 982405651) & VectorMask(i % 32));

        TBufferStream stream;

        TVecOutput output(&stream);
        for (size_t i = 0; i < 2048; i++) {
            offsets.push_back(output.Position());
            output.Write(data[i], i % 32);
        }
        output.Finish();

        TVecInput input2(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        TVec4u tmp;
        size_t k = 0;
        for (size_t j = 0; j < 2048; ++j) {
            k = k * 5 + 1;
            size_t i = (k >> 15) % 2048;
            input2.Seek(offsets[i]);

            size_t bits = input2.Read(&tmp, i % 32);
            tmp &= VectorMask(i % 32);

            UNIT_ASSERT_VALUES_EQUAL(bits, i % 32);
            UNIT_ASSERT_VALUES_EQUAL(tmp, data[i]);
        }
    }
}
