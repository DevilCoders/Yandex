#include <library/cpp/testing/unittest/registar.h>

#include "flat_key_storage.h"
#include "offroad_minhash_wad_io.h"
#include "offroad_minhash_wad_searcher.h"
#include "offroad_minhash_wad_writer.h"
#include "string_key_storage.h"

#include <kernel/doom/offroad_key_wad/combiners.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/ui32_varint_serializer.h>

#include <util/random/shuffle.h>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestOffroadMinHashWad) {
    template <typename Io, typename Generator>
    void TestIndex(Generator generator) {
        using TKey = typename Generator::TKey;
        using TData = typename Generator::TData;
        using TWriter = typename Io::TWriter;
        using TSearcher = typename Io::TSearcher;

        ui32 numKeys = generator.GetNumKeys();
        TVector<TKey> keys(Reserve(numKeys));
        TVector<TData> datas(Reserve(numKeys));

        TBufferOutput wadBuffer;
        {
            TMegaWadWriter wadWriter{&wadBuffer};

            TWriter writer{{}, &wadWriter};

            for (ui32 _ : xrange(numKeys)) {
                Y_UNUSED(_);

                keys.push_back(generator.GenKey());
                datas.push_back(generator.GenData());

                writer.WriteKey(keys.back(), datas.back());
            }
            writer.Finish();
            wadWriter.Finish();
            wadBuffer.Finish();
        }

        size_t totalSize = 0;
        THolder<IWad> wad = IWad::Open(wadBuffer.Buffer());
        for (TStringBuf lump : wad->GlobalLumpsNames()) {
            const size_t lumpSize = wad->LoadGlobalLump(lump).size();
            totalSize += lumpSize;
            Cerr << "Global lump " << lump << " of size " << HumanReadableSize(lumpSize, SF_BYTES) << Endl;
        }
        Cerr
            << "Total global lumps size: " << HumanReadableSize(totalSize, SF_BYTES)
            << ", WAD size: " << HumanReadableSize(wadBuffer.Buffer().size(), SF_BYTES)
            << " (" << HumanReadableSize(static_cast<double>(totalSize) / numKeys, SF_BYTES) << " per key)"
            << Endl;

        TSearcher searcher{ wad.Get() };

        TVector<ui32> idx(xrange(numKeys).begin(), xrange(numKeys).end());
        ShuffleRange(idx);

        THashMap<TKey, ui32> idByKey;
        THashMap<ui32, TKey> keyById;

        for (ui32 iter : xrange(2)) {
            Y_UNUSED(iter);
            for (ui32 i : idx) {
                TKey key = keys[i];

                TMaybe<typename TSearcher::TResult> res = searcher.Find(key);
                UNIT_ASSERT(res);
                UNIT_ASSERT(res->Value == datas[i]);

                ui32 id = res->Index;
                if (idByKey.contains(key)) {
                    UNIT_ASSERT(idByKey[key] == id);
                    UNIT_ASSERT(keyById.contains(id));
                    UNIT_ASSERT(keyById.at(id) == key);
                } else {
                    UNIT_ASSERT(!keyById.contains(id));
                    keyById[id] = key;
                    idByKey[key] = id;
                }
            }
        }

        for (ui32 badKeyIndex : xrange(numKeys)) {
            Y_UNUSED(badKeyIndex);

            TKey key = generator.GenKey();
            TMaybe<typename TSearcher::TResult> res = searcher.Find(key);
            if (res) {
                UNIT_ASSERT(idByKey.contains(key));
                UNIT_ASSERT(idByKey.at(key) == res->Index);
            }
        }
    }

    template <typename Key, typename Data, int Seed = 42, ui32 NumKeys = 1000>
    struct TIntGenerator {
        using TKey = Key;
        using TData = Data;

        TIntGenerator()
            : Rng_{ Seed }
        {
        }

        ui32 GetNumKeys() const {
            return NumKeys;
        }

        Key GenKey() {
            return Rng_.GenRand64();
        }

        Data GenData() {
            return Rng_.GenRand64();
        }

    private:
        TReallyFastRng32 Rng_;
    };

    template <typename Key, typename Data, int Seed = 42, ui32 NumKeys = 1000>
    struct TStringGenerator {
        using TKey = Key;
        using TData = Data;

        TStringGenerator()
            : Rng_{ Seed }
        {
        }

        ui32 GetNumKeys() {
            return NumKeys;
        }

        TString GenKey() {
            size_t len = 250;
            TString ret(len, '\0');
            for (char& c : ret) {
                c = Rng_.Uniform('a', 'z' + 1);
            }

            return ret;
        }

        Data GenData() {
            return Rng_();
        }

    private:
        TReallyFastRng32 Rng_;
    };

    using TFlatIntKeyIo = TFlatKeyValueStorage<
        EWadIndexType::PantherIndexType,
        ui64,
        ui32,
        NOffroad::TUi64Vectorizer,
        NOffroad::TUi32Vectorizer
    >;
    using TStringIntKeyIo = TStringKeyValueStorage<
        EWadIndexType::PantherIndexType,
        ui64,
        ui32,
        NOffroad::NMinHash::TTriviallyCopyableKeySerializer<ui64>,
        NOffroad::TUi32Vectorizer,
        NOffroad::TI1Subtractor,
        NOffroad::TUi32VarintSerializer,
        TIdentityCombiner
    >;
    using TStringKeyIo = TStringKeyValueStorage<
        EWadIndexType::PantherIndexType,
        TString,
        ui32,
        NOffroad::NMinHash::TLimitedStringKeySerializer<256>,
        NOffroad::TUi32Vectorizer,
        NOffroad::TI1Subtractor,
        NOffroad::TUi32VarintSerializer,
        TIdentityCombiner
    >;

    template <typename KeyIo>
    using TIntIo = TOffroadMinHashWadIo<
        EWadIndexType::PantherIndexType,
        ui64,
        ui32,
        NOffroad::NMinHash::TTriviallyCopyableKeySerializer<ui64>,
        KeyIo
    >;
    template <typename KeyIo>
    using TStringIo = TOffroadMinHashWadIo<
        EWadIndexType::PantherIndexType,
        TString,
        ui32,
        NOffroad::NMinHash::TLimitedStringKeySerializer<256>,
        KeyIo
    >;

    Y_UNIT_TEST(10IntFlatKeys) {
        TestIndex<TIntIo<TFlatIntKeyIo>>(TIntGenerator<ui64, ui32, 42, 10>{});
    }

    Y_UNIT_TEST(10IntStringKeys) {
        TestIndex<TIntIo<TStringIntKeyIo>>(TIntGenerator<ui64, ui32, 42, 10>{});
    }

    Y_UNIT_TEST(700IntFlatKeys) {
        TestIndex<TIntIo<TFlatIntKeyIo>>(TIntGenerator<ui64, ui32, 1337, 700>{});
    }

    Y_UNIT_TEST(700IntStringKeys) {
        TestIndex<TIntIo<TStringIntKeyIo>>(TIntGenerator<ui64, ui32, 1337, 700>{});
    }

    Y_UNIT_TEST(1e3IntFlatKeys) {
        TestIndex<TIntIo<TFlatIntKeyIo>>(TIntGenerator<ui64, ui32, 875191, 1'000>{});
    }

    Y_UNIT_TEST(1e3IntStringKeys) {
        TestIndex<TIntIo<TStringIntKeyIo>>(TIntGenerator<ui64, ui32, 875191, 1'000>{});
    }

    Y_UNIT_TEST(1e4IntFlatKeys) {
        TestIndex<TIntIo<TFlatIntKeyIo>>(TIntGenerator<ui64, ui32, 58342897, 10'000>{});
    }

    Y_UNIT_TEST(1e4IntStringKeys) {
        TestIndex<TIntIo<TStringIntKeyIo>>(TIntGenerator<ui64, ui32, 58342897, 10'000>{});
    }

    Y_UNIT_TEST(1e5IntFlatKeys) {
        TestIndex<TIntIo<TFlatIntKeyIo>>(TIntGenerator<ui64, ui32, 18927387, 100'000>{});
    }

    Y_UNIT_TEST(1e5IntStringKeys) {
        TestIndex<TIntIo<TStringIntKeyIo>>(TIntGenerator<ui64, ui32, 18927387, 100'000>{});
    }

    Y_UNIT_TEST(1e6IntFlatKeys) {
        TestIndex<TIntIo<TFlatIntKeyIo>>(TIntGenerator<ui64, ui32, 1293, 1'000'000>{});
    }

    Y_UNIT_TEST(1e6IntStringKeys) {
        TestIndex<TIntIo<TStringIntKeyIo>>(TIntGenerator<ui64, ui32, 1293, 1'000'000>{});
    }

    Y_UNIT_TEST(1e3StringKeys) {
        TestIndex<TStringIo<TStringKeyIo>>(TStringGenerator<TString, ui32, 12345, 1'000>{});
    }

    Y_UNIT_TEST(1e5StringKeys) {
        TestIndex<TStringIo<TStringKeyIo>>(TStringGenerator<TString, ui32, 1791791791, 100'000>{});
    }
}
