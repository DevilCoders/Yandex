#include <library/cpp/testing/unittest/registar.h>

#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/custom/ui32_varint_serializer.h>
#include <library/cpp/offroad/test/test_md5.h>

#include "offroad_key_wad_io.h"

#include <random>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestOffroadKeyWadIo) {

    template <class KeyData>
    using TIndexEntry = std::pair<TString, KeyData>;

    template <class KeyData>
    using TIndex = TVector<TIndexEntry<KeyData>>;

    template <class KeyDataGenerator>
    TIndex<typename KeyDataGenerator::TKeyData> GenerateIndex(ui32 seed, size_t size, KeyDataGenerator&& keyDataGenerator) {
        std::minstd_rand rand(seed);

        THashSet<TString> keysSet;
        while (keysSet.size() < size) {
            const size_t len = 1 + rand() % 12;
            TString current;
            current.reserve(len);
            for (size_t i = 0; i < len; ++i) {
                current.push_back('a' + rand() % 3);
            }
            keysSet.insert(current);
        }

        TVector<TString> keys(keysSet.begin(), keysSet.end());
        Sort(keys);

        using TKeyData = typename KeyDataGenerator::TKeyData;
        TIndex<TKeyData> index(size);

        TKeyData lastData = TKeyData();
        for (size_t i = 0; i < size; ++i) {
            index[i].first = std::move(keys[i]);
            index[i].second = keyDataGenerator(lastData, rand);
            lastData = index[i].second;
        }

        return index;
    }

    template <class Writer>
    void Transfer(const TIndex<typename Writer::TKeyData>& index, Writer* writer) {
        using TKeyData = typename Writer::TKeyData;
        for (const TIndexEntry<TKeyData>& entry : index) {
            writer->WriteKey(entry.first, entry.second);
        }
    }

    template <class Io>
    void WriteIndex(const TIndex<typename Io::TWriter::TKeyData>& index, IOutputStream* output) {
        using TSampler = typename Io::TSampler;
        using TWriter = typename Io::TWriter;

        TSampler sampler;
        Transfer(index, &sampler);
        auto model = sampler.Finish();

        TMegaWadWriter megaWadWriter(output);
        TWriter writer(model, &megaWadWriter);
        Transfer(index, &writer);
        writer.Finish();
        megaWadWriter.Finish();
    }

    template <class Io>
    void TestReader(const IWad* wad, const TIndex<typename Io::TWriter::TKeyData>& index) {
        using TReader = typename Io::TReader;
        using TKeyData = typename TReader::TKeyData;

        TReader reader(wad);

        for (const TIndexEntry<TKeyData>& entry : index) {
            TStringBuf key;
            TKeyData data;

            UNIT_ASSERT(reader.ReadKey(&key, &data));
            UNIT_ASSERT_EQUAL(entry.first, key);
            UNIT_ASSERT_EQUAL(entry.second, data);
        }
        TStringBuf key;
        TKeyData data;
        UNIT_ASSERT(!reader.ReadKey(&key, &data));
    }

    template <class Io>
    void TestSearcher(const IWad* wad, const TIndex<typename Io::TWriter::TKeyData>& index) {
        using TSearcher = typename Io::TSearcher;
        using TIterator = typename TSearcher::TIterator;
        using TKeyData = typename TSearcher::TKeyData;

        TSearcher searcher(wad);

        TVector<TString> allKeys;
        allKeys.reserve(index.size());

        TVector<TString> keysToCheck;

        for (const TIndexEntry<TKeyData>& entry : index) {
            allKeys.push_back(entry.first);
            keysToCheck.push_back(entry.first);
            keysToCheck.push_back(entry.first + "a");
            {
                TString tmp = entry.first;
                tmp.back() = tmp.back() - 1;
                keysToCheck.push_back(tmp);
            }
            {
                TString tmp = entry.first;
                tmp.back() = tmp.back() + 1;
                keysToCheck.push_back(tmp);
            }
        }

        keysToCheck.push_back("abcdefgh");

        std::minstd_rand rand(239017);
        for (size_t i = 0; i < 1024; ++i) {
            const size_t len = 1 + rand() % 12;
            TString current;
            current.reserve(len);
            for (size_t j = 0; j < len; ++j) {
                current.push_back('a' + rand() % 3);
            }
            keysToCheck.push_back(std::move(current));
        }

        for (size_t it = 0; it < 2; ++it) {
            std::shuffle(keysToCheck.begin(), keysToCheck.end(), std::minstd_rand(4243 + it * 424243));

            TIterator iterator;
            for (const TString& key : keysToCheck) {
                size_t pos = LowerBound(allKeys.begin(), allKeys.end(), key) - allKeys.begin();
                TStringBuf firstKey;
                TKeyData firstData;
                if (pos >= allKeys.size()) {
                    UNIT_ASSERT(!searcher.LowerBound(key, &firstKey, &firstData, &iterator));
                } else {
                    UNIT_ASSERT(searcher.LowerBound(key, &firstKey, &firstData, &iterator));
                    UNIT_ASSERT_EQUAL(index[pos].first, firstKey);
                    UNIT_ASSERT_EQUAL(index[pos].second, firstData);
                    for (; pos < allKeys.size(); ++pos) {
                        UNIT_ASSERT(iterator.ReadKey(&firstKey, &firstData));
                        UNIT_ASSERT_EQUAL(index[pos].first, firstKey);
                        UNIT_ASSERT_EQUAL(index[pos].second, firstData);
                    }
                    UNIT_ASSERT(!iterator.ReadKey(&firstKey, &firstData));
                }
            }
        }
    }

    template <class Io, class KeyDataGenerator>
    void TestIndex(ui32 seed, size_t size, const TString& md5) {
        using TKeyData = typename KeyDataGenerator::TKeyData;
        TIndex<TKeyData> index = GenerateIndex(seed, size, KeyDataGenerator());

        TBufferStream stream;
        WriteIndex<Io>(index, &stream);
        UNIT_ASSERT_MD5_EQUAL(stream.Buffer(), md5);

        THolder<IWad> wad = IWad::Open(TArrayRef<const char>(stream.Buffer().Data(), stream.Buffer().Size()));
        TestReader<Io>(wad.Get(), index);
        TestSearcher<Io>(wad.Get(), index);
    }

    struct TRandomUi32Generator {

        using TKeyData = ui32;

        template <class Rand>
        ui32 operator()(ui32, Rand&& rand) {
            return rand() % 42;
        }

    };

    Y_UNIT_TEST(NoKeyDataInFat) {
        using TIo = TOffroadKeyWadIo<PantherIndexType, ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor, NOffroad::TNullSerializer, TIdentityCombiner, NoStandardIoModel>;
        TestIndex<TIo, TRandomUi32Generator>(42, (1 << 10), "7ac6105088c76ad29815fbecd6978867");
    }

    struct TRandomAdditionalUi32Generator {

        using TKeyData = ui32;

        template <class Rand>
        ui32 operator()(ui32 last, Rand&& rand) {
            return last + 1 + rand() % 18;
        }

    };

    Y_UNIT_TEST(KeyDataInFat) {
        using TIo = TOffroadKeyWadIo<PantherIndexType, ui32, NOffroad::TUi32Vectorizer, NOffroad::TD1Subtractor, NOffroad::TUi32VarintSerializer, TIdentityCombiner, NoStandardIoModel>;
        TestIndex<TIo, TRandomAdditionalUi32Generator>(4243, (1 << 10), "432c5650ba65551ad621a9e7e2f4101c");
    }

    struct TUi32Pair {
        ui32 Start = 0;
        ui32 End = 0;

        TUi32Pair() = default;

        TUi32Pair(ui32 start, ui32 end)
            : Start(start)
            , End(end)
        {

        }

        friend bool operator==(const TUi32Pair& l, const TUi32Pair& r) {
            return l.Start == r.Start && l.End == r.End;
        }

    };

    struct TUi32PairVectorizer {
        enum {
            TupleSize = 1
        };

        template <class Slice>
        Y_FORCE_INLINE static void Scatter(const TUi32Pair& pair, Slice&& slice) {
            slice[0] = pair.End;
        }

        template <class Slice>
        Y_FORCE_INLINE static void Gather(Slice&& slice, TUi32Pair* pair) {
            *pair = TUi32Pair(0, slice[0]);
        }

    };

    using TUi32PairSubtractor = NOffroad::TD1Subtractor;

    struct TUi32PairSerialializer {
        enum {
            MaxSize = NOffroad::TUi32VarintSerializer::MaxSize
        };

        Y_FORCE_INLINE static size_t Serialize(const TUi32Pair& pair, ui8* dst) {
            return NOffroad::TUi32VarintSerializer::Serialize(pair.End, dst);
        }

        Y_FORCE_INLINE static size_t Deserialize(const ui8* src, TUi32Pair* dst) {
            dst->Start = 0;
            return NOffroad::TUi32VarintSerializer::Deserialize(src, &dst->End);
        }
    };

    struct TUi32PairCombiner {
        static constexpr bool IsIdentity = false;

        Y_FORCE_INLINE static void Combine(const TUi32Pair& prev, const TUi32Pair& next, TUi32Pair* res) {
            *res = TUi32Pair(prev.End, next.End);
        }
    };

    struct TUi32PairRandomGenerator {

        using TKeyData = TUi32Pair;

        template <class Rand>
        TKeyData operator()(const TKeyData& prev, Rand&& rand) {
            return TKeyData(prev.End, prev.End + 1 + rand() % 22);
        }

    };

    Y_UNIT_TEST(CombinedKeyData) {
        using TIo = TOffroadKeyWadIo<PantherIndexType, TUi32Pair, TUi32PairVectorizer, TUi32PairSubtractor, TUi32PairSerialializer, TUi32PairCombiner, NoStandardIoModel>;
        TestIndex<TIo, TUi32PairRandomGenerator>(239017, (1 << 10), "2c4320034487334d652fffd172fe8fd4");
    }

}
