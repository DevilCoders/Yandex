#include <library/cpp/testing/unittest/registar.h>

#include <array>
#include <util/generic/vector.h>
#include <util/stream/buffer.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/streams/vec_input.h>
#include <library/cpp/offroad/streams/vec_output.h>
#include <library/cpp/offroad/streams/bit_input.h>
#include <library/cpp/offroad/streams/bit_output.h>
#include <library/cpp/offroad/test/test_md5.h>

#include "bit_encoder_16.h"
#include "bit_decoder_16.h"
#include "bit_sampler_16.h"

#include "encoder_16.h"
#include "decoder_16.h"
#include "sampler_16.h"

#include "encoder_64.h"
#include "decoder_64.h"
#include "sampler_64.h"

#include "interleaved_encoder.h"
#include "interleaved_decoder.h"
#include "interleaved_sampler.h"
#include "multi_table.h"

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TCompressorTest) {
    template <size_t N>
    void FillData(size_t size, TVector<std::array<ui32, N>> * target) {
        ui32 counter = 961749527;

        for (size_t i = 0; i < size; i++) {
            std::array<ui32, N> chunk;
            for (size_t j = 0; j < N; j++) {
                size_t mod;
                if (j < N / 16) {
                    mod = 961813891;
                } else if (j < N / 8) {
                    mod = 3491;
                } else if (j < N / 4) {
                    mod = 499;
                } else if (j < N / 2) {
                    mod = 71;
                } else {
                    mod = 7;
                }
                counter = counter * 961754263 + 961786963;
                chunk[j] = counter % mod;
            }
            target->push_back(chunk);
        }
    }

    template <class Encoder, class Decoder, class Sampler, class Output, class Input>
    void RunTest(size_t chunkCount, const TString& dataMd5, const TString& modelMd5) {
        using TEncoder = Encoder;
        using TDecoder = Decoder;
        using TSampler = Sampler;

        enum {
            BlockSize = TSampler::BlockSize,
            TupleSize = TSampler::TupleSize
        };

        TVector<std::array<ui32, BlockSize>> blocks;
        FillData(chunkCount, &blocks);

        size_t channel = 0;
        TSampler sampler;
        for (const auto& chunk : blocks) {
            sampler.Write(channel, chunk);
            channel = (channel + 1) % TupleSize;
        }
        auto model = sampler.Finish();
        auto table = NewMultiTable(model);

        TBufferStream stream;
        Output output(&stream);
        TEncoder encoder(table.Get(), &output);

        channel = 0;
        for (const auto& chunk : blocks) {
            encoder.Write(channel, chunk);
            channel = (channel + 1) % TupleSize;
        }
        output.Finish();

        {
            channel = 0;
            Input input(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
            TDecoder decoder(table.Get(), &input);
            for (const auto& chunk : blocks) {
                std::array<ui32, BlockSize> decompressedChunk;
                decoder.Read(channel, &decompressedChunk);
                UNIT_ASSERT_VALUES_EQUAL(chunk, decompressedChunk);

                channel = (channel + 1) % TupleSize;
            }
        }

        UNIT_ASSERT_MD5_EQUAL(stream.Buffer(), dataMd5);
        TBufferStream modelStream;
        model.Save(&modelStream);
        UNIT_ASSERT_MD5_EQUAL(modelStream.Buffer(), modelMd5);
    }

    template <class Encoder, class Decoder, class Sampler, class Output, class Input>
    void TestOverflow() {
        using TEncoder = Encoder;
        using TDecoder = Decoder;
        using TSampler = Sampler;
        enum {
            BlockSize = TSampler::BlockSize,
        };

        TSampler sampler;
        std::array<ui32, BlockSize> block;
        block.fill(0);
        sampler.Write(0, block);
        auto model = sampler.Finish();
        auto table = NewMultiTable(model);

        TBufferStream stream;
        Output output(&stream);
        TEncoder encoder(table.Get(), &output);

        block.fill(ui32(-1));
        encoder.Write(0, block);
        output.Finish();

        {
            Input input(TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
            TDecoder decoder(table.Get(), &input);
            std::array<ui32, BlockSize> decompressedChunk;
            decoder.Read(0, &decompressedChunk);
            UNIT_ASSERT_VALUES_EQUAL(block, decompressedChunk);
        }
    }

    Y_UNIT_TEST(BitTest16) {
        RunTest<TBitEncoder16, TBitDecoder16, TBitSampler16, TBitOutput, TBitInput>(1024, "6663b129a91ef5eb38d71f21f8136b6e", "ad278813d9c3a3b8684ea969da1f705d");
    }

    Y_UNIT_TEST(Test16) {
        RunTest<TEncoder16, TDecoder16, TSampler16, TVecOutput, TVecInput>(1024, "9394c1344fb8a29b2d4210c9d6336875", "1346a937b69e571135519796dd59ec94");
    }

    Y_UNIT_TEST(Test64) {
        RunTest<TEncoder64, TDecoder64, TSampler64, TVecOutput, TVecInput>(1024, "4c39ef5504744553e6f4c484cd18dc7f", "467a4a6e53b384122678cd446e1232fc");
    }

    Y_UNIT_TEST(TestOverflowBit16) {
        TestOverflow<TBitEncoder16, TBitDecoder16, TBitSampler16, TBitOutput, TBitInput>();
    }

    Y_UNIT_TEST(TestOverflow16) {
        TestOverflow<TEncoder16, TDecoder16, TSampler16, TVecOutput, TVecInput>();
    }

    Y_UNIT_TEST(TestOverflow64) {
        TestOverflow<TEncoder64, TDecoder64, TSampler64, TVecOutput, TVecInput>();
    }

    Y_UNIT_TEST(TestInterleaved) {
        RunTest<
            TInterleavedEncoder<2, TEncoder64>,
            TInterleavedDecoder<2, TDecoder64>,
            TInterleavedSampler<2, TSampler64>,
            TVecOutput,
            TVecInput>(512, "4d8457d09492e9b59ddfb159a6044a59", "7ff5071e8e3f8f8a64da272fa9c4e537");
    }

    Y_UNIT_TEST(TestBitInterleaved) {
        RunTest<
            TInterleavedEncoder<2, TBitEncoder16>,
            TInterleavedDecoder<2, TBitDecoder16>,
            TInterleavedSampler<2, TBitSampler16>,
            TBitOutput,
            TBitInput>(512, "6a39d85a2157a18e60a8f4893315e816", "05ee8927bdf7724cdc42e6804cd88fd3");
    }
}
