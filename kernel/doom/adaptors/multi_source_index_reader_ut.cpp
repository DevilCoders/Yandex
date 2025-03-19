#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>
#include <util/generic/noncopyable.h>
#include <util/random/mersenne.h>

#include <utility>

#include "multi_source_index_reader.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TMultiSourceIndexReaderTests) {
    template <class TKey, class THit>
    struct TReaderSource: public std::pair<TVector<TKey>, TVector<TVector<THit>>> {
        using TBase = std::pair<TVector<TKey>, TVector<TVector<THit>>>;

        template <class... Args>
        TReaderSource(Args... args)
            : TBase(std::forward<Args>(args)...)
        {
        }

        TVector<TKey> Keys() {
            return TBase::first;
        }

        TVector<TVector<THit>> Hits() {
            return TBase::second;
        }
    };

    template <class Key, class KeyRef, class Hit>
    class TSimpleReader: private TNonCopyable {
    public:
        using TKeyRef = KeyRef;
        using TKey = Key;
        using TKeyData = ui32;
        using THit = Hit;
        using TSource = TReaderSource<TKey, THit>;

        TSimpleReader(TSource source)
            : Keys_(std::move(source.Keys()))
            , Hits_(std::move(source.Hits()))
        {
        }

        bool ReadKey(TKeyRef* key, TKeyData* data) {
            if (KeyPos_ < Keys_.size()) {
                *key = Keys_[KeyPos_++];
                *data = Hits().size();
                HitPos_ = 0;
                return true;
            }
            return false;
        }

        bool ReadHit(THit* hit) {
            if (HitPos_ < Hits().size()) {
                *hit = Hits()[HitPos_++];
                return true;
            }
            return false;
        }

        TProgress Progress() const {
            return TProgress(KeyPos_, Keys_.size());
        }

    private:
        TVector<TKey> Keys_;
        TVector<TVector<THit>> Hits_;
        size_t KeyPos_ = 0;
        size_t HitPos_ = 0;

        const TVector<THit>& Hits() {
            return Hits_[KeyPos_ - 1];
        }
    };

    template <class Reader>
    void DoTest(const TVector<typename Reader::TSource>& readerSources,
                const TVector<typename Reader::TKey>& keysExpected,
                const TVector<TVector<typename Reader::THit>>& hitsExpected) {
        using TReader = Reader;
        using TKey = typename TReader::TKey;
        using TKeyRef = typename TReader::TKeyRef;
        using TKeyData = typename TReader::TKeyData;
        using THit = typename TReader::THit;
        using TMultiReader = TMultiSourceIndexReader<TReader>;

        TMultiReader multiReader(readerSources);

        TVector<TKey> keysActual;
        TVector<TKeyData> keyDatums;
        TVector<TVector<THit>> hitsActual;
        TKeyRef key;
        TKeyData data;
        while (multiReader.ReadKey(&key, &data)) {
            keysActual.emplace_back(key);
            keyDatums.push_back(data);
            TVector<THit> keyHits;
            THit hit;
            while (multiReader.ReadHit(&hit)) {
                keyHits.push_back(hit);
            }
            hitsActual.push_back(keyHits);
            TProgress progress = multiReader.Progress();
            UNIT_ASSERT_C(progress.Current() <= progress.Total(), "Invalid progress.");
        }

        TProgress progress = multiReader.Progress();
        UNIT_ASSERT_VALUES_EQUAL_C(progress.Current(), progress.Total(), "Invalid progress.");
        UNIT_ASSERT_VALUES_EQUAL_C(keysActual, keysExpected, "Invalid keys.");
        for (size_t i = 0; i < hitsExpected.size(); i++) {
            UNIT_ASSERT_VALUES_EQUAL_C(keyDatums[i], hitsExpected[i].size(), "Invalid key data.");
            UNIT_ASSERT_VALUES_EQUAL_C(hitsActual[i], hitsExpected[i], "Invalid hits.");
        }
    }

    class TReaderSourceGenerator {
        const ui32 KeysSizeMax;
        const ui32 KeyMax;
        const ui32 HitsSizeMax;
        const ui32 HitMax;

        TMersenne<ui32> KeysSizeRng;
        TMersenne<ui32> KeyRng;
        TMersenne<ui32> HitsSizeRng;
        TMersenne<ui32> HitRng;

        THashMap<TString, TVector<ui32>> AllKeyHits;

    public:
        using TReader = TSimpleReader<TString, TStringBuf, ui32>;
        using TSource = TReader::TSource;

        TReaderSourceGenerator(
            ui32 keysSizeMax = 50, ui32 keyMax = 10, ui32 hitsSizeMax = 50, ui32 hitMax = 10)
            : KeysSizeMax(keysSizeMax)
            , KeyMax(keyMax)
            , HitsSizeMax(hitsSizeMax)
            , HitMax(hitMax)
        {
        }

        TSource Next() {
            THashSet<TString> keySet;
            size_t keysSize = KeysSizeRng() % KeysSizeMax;
            for (size_t i = 0; i < keysSize; i++) {
                TString key = ToString(KeyRng() % KeyMax);
                keySet.insert(key);
            }

            TVector<TString> keys(keySet.size());
            TVector<TVector<ui32>> hits(keys.size());
            auto keyIter = keySet.begin();
            for (size_t i = 0; i < keys.size(); i++, keyIter++) {
                keys[i] = *keyIter;
                TVector<ui32> keyHits(HitsSizeRng() % HitsSizeMax);
                for (size_t j = 0; j < keyHits.size(); j++) {
                    keyHits[j] = HitRng() % HitMax;
                }
                Sort(keyHits);
                hits[i] = std::move(keyHits);
            }

            Sort(keys);
            for (size_t i = 0; i < keys.size(); i++) {
                TString key = keys[i];
                TVector<ui32>& keyHits = hits[i];
                if (AllKeyHits.contains(key)) {
                    AllKeyHits[key].insert(AllKeyHits[key].end(), keyHits.begin(), keyHits.end());
                } else {
                    AllKeyHits[key] = keyHits;
                }
            }

            return TSource(std::move(keys), std::move(hits));
        }

        TVector<TString> AllKeys() {
            TVector<TString> keys;
            for (auto key : AllKeyHits) {
                keys.push_back(key.first);
            }

            Sort(keys);
            return keys;
        }

        TVector<TVector<ui32>> AllHits(const TVector<TString>& keys) {
            TVector<TVector<ui32>> hits;
            for (auto key : keys) {
                TVector<ui32>& keyHits = AllKeyHits[key];
                Sort(keyHits);
                hits.push_back(std::move(keyHits));
            }
            return hits;
        }
    };

    Y_UNIT_TEST(TestRandomSource) {
        using TReader = typename TReaderSourceGenerator::TReader;
        using TReaderSource = typename TReaderSourceGenerator::TSource;

        static constexpr size_t ReadersSize = 1000;

        TReaderSourceGenerator readerSourceGnt;
        TVector<TReaderSource> readerSources(ReadersSize);
        for (size_t i = 0; i < readerSources.size(); i++) {
            readerSources[i] = readerSourceGnt.Next();
        }

        auto keysExpected = readerSourceGnt.AllKeys();
        auto hitsExpected = readerSourceGnt.AllHits(keysExpected);
        DoTest<TReader>(readerSources, keysExpected, hitsExpected);
    }
}
