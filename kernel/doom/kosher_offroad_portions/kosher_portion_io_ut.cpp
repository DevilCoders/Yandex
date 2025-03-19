#include <library/cpp/testing/unittest/registar.h>

#include <kernel/doom/kosher_offroad_portions/kosher_portion_sampler.h>
#include <kernel/doom/kosher_offroad_portions/kosher_portion_reader.h>
#include <kernel/doom/kosher_offroad_portions/kosher_portion_writer.h>

#include <library/cpp/offroad/custom/subtractors.h>

#include <util/stream/buffer.h>

#include <tuple>

using namespace NDoom;

Y_UNIT_TEST_SUITE(KosherPortionIoTest) {

    class THit {
    public:
        THit() = default;

        THit(ui16 breuk, ui16 word, ui16 form)
            : Break_(breuk)
            , Word_(word)
            , Form_(form)
        {

        }

        ui16 Break() const {
            return Break_;
        }

        void SetBreak(ui16 breuk) {
            Break_ = breuk;
        }

        ui16 Word() const {
            return Word_;
        }

        void SetWord(ui16 word) {
            Word_ = word;
        }

        ui16 Form() const {
            return Form_;
        }

        void SetForm(ui16 form) {
            Form_ = form;
        }

        friend bool operator==(const THit& l, const THit& r) {
            return l.Break_ == r.Break_ && l.Word_ == r.Word_ && l.Form_ == r.Form_;
        }

        friend bool operator<(const THit& l, const THit& r) {
            return std::tie(l.Break_, l.Word_, l.Form_) < std::tie(r.Break_, r.Word_, r.Form_);
        }

        friend IOutputStream& operator<<(IOutputStream& out, const THit& hit) {
            out << "[" << hit.Break() << "." << hit.Word() << "." << hit.Form() << "]";
            return out;
        }

    private:
        ui16 Break_;
        ui16 Word_;
        ui16 Form_;
    };

    struct THitVectorizer {
        enum {
            TupleSize = 3
        };

        template <class Slice>
        static void Gather(Slice&& slice, THit* hit) {
            *hit = THit(slice[0], slice[1], slice[2]);
        }

        template <class Slice>
        static void Scatter(const THit& hit, Slice&& slice) {
            slice[0] = hit.Break();
            slice[1] = hit.Word();
            slice[2] = hit.Form();
        }
    };

    using THitSubtractor = NOffroad::TD2I1Subtractor;

    TVector<std::pair<TString, TVector<THit>>> GenerateIndex() {
        TVector<std::pair<TString, TVector<THit>>> result;
        TVector<TString> keys;
        for (size_t i = 0; i < 100; ++i) {
            size_t len = 1 + ((i + 13) * 42424243) % 43;
            keys.emplace_back();
            for (size_t j = 0; j < len; ++j) {
                keys.back().push_back('a' + (j * 4243) % 13);
            }
        }
        Sort(keys);
        keys.resize(Unique(keys.begin(), keys.end()) - keys.begin());
        for (size_t i = 0; i < keys.size(); ++i) {
            TVector<THit> hits;
            size_t count = ((i + 17) * 239017) % 157;
            for (size_t j = 0; j < count; ++j) {
                hits.emplace_back(1 + (i * j * 1234) % 15, 1 + (i * j * 124125) % 15, 1 + (i * j * 14124525) % 15);
            }
            Sort(hits);
            hits.resize(Unique(hits.begin(), hits.end()) - hits.begin());
            result.emplace_back(keys[i], hits);
        }
        return result;
    }

    Y_UNIT_TEST(WriteRead) {
        TVector<std::pair<TString, TVector<THit>>> index = GenerateIndex();

        TKosherPortionSampler<THit, THitVectorizer, THitSubtractor> sampler;
        for (const auto& entry : index) {
            for (const auto& hit : entry.second) {
                sampler.WriteHit(hit);
            }
            sampler.WriteKey(entry.first);
        }
        TKosherPortionSampler<THit, THitVectorizer, THitSubtractor>::THitModel hitModel;
        TKosherPortionSampler<THit, THitVectorizer, THitSubtractor>::TKeyModel keyModel;
        std::tie(hitModel, keyModel) = sampler.Finish();

        using TWriterHitTable = TKosherPortionWriter<THit, THitVectorizer, THitSubtractor>::THitTable;
        using TWriterKeyTable = TKosherPortionWriter<THit, THitVectorizer, THitSubtractor>::TKeyTable;

        THolder<TWriterHitTable> hitTable = MakeHolder<TWriterHitTable>();
        hitTable->Reset(hitModel);

        THolder<TWriterKeyTable> keyTable = MakeHolder<TWriterKeyTable>();
        keyTable->Reset(keyModel);

        TBufferStream hitStream;
        TBufferStream keyStream;

        TKosherPortionWriter<THit, THitVectorizer, THitSubtractor> writer(hitTable.Get(), &hitStream, keyTable.Get(), &keyStream);
        for (const auto& entry : index) {
            for (const auto& hit : entry.second) {
                writer.WriteHit(hit);
            }
            writer.WriteKey(entry.first);
        }
        writer.Finish();

        using TReaderHitTable = TKosherPortionReader<THit, THitVectorizer, THitSubtractor>::THitTable;
        using TReaderKeyTable = TKosherPortionReader<THit, THitVectorizer, THitSubtractor>::TKeyTable;

        THolder<TReaderHitTable> readerHitTable = MakeHolder<TReaderHitTable>();
        readerHitTable->Reset(hitModel);

        THolder<TReaderKeyTable> readerKeyTable = MakeHolder<TReaderKeyTable>();
        readerKeyTable->Reset(keyModel);

        TKosherPortionReader<THit, THitVectorizer, THitSubtractor> reader(readerHitTable.Get(), TArrayRef<const char>(hitStream.Buffer().data(), hitStream.Buffer().size()),
            readerKeyTable.Get(), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()));

        TVector<std::pair<TString, TVector<THit>>> index2;
        TStringBuf key;
        while (reader.ReadKey(&key)) {
            TVector<THit> hits;
            THit hit;
            while (reader.ReadHit(&hit)) {
                hits.push_back(hit);
            }
            index2.emplace_back(TString(key), hits);
        }

        UNIT_ASSERT_VALUES_EQUAL(index, index2);
    }

}
