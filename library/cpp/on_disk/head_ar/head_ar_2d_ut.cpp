#include "head_ar_2d.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/system/tempfile.h>

#include <cstdlib>

namespace {
    const bool USE_MAP = true;
    const bool NOT_USE_MAP = false;
    const bool USE_RAW_WRITER = true;
    const bool NOT_USE_RAW_WRITER = false;

    struct TRec {
        static const ui32 Version = 1;

        ui64 A;
        TRec(ui64 a = 0)
            : A(a)
        {
        }
    };

    struct TRec2 {
        ui64 A;
        ui32 B;
        TRec2(ui64 a = 0, ui32 b = 0)
            : A(a)
            , B(b)
        {
        }
    };

    void RunSimpleTest(bool useMap, bool useRawWriter) {
        TString path = MakeTempName(nullptr, "TArrayWithHead2DTest");
        TTempFile deleter(path);

        struct TTestData {
            ui32 DocId;
            ui64 Key;
            TRec Data;
        } testData[] = {
            {1, 213, TRec(100)},
            {1, 143, TRec(200)},
            {2, 143, TRec(300)},
            {2, 213, TRec(400)},
            {10, 10000, TRec(500)},
            {11, 1ULL << 50, TRec(600)},
        };

        if (useRawWriter) {
            TArrayWithHead2DWriter<TStringBuf> writer(path.data(), sizeof(TRec), TRec::Version);
            for (TTestData* cur = std::begin(testData);
                 cur != std::end(testData); ++cur) {
                TStringBuf data((const char*)&cur->Data, sizeof(cur->Data));
                writer.Put(cur->DocId, cur->Key, data);
            }
        } else {
            TArrayWithHead2DWriter<TRec> writer(path.data());
            for (TTestData* cur = std::begin(testData);
                 cur != std::end(testData); ++cur) {
                writer.Put(cur->DocId, cur->Key, cur->Data);
            }
        }

        {
            THolder<TMemoryMap> hMapping;
            THolder<TArrayWithHead2D<TRec>> hReader;
            if (useMap) {
                hMapping.Reset(new TMemoryMap(path.data()));
                hReader.Reset(new TArrayWithHead2D<TRec>(*hMapping));
            } else
                hReader.Reset(new TArrayWithHead2D<TRec>(path.data()));
            TArrayWithHead2D<TRec>& reader = *hReader;

            UNIT_ASSERT(reader.GetSize() == 12);
            UNIT_ASSERT(reader.GetVersion() == TRec::Version);
            UNIT_ASSERT(reader.GetRecordSize() == sizeof(TRec));
            UNIT_ASSERT(reader.Find(1, 213) != nullptr);
            UNIT_ASSERT_EQUAL(reader.Get(1, 213).A, 100);
            UNIT_ASSERT_EQUAL(reader.Get(1, 143).A, 200);
            UNIT_ASSERT_EQUAL(reader.Get(2, 143).A, 300);
            UNIT_ASSERT_EQUAL(reader.Get(2, 213).A, 400);
            UNIT_ASSERT_EQUAL(reader.Find(1, 10000), NULL);
            UNIT_ASSERT_EQUAL(reader.Find(100, 10000), NULL);
            UNIT_ASSERT_EQUAL(reader.Get(1, 10000).A, 0);
            UNIT_ASSERT_EQUAL(reader.Get(100, 10000).A, 0);
            UNIT_ASSERT_EQUAL(reader.Get(10, 10000).A, 500);
            UNIT_ASSERT_EQUAL(reader.Get(11, 1ULL << 50).A, 600);
            hReader.Destroy();
            hMapping.Destroy();
        }

        {
            THolder<TMemoryMap> hMapping;
            THolder<TArrayWithHead2D<TRec2>> hReader;

            if (useMap) {
                hMapping.Reset(new TMemoryMap(path.data()));
                hReader.Reset(new TArrayWithHead2D<TRec2>(*hMapping, true, true)); // isPolite=true, quiet=true
            } else
                hReader.Reset(new TArrayWithHead2D<TRec2>(path.data(), true, true)); // isPolite=true, quiet=true
            TArrayWithHead2D<TRec2>& reader = *hReader;

            UNIT_ASSERT(reader.GetVersion() == TRec::Version);
            UNIT_ASSERT(reader.GetRecordSize() == sizeof(TRec));
            UNIT_ASSERT(reader.GetSize() == 12);
            UNIT_ASSERT_EQUAL(reader.Get(2, 143).A, 300);
            UNIT_ASSERT_EQUAL(reader.Get(2, 143).B, 0);
            UNIT_ASSERT_EQUAL(reader.Get(2, 213).A, 400);
            UNIT_ASSERT_EQUAL(reader.Get(2, 213).B, 0);
            UNIT_ASSERT_EQUAL(reader.Get(10000, 10000).B, 0);

            hReader.Destroy();
            hMapping.Destroy();
        }
        {
            TArrayWithHead2D<TRec> reader;
            UNIT_ASSERT_EQUAL(reader.Find(0, 0), nullptr);
        }
    }

