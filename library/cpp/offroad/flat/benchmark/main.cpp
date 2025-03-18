#include <library/cpp/testing/benchmark/bench.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/flat/flat_ui32_searcher.h>
#include <library/cpp/offroad/flat/flat_ui64_searcher.h>
#include <library/cpp/offroad/flat/flat_writer.h>
#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/test/test_md5.h>

#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/stream/buffer.h>
#include <util/random/fast.h>

#include <random>

using namespace NOffroad;

class TFlatIo {
public:
    using TWriter = TFlatWriter<TTestData, TTestData, TTestDataVectorizer, TTestDataVectorizer>;
    using TSearcher = TFlatSearcher<TTestData, TTestData, TTestDataVectorizer, TTestDataVectorizer>;

    TFlatIo() {
        TVector<TTestData> index0 = MakeTestData(10000, 6666);
        TVector<TTestData> index1 = MakeTestData(10000, 4321);
        Sort(index0);

        TWriter writer(&Stream_);
        for (size_t i = 0; i < index0.size(); i++) {
            writer.Write(index0[i], index1[i]);
        }
        writer.Finish();
    }

    TSearcher GetSearcher() const {
        return TSearcher(TArrayRef<const char>(Stream_.Buffer().data(), Stream_.Buffer().size()));
    }

private:
    TBufferStream Stream_;
};

template<typename T>
struct TUintFlatIoTraits;

template<>
struct TUintFlatIoTraits<ui32> {
    using TWriter = TFlatWriter<ui32, std::nullptr_t, TUi32Vectorizer, TNullVectorizer>;
    using TSearcher = TFlatUi32Searcher;
    static constexpr ui32 Mask = ~0u;
};

template<>
struct TUintFlatIoTraits<ui64> {
    using TWriter = TFlatWriter<ui64, std::nullptr_t, TUi64Vectorizer, TNullVectorizer>;
    using TSearcher = TFlatUi64Searcher;
    static constexpr ui64 Mask = (1ull << 56) - 1;
};

template<typename TUint>
class TUintFlatIo {
public:
    using TWriter = typename TUintFlatIoTraits<TUint>::TWriter;
    using TSearcher = typename TUintFlatIoTraits<TUint>::TSearcher;

    TUintFlatIo() {
        TFastRng<TUint> rng(314159265);
        TVector<TUint> index(10000);
        for (TUint& val : index) {
            val = rng() & TUintFlatIoTraits<TUint>::Mask;
        }

        TWriter writer(&Stream_);
        for (TUint val : index) {
            writer.Write(val, nullptr);
        }
        writer.Finish();
        Blob_ = TBlob::NoCopy(Stream_.Buffer().data(), Stream_.Buffer().size());
    }

    TSearcher GetSearcher() const {
        return TSearcher(Blob_);
    }

private:
    TBufferStream Stream_;
    TBlob Blob_;
};

using TFlatIo32 = TUintFlatIo<ui32>;
using TFlatIo64 = TUintFlatIo<ui64>;

#define REGISTER_WRITE_BENCHMARK(Bits)                              \
Y_CPU_BENCHMARK(WriteFlat##Bits, iface) {                           \
    for (size_t i : xrange<size_t>(iface.Iterations())) {           \
        Y_UNUSED(i);                                                \
        TFlatIo##Bits io;                                           \
        Y_DO_NOT_OPTIMIZE_AWAY(io);                                 \
    }                                                               \
}

REGISTER_WRITE_BENCHMARK()
REGISTER_WRITE_BENCHMARK(32)
REGISTER_WRITE_BENCHMARK(64)

#define REGISTER_READ_BENCHMARK(Bits)                               \
Y_CPU_BENCHMARK(ReadFlat##Bits, iface) {                            \
    using TIo = TFlatIo##Bits;                                      \
    using TSearcher = TIo::TSearcher;                               \
    const TIo& io = *Singleton<TIo>();                              \
    for (size_t i : xrange<size_t>(iface.Iterations())) {           \
        Y_UNUSED(i);                                                \
        TSearcher searcher = io.GetSearcher();                      \
        for (size_t j : xrange<size_t>(searcher.Size())) {          \
            auto key = searcher.ReadKey(j);                         \
            Y_DO_NOT_OPTIMIZE_AWAY(key);                            \
        }                                                           \
    }                                                               \
}

REGISTER_READ_BENCHMARK()
REGISTER_READ_BENCHMARK(32)
REGISTER_READ_BENCHMARK(64)

Y_CPU_BENCHMARK(SearchFlat, iface) {
    const TFlatIo& io = *Singleton<TFlatIo>();
    for (size_t i : xrange<size_t>(iface.Iterations())) {
        typename TFlatIo::TSearcher searcher = io.GetSearcher();
        for (size_t j : xrange<size_t>(searcher.Size())) {
            TTestData key = searcher.ReadKey(j);
            size_t index = searcher.LowerBound(key, i % searcher.Size(), searcher.Size());
            Y_DO_NOT_OPTIMIZE_AWAY(index);
        }
    }
}
