#include "one_doc_mr_ann_index.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/output.h>

using namespace NIndexAnn;

template <>
void Out<THit>(IOutputStream& out, const THit& hit) {
    out << "[" << hit.DocId() << "." << hit.Break() << "." << hit.Region() << "." << hit.Stream() << "." << hit.Value() << "]";
}

Y_UNIT_TEST_SUITE(TestOneDocMrIndex) {
    void SetBreak(T4DArrayElement& elem, THit::TBreak breuk) {
        elem.SetElementId(breuk);
    }

    void SetRegion(T4DArrayElement& elem, THit::TRegion region) {
        elem.SetEntryKey(region);
    }

    void SetStream(T4DArrayElement& elem, THit::TStream stream) {
        elem.SetItemKey(stream);
    }

    void SetValue(T4DArrayElement& elem, ui32 value) {
        char* ptr = reinterpret_cast<char*>(&value);
        size_t len = 4;
        while (len > 1 && ptr[len - 1] == 0) {
            --len;
        }
        elem.SetData(TString(ptr, len));
    }

    void AddHit(T4DArrayRow& row, const THit& hit) {
        T4DArrayElement& elem = *row.AddElements();
        SetBreak(elem, hit.Break());
        SetRegion(elem, hit.Region());
        SetStream(elem, hit.Stream());
        SetValue(elem, hit.Value());
    }

    void CheckHits(IDocDataIterator& iterator, const TVector<THit>& expected) {
        size_t ptr = 0;
        while (iterator.Valid()) {
            UNIT_ASSERT(ptr < expected.size());
            UNIT_ASSERT_VALUES_EQUAL(expected[ptr], iterator.Current());
            UNIT_ASSERT_VALUES_EQUAL(expected[ptr], iterator.Current());
            const THit* next = iterator.Next();
            if (next) {
                UNIT_ASSERT(iterator.Valid());
                UNIT_ASSERT_VALUES_EQUAL(*next, iterator.Current());
            }
            ++ptr;
        }
        UNIT_ASSERT_VALUES_EQUAL(expected.size(), ptr);
    }

    Y_UNIT_TEST(Simple) {
        T4DArrayRow row;
        AddHit(row, THit(0, 2, 42, 43, (12 << 24)));
        AddHit(row, THit(0, 5, 1, 11, (42 << 16)));
        AddHit(row, THit(0, 5, 12, 13, (14 << 8)));
        AddHit(row, THit(0, 42, 1024, 512, 43));
        AddHit(row, THit(0, 4243, 124, 4124, 1424214));
        THolder<IDocDataIndex> index = MakeHolder<TOneDocMrIndex>(&row);
        THolder<IDocDataIterator> iterator = index->CreateIterator();

        iterator->Restart(THitMask(0, 2));
        CheckHits(*iterator, { THit(0, 2, 42, 43, (12 << 24)) });

        iterator->Restart(THitMask(0, 5));
        CheckHits(*iterator, { THit(0, 5, 1, 11, (42 << 16)), THit(0, 5, 12, 13, (14 << 8)) });

        iterator->Restart(THitMask(0, 42));
        CheckHits(*iterator, { THit(0, 42, 1024, 512, 43) });

        iterator->Restart(THitMask(0, 4243));
        CheckHits(*iterator, { THit(0, 4243, 124, 4124, 1424214) });

        iterator->Restart(THitMask(0, 5000));
        CheckHits(*iterator, {});

        iterator->Restart(THitMask(0, 1));
        CheckHits(*iterator, {});

        iterator->Restart(THitMask(0, 13));
        CheckHits(*iterator, {});
    }

    ui64 NextPseudoRandom(ui64& value) {
        return value = value * 6364136223846793005ULL + 1442695040888963407ULL;
    }

    THit GenerateHit(ui64& seed) {
        THit::TDocId docId = 0;
        THit::TBreak breuk = 1 + NextPseudoRandom(seed) % 100;
        THit::TRegion region = NextPseudoRandom(seed);
        THit::TStream stream = NextPseudoRandom(seed);
        THit::TValue value = NextPseudoRandom(seed);
        return THit(docId, breuk, region, stream, value);
    }

    Y_UNIT_TEST(Random) {
        TVector<THit> hits;
        size_t hitsCount = 1024;
        ui64 seed = 0;
        for (size_t i = 0; i < hitsCount; ++i) {
            hits.push_back(GenerateHit(seed));
        }
        Sort(hits);
        T4DArrayRow row;
        for (const auto& hit : hits) {
            AddHit(row, hit);
        }
        THolder<IDocDataIndex> index = MakeHolder<TOneDocMrIndex>(&row);
        THolder<IDocDataIterator> iterator = index->CreateIterator();
        for (size_t it = 0; it < 512; ++it) {
            THitMask mask(0, NextPseudoRandom(seed) % 1024);
            iterator->Restart(mask);
            TVector<THit> expected;
            for (const auto& hit : hits) {
                if (mask.Matches(hit)) {
                    expected.push_back(hit);
                }
            }
            CheckHits(*iterator, expected);
        }
    }
}
