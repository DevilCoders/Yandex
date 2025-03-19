#include <kernel/doom/item_storage/item_key.h>
#include <kernel/doom/item_storage/item_chunk_mapper.h>

#include <kernel/doom/offroad_minhash_wad/offroad_minhash_wad_io.h>
#include <kernel/doom/offroad_minhash_wad/offroad_minhash_wad_writer.h>

#include <kernel/doom/offroad_minhash_wad/flat_key_storage.h>

#include <kernel/doom/offroad_key_wad/combiners.h>

#include <library/cpp/offroad/minhash/writer.h>
#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/testing/benchmark/bench.h>

#include <util/generic/singleton.h>
#include <util/random/shuffle.h>

namespace NDoom {

using TFlatItemChunkKeyIo = TFlatKeyValueStorage<
    EWadIndexType::ItemStorage,
    NItemStorage::TItemKey,
    NItemStorage::TChunkLocalId,
    NItemStorage::TItemKeyVectorizer,
    NItemStorage::TChunkLocalIdVectorizer
>;

using TItemChunkMappingIo = TOffroadMinHashWadIo<
    EWadIndexType::ItemStorage,
    NItemStorage::TItemKey,
    NItemStorage::TChunkLocalId,
    NOffroad::NMinHash::TTriviallyCopyableKeySerializer<NItemStorage::TItemKey>,
    TFlatItemChunkKeyIo
>;

template <int Seed, ui32 NumKeys>
struct TIntGenerator {
    TIntGenerator() {
        TReallyFastRng32 gen(Seed);
        Keys_.reserve(NumKeys);
        SearchKeys_.reserve(NumKeys);
        for (ui32 i = 0; i < NumKeys; ++i) {
            Keys_.push_back(gen.GenRand64());
            SearchKeys_.push_back(gen.GenRand64());
        }
    }

    ui32 GetNumKeys() const {
        return NumKeys;
    }

    const TVector<ui64>& GetKeys() const {
        return Keys_;
    }

    const TVector<ui64>& GetSearchKeys() const {
        return Keys_;
    }

private:
    TVector<ui64> Keys_;
    TVector<ui64> SearchKeys_;
};

template <typename Generator>
TBuffer BuildMinHash(ui32 chunk, ui32 loadFactor) {
    using TWriter = typename TItemChunkMappingIo::TWriter;

    NOffroad::NMinHash::TMinHashParams params {
        .LoadFactor = loadFactor * 0.001,
    };

    TBuffer buffer;
    TBufferOutput wadBuffer(buffer);
    {
        TMegaWadWriter wadWriter{ &wadBuffer };

        TWriter writer{ {}, &wadWriter };

        const Generator& generator = Default<Generator>();
        const ui32 numKeys = generator.GetNumKeys();
        const TVector<ui64>& keys = generator.GetKeys();
        for (ui32 i = 0; i < numKeys; ++i) {
            writer.WriteKey(keys[i], NItemStorage::TChunkLocalId{.Chunk = chunk, .LocalId = i});
        }
        writer.Finish(params);
        wadWriter.Finish();
    }
    wadBuffer.Finish();

    return buffer;
}

template <typename Generator, ui32 loadFactor>
struct TTestSearcher {
    using TSearcher = typename TItemChunkMappingIo::TSearcher;

    TTestSearcher()
        : Buffer_(BuildMinHash<Generator>(0, loadFactor))
        , Wad_(IWad::Open(Buffer_))
        , Searcher_(Wad_.Get())
    {
    }

    ui32 FindAll() {
        const Generator& generator = Default<Generator>();
        ui32 count = 0;
        for (auto& key : generator.GetKeys()) {
            auto found = Searcher_.Find(key);
            count += found.Defined();
        }
        for (auto& key : generator.GetSearchKeys()) {
            auto found = Searcher_.Find(key);
            count += found.Defined();
        }
        return count;
    }

private:
    TBuffer Buffer_;
    THolder<IWad> Wad_;
    TSearcher Searcher_;
};

constexpr ui32 DATA_SIZE = 75000;

#define DEFINE_BENCH(seed, dataSize, loadFactor) \
    Y_CPU_BENCHMARK(Build_LF_##loadFactor, iface) { \
        size_t sz = 0; \
        for (size_t i = 0, imax = iface.Iterations(); i < imax; ++i) { \
            TBuffer buffer = BuildMinHash<TIntGenerator<seed, dataSize>>(static_cast<ui32>(i), loadFactor); \
            sz = buffer.size(); \
            Y_DO_NOT_OPTIMIZE_AWAY(buffer.Data()); \
            NBench::Clobber(); \
        } \
    } \
    Y_CPU_BENCHMARK(Search_LF_##loadFactor, iface) { \
        TTestSearcher<TIntGenerator<seed, dataSize>, loadFactor> searcher; \
        for (size_t i = 0, imax = iface.Iterations(); i < imax; ++i) { \
            auto count = searcher.FindAll(); \
            Y_DO_NOT_OPTIMIZE_AWAY(count); \
            NBench::Clobber(); \
        } \
    }

DEFINE_BENCH(43, DATA_SIZE, 995)
DEFINE_BENCH(47, DATA_SIZE, 990)
DEFINE_BENCH(53, DATA_SIZE, 950)
DEFINE_BENCH(24, DATA_SIZE, 900)

}  // namespace NDoom
