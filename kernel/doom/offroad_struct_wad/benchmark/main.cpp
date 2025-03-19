#include <library/cpp/testing/benchmark/bench.h>

#include <kernel/doom/offroad_struct_wad/offroad_struct_wad_ut.h>
#include <util/generic/singleton.h>


template <EStructType StructType, ECompressionType CompressionType>
void ReadAll(ui32 size, ui32 docs) {
    using TSelectedIndex = TIndex<StructType, CompressionType>;
    using TReader = typename TSelectedIndex::TReader;
    using THit = typename TReader::THit;

    const TSelectedIndex& index = *Singleton<TSelectedIndex>(size, 0, docs ? docs - 1 : docs, docs);

    THolder<TReader> reader = index.GetReader();
    for (ui32 iter = 0; iter < 2; ++iter, reader->Reset(index.GetWad())) {
        ui32 docId;
        while (reader->ReadDoc(&docId)) {
            THit hit;
            while (reader->ReadHit(&hit)) {
                NBench::Escape(&hit);
            }
        }
    }
}

template <EStructType StructType, ECompressionType CompressionType>
void SearchAll(ui32 size, ui32 docs) {
    using TSelectedIndex = TIndex<StructType, CompressionType>;
    using TSearcher = typename TSelectedIndex::TSearcher;
    using TIterator = typename TSearcher::TIterator;
    using THit = typename TSearcher::THit;

    const TSelectedIndex& index = *Singleton<TSelectedIndex>(size, 0, docs ? docs - 1 : docs, docs);

    const typename TSelectedIndex::TData& testData = index.GetData();
    const ui32 first = size ? testData[0].first : 0;
    const ui32 last = size ? testData.back().first : 0;
    TVector<ui32> val(last - first + 1);
    Iota(val.begin(), val.end(), 0);
    Shuffle(val.begin(), val.end());

    THolder<TSearcher> searcher = index.GetSearcher();

    for (ui32 id : val) {
        TIterator it;
        if (searcher->Find(id, &it)) {
            THit hit;
            it.ReadHit(&hit);
            NBench::Escape(&hit);
        }
    }
}

#define REGISTER_BUILD_BENCHMARK_IMPL(BenchName, StructType, CompressionType, Size, Docs)          \
    Y_CPU_BENCHMARK(BenchName, iface) {                                                            \
        for (size_t i : xrange<size_t>(iface.Iterations())) {                                      \
            Y_UNUSED(i);                                                                           \
            ui32 sz = Size;                                                                        \
            ui32 docs = Docs;                                                                      \
            TIndex<StructType, CompressionType> index(sz, 0, docs ? docs - 1 : docs, docs);        \
            Y_DO_NOT_OPTIMIZE_AWAY(index);                                                         \
        }                                                                                          \
    }

#define REGISTER_READ_BENCHMARK_IMPL(BenchName, StructType, CompressionType, Size, Docs)           \
    Y_CPU_BENCHMARK(BenchName, iface) {                                                            \
        for (size_t i : xrange<size_t>(iface.Iterations())) {                                      \
            Y_UNUSED(i);                                                                           \
            ReadAll<StructType, CompressionType>(Size, Docs);                                      \
        }                                                                                          \
    }

#define REGISTER_SEARCH_BENCHMARK_IMPL(BenchName, StructType, CompressionType, Size, Docs)         \
    Y_CPU_BENCHMARK(BenchName, iface) {                                                            \
        for (size_t i : xrange<size_t>(iface.Iterations())) {                                      \
            Y_UNUSED(i);                                                                           \
            SearchAll<StructType, CompressionType>(Size, Docs);                                    \
        }                                                                                          \
    }

#define REGISTER_BENCHMARK_IMPL(BenchName, StructType, CompressionType, Size, Docs)                \
    REGISTER_BUILD_BENCHMARK_IMPL(Build ## BenchName, StructType, CompressionType, Size, Docs)     \
    REGISTER_READ_BENCHMARK_IMPL(Read ## BenchName, StructType, CompressionType, Size, Docs)       \
    REGISTER_SEARCH_BENCHMARK_IMPL(Search ## BenchName, StructType, CompressionType, Size, Docs)

#define REGISTER_SMALL_BENCHMARK(BenchName, StructType, CompressionType)                           \
    REGISTER_BENCHMARK_IMPL(BenchName ## Small, StructType, CompressionType, 30, 20000)

#define REGISTER_BIG_BENCHMARK(BenchName, StructType, CompressionType)                             \
    REGISTER_BENCHMARK_IMPL(BenchName ## Big, StructType, CompressionType, 7000, 0)

#define REGISTER_BENCHMARK(BenchName, StructType, CompressionType)                                 \
    REGISTER_SMALL_BENCHMARK(BenchName, StructType, CompressionType)                               \
    REGISTER_BIG_BENCHMARK(BenchName, StructType, CompressionType)

REGISTER_BENCHMARK(OffroadAutoEof, AutoEofStructType, OffroadCompressionType)
REGISTER_BENCHMARK(OffroadFixedSize, FixedSizeStructType, OffroadCompressionType)
REGISTER_BENCHMARK(RawAutoEof, AutoEofStructType, RawCompressionType)
REGISTER_BENCHMARK(RawFixedSize, FixedSizeStructType, RawCompressionType)
REGISTER_BENCHMARK(RawVariableSize, VariableSizeStructType, RawCompressionType)

