#include <library/cpp/testing/benchmark/bench.h>

#include <util/generic/buffer.h>
#include <util/generic/vector.h>
#include <util/stream/buffer.h>

#include <library/cpp/offroad/codec/bit_decoder_16.h>
#include <library/cpp/offroad/codec/bit_encoder_16.h>
#include <library/cpp/offroad/codec/bit_sampler_16.h>
#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/streams/bit_input.h>
#include <library/cpp/offroad/streams/bit_output.h>
#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/tuple/tuple_writer.h>

using namespace NOffroad;

template<typename Reader, typename Writer, typename Sampler>
class TSimpleIo {
    using TModel = typename Sampler::TModel;
    using TTable = TMultiTable<typename TModel::TEncoderTable, typename TModel::TDecoderTable>;

public:
    TSimpleIo() {
        TVector<ui64> hits = MakeTestData(1024, 0);

        Sampler sampler;
        for (ui64 hit : hits) {
            sampler.WriteHit(hit);
        }
        Model_ = sampler.Finish();
        Table_ = NewMultiTable(Model_);

        Writer writer(Table_.Get(), &Stream_);

        for (ui64 hit : hits) {
            writer.WriteHit(hit);
        }
        writer.Finish();
    }

    Reader GetReader() const {
        return Reader(Table_.Get(), TArrayRef<const char>(Stream_.Buffer().data(), Stream_.Buffer().size()));
    }

private:
    TBufferStream Stream_;
    THolder<TTable> Table_;
    TModel Model_;
};

using TSimpleBlockIo = TSimpleIo<
    TTupleReader<ui64, TTestDataVectorizer, TINSubtractor>,
    TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor>,
    TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor>
>;

using TSimpleBitIo = TSimpleIo<
    TTupleReader<ui64, TTestDataVectorizer, TINSubtractor, TBitDecoder16>,
    TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor, TBitEncoder16>,
    TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor, TBitSampler16>
>;

using TSimpleMultiBitIo = TSimpleIo<
    TTupleReader<ui64, TTestDataVectorizer, TINSubtractor, TBitDecoder16, 13>,
    TTupleWriter<ui64, TTestDataVectorizer, TINSubtractor, TBitEncoder16, 7>,
    TTupleSampler<ui64, TTestDataVectorizer, TINSubtractor, TBitSampler16>
>;

#define REGISTER_FLAT_BENCHMARK(IoType)                                   \
Y_CPU_BENCHMARK(Write##IoType, iface) {                                   \
    for (size_t i : xrange<size_t>(iface.Iterations())) {                 \
        Y_UNUSED(i);                                                      \
        TSimple##IoType##Io io;                                           \
    }                                                                     \
}                                                                         \
                                                                          \
Y_CPU_BENCHMARK(Read##IoType, iface) {                                    \
    TSimple##IoType##Io* io = Singleton<TSimple##IoType##Io>();           \
    for (size_t i : xrange<size_t>(iface.Iterations())) {                 \
        Y_UNUSED(i);                                                      \
        auto reader = io->GetReader();                                    \
        ui64 hit;                                                         \
        while (reader.ReadHit(&hit)) {                                    \
            Y_DO_NOT_OPTIMIZE_AWAY(hit);                                  \
        }                                                                 \
    }                                                                     \
}

REGISTER_FLAT_BENCHMARK(Block)
REGISTER_FLAT_BENCHMARK(Bit)
REGISTER_FLAT_BENCHMARK(MultiBit)

Y_CPU_BENCHMARK(Seek, iface) {
    TSimpleBlockIo* io = Singleton<TSimpleBlockIo>();
    for (size_t i : xrange<size_t>(iface.Iterations())) {
        Y_UNUSED(i);
        TTupleReader<ui64, TTestDataVectorizer, TINSubtractor> reader = io->GetReader();
        for (size_t offset : xrange(128)) {
            for (size_t index : xrange(128)) {
                if (reader.Seek(TDataOffset(offset, index), 0)) {
                    ui64 hit;
                    reader.ReadHit(&hit);
                    Y_DO_NOT_OPTIMIZE_AWAY(hit);
                } else {
                    reader.Reset();
                }
            }
        }
    }
}