    void RunRandomTest(bool useMap, bool useRawWriter) {
        TString path = MakeTempName(nullptr, "TArrayWithHead2DTest");
        TTempFile deleter(path);

        srand(42);
        for (int attempt = 0; attempt < 20; attempt++) {
            // Generate data
            const ui32 maxDocId = 1000;
            THashMap<ui32, TVector<std::pair<ui64, ui64>>> data;
            for (ui32 docId = 0; docId < maxDocId; docId++) {
                if (rand() % 2 == 0)
                    continue;

                THashMap<ui64, bool> seen;
                for (int n = rand() % 10; n >= 0; n--) {
                    ui64 key = rand(), value = rand() % (attempt < 5 ? 1000 : 100000);
                    if (seen.count(key) != 0)
                        continue;
                    seen[key] = true;
                    data[docId].push_back(std::pair<ui64, ui64>(key, value));
                }
            }

            // Write data
            const bool useIndirectKeys = (attempt % 2 == 1);
            if (useRawWriter) {
                TArrayWithHead2DWriter<TStringBuf> writer(path, sizeof(TRec), TRec::Version, useIndirectKeys);
                for (ui32 docId = 0; docId < maxDocId; ++docId) {
                    const TVector<std::pair<ui64, ui64>>& vec = data[docId];
                    for (const auto& it : vec) {
                        TRec rec(it.second);
                        writer.Put(docId, it.first, TString((const char*)&rec, sizeof(rec)));
                    }
                }
                writer.Finish();
            } else {
                TArrayWithHead2DWriter<TRec> writer(path, useIndirectKeys);
                for (ui32 docId = 0; docId < maxDocId; ++docId) {
                    const TVector<std::pair<ui64, ui64>>& vec = data[docId];
                    for (const auto& it : vec) {
                        writer.Put(docId, it.first, it.second);
                    }
                }
                writer.Finish();
            }

            // Add 1 to all data to check memory-mapped updatable
            TFileMappedArrayWithHead2DUpdatable<TRec> updatableArray(path);
            // Creating it here to check if value is updated in real time, not after destruction of updatable
            if (attempt > 10) {
                for (ui32 docId = 0; docId < maxDocId; docId++) {
                    TVector<std::pair<ui64, ui64>>& records = data[docId];
                    Sort(records);

                    TVector<ui64> keys;
                    updatableArray.GetKeys(keys, docId);
                    for (size_t i = 0; i < keys.size(); ++i) {
                        records[i].second++;
                        updatableArray.Set(docId, keys[i], records[i].second);
                    }
                }
            }

            // Check written data
            THolder<TMemoryMap> hMapping;
            THolder<TArrayWithHead2D<TRec>> hReader;
            THolder<TArrayWithHead2D<TRec2>> hReaderPolite;

            if (useMap) {
                hMapping.Reset(new TMemoryMap(path.data()));
                hReader.Reset(new TArrayWithHead2D<TRec>(*hMapping));
                hReaderPolite.Reset(new TArrayWithHead2D<TRec2>(*hMapping, true, true));
            } else {
                hReader.Reset(new TArrayWithHead2D<TRec>(path.data()));
                hReaderPolite.Reset(new TArrayWithHead2D<TRec2>(path.data(), true, true));
            }
            TArrayWithHead2D<TRec>& reader = *hReader;
            TArrayWithHead2D<TRec2>& readerPolite = *hReaderPolite;

            for (ui32 docId = 0; docId < 1000; docId++) {
                TVector<std::pair<ui64, ui64>>& records = data[docId];
                Sort(records.begin(), records.end());

                TVector<ui64> keys, keysPolite;
                reader.GetKeys(keys, docId);
                readerPolite.GetKeys(keysPolite, docId);
                UNIT_ASSERT(keys == keysPolite);
                for (size_t i = 0; i < records.size(); i++) {
                    UNIT_ASSERT_VALUES_EQUAL(records[i].first, keys[i]);
                    UNIT_ASSERT_VALUES_EQUAL(reader.Get(docId, records[i].first).A, records[i].second);
                    UNIT_ASSERT_VALUES_EQUAL(readerPolite.Get(docId, records[i].first).A, records[i].second);
                    UNIT_ASSERT_VALUES_EQUAL(readerPolite.Get(docId, records[i].first).B, 0u);
                    if (!(i + 1 < records.size() && records[i + 1].first == keys[i] + 1)) {
                        UNIT_ASSERT_EQUAL(reader.Find(docId, keys[i] + 1), NULL);
                        UNIT_ASSERT_EQUAL(readerPolite.Find(docId, keys[i] + 1), NULL);
                    }
                }
            }
        }
    }

}

Y_UNIT_TEST_SUITE(TArrayWithHead2DTest) {
    Y_UNIT_TEST(TestSimpleUseMap) {
        RunSimpleTest(USE_MAP, NOT_USE_RAW_WRITER);
    }
    Y_UNIT_TEST(TestSimpleNotUseMap) {
        RunSimpleTest(NOT_USE_MAP, NOT_USE_RAW_WRITER);
    }
    Y_UNIT_TEST(TestRandomUseMap) {
        RunRandomTest(USE_MAP, NOT_USE_RAW_WRITER);
    }
    Y_UNIT_TEST(TestRandomNotUseMap) {
        RunRandomTest(NOT_USE_MAP, NOT_USE_RAW_WRITER);
    }
}

Y_UNIT_TEST_SUITE(TArrayWithHead2DStrokaTest) {
    Y_UNIT_TEST(TestSimpleUseMap) {
        RunSimpleTest(USE_MAP, USE_RAW_WRITER);
    }
    Y_UNIT_TEST(TestSimpleNotUseMap) {
        RunSimpleTest(NOT_USE_MAP, USE_RAW_WRITER);
    }
    Y_UNIT_TEST(TestRandomUseMap) {
        RunRandomTest(USE_MAP, USE_RAW_WRITER);
    }
    Y_UNIT_TEST(TestRandomNotUseMap) {
        RunRandomTest(NOT_USE_MAP, USE_RAW_WRITER);
    }
}
