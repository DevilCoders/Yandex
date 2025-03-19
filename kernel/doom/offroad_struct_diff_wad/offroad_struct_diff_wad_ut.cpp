#include <library/cpp/testing/unittest/registar.h>

#include "offroad_struct_diff_wad_ut.h"

Y_UNIT_TEST_SUITE(TOffroadStructDiffWadTest) {
    template <typename StructData>
    void DoTest(ui32 minDocId, ui32 maxDocId, ui32 docCount, bool use64BitSubWriter = false) {
        using TSelectedIndex = TIndex<StructData>;
        using TReader = typename TSelectedIndex::TReader;
        using TSearcher = typename TSelectedIndex::TSearcher;
        using TIterator = typename TSearcher::TIterator;

        TSelectedIndex index(minDocId, maxDocId, docCount, use64BitSubWriter);

        // Read all
        {
            THolder<TReader> reader = index.GetReader();
            for (ui32 iter = 0; iter < 2; ++iter, reader->Reset(index.GetWad())) {
                ui32 docId;
                const StructData* data;
                UNIT_ASSERT_GE(index.GetData().size(), 1);
                for (const auto& [docHitId, docHitData] : index.GetData()) {
                    UNIT_ASSERT(reader->Next(&docId, &data));
                    UNIT_ASSERT_EQUAL(docHitId, docId);
                    UNIT_ASSERT(memcmp(&docHitData, data, sizeof(StructData)) == 0);
                }
                UNIT_ASSERT(!reader->Next(&docId, &data));
            }
        }

        // Search all
        {
            THolder<TSearcher> searcher = index.GetSearcher();
            TIterator it;
            const auto& testData = index.GetData();
            TVector<ui32> toSearch;
            toSearch.reserve(testData.size());
            for (const auto& [docId, data] : testData) {
                toSearch.push_back(docId);
                Y_UNUSED(data);
            }
            Shuffle(toSearch.begin(), toSearch.end());
            ui32 docId;
            const StructData* data;
            for (ui32 docHitId : toSearch) {
                UNIT_ASSERT(searcher->LowerBound(docHitId, &docId, &data, &it));
                auto dataIter = LowerBound(testData.begin(), testData.end(), docHitId, [](const auto& lhs, ui32 doc) {
                    return lhs.first < doc;
                });
                UNIT_ASSERT_UNEQUAL(dataIter, testData.end());
                UNIT_ASSERT(memcmp(data, &dataIter->second, sizeof(StructData)) == 0);

                UNIT_ASSERT(it.Next(&docId, &data));
                UNIT_ASSERT_EQUAL(docId, docHitId);
                UNIT_ASSERT(memcmp(data, &dataIter->second, sizeof(StructData)) == 0);
                size_t toEnd = testData.end() - dataIter - 1;
                for (size_t i = 0; i < Min(toEnd, size_t(80)); ++i) {
                    ++dataIter;
                    UNIT_ASSERT(it.Next(&docId, &data));
                    UNIT_ASSERT_EQUAL(docId, dataIter->first);
                    UNIT_ASSERT(memcmp(data, &dataIter->second, sizeof(StructData)) == 0);
                }
                if (Min(toEnd, size_t(80)) == toEnd) {
                    UNIT_ASSERT(!it.Next(&docId, &data));
                }
            }
        }
    }

    struct TAlmostEmptyData {
        ui8 data;
    };

    Y_UNIT_TEST(AlmostEmptyIndexCheck) {
        UNIT_ASSERT_EXCEPTION(DoTest<TAlmostEmptyData>(0, 0, 1), yexception);
    }

    Y_UNIT_TEST(AlmostEmptyData2Docs) {
        DoTest<TAlmostEmptyData>(0, 1, 2);
    }

    Y_UNIT_TEST(AlmostEmptyDataBigger) {
        DoTest<TAlmostEmptyData>(0, 10, 5);
    }

    Y_UNIT_TEST(AlmostEmptyDataRange) {
        for (size_t i = 2; i <= 130; ++i) {
            DoTest<TAlmostEmptyData>(0, 300, i);
        }
    }

    Y_UNIT_TEST(AlmostEmptyDataBig) {
        DoTest<TAlmostEmptyData>(0, 10000, 10001);
    }

    Y_UNIT_TEST(AlmostEmptyDataHuge) {
        DoTest<TAlmostEmptyData>(0, 10000, 1000);
    }

    Y_UNIT_TEST(AlmostEmptyDataVeryHuge) {
        DoTest<TAlmostEmptyData>(0, 30000, 30001);
    }

    struct TSimpleData {
        ui32 first;
        ui32 second;
        ui16 third;
        ui64 fourth;
        ui8 bytik;
    };

    Y_UNIT_TEST(SimpleIndexCheck) {
        UNIT_ASSERT_EXCEPTION(DoTest<TSimpleData>(0, 0, 1), yexception);
    }

    Y_UNIT_TEST(SimpleEmptyData2Docs) {
        DoTest<TSimpleData>(0, 1, 2);
    }

    Y_UNIT_TEST(SimpleDataBiggerRange) {
        for (size_t i = 2; i <= 130; ++i) {
            DoTest<TSimpleData>(0, 300, i);
        }
    }

    Y_UNIT_TEST(SimpleDataBig) {
        DoTest<TSimpleData>(0, 10000, 10001);
    }

    Y_UNIT_TEST(SimpleDataBitSmaller) {
        DoTest<TSimpleData>(0, 10000, 1000);
    }

    Y_UNIT_TEST(SimpleDataHuge) {
        DoTest<TSimpleData>(0, 20000, 20001);
    }

    Y_UNIT_TEST(SimpleData64BitSubWriter) {
        DoTest<TSimpleData>(0, 10000, 10001, true);
    }

    struct TReallyHugeData {
        TSimpleData data1[20];
        long double dbl;
        TSimpleData data2[20];
    };

    Y_UNIT_TEST(HugeIndexCheck) {
        UNIT_ASSERT_EXCEPTION(DoTest<TReallyHugeData>(0, 0, 1), yexception);
    }

    Y_UNIT_TEST(HugeDataBiggerRange) {
        for (size_t i = 2; i <= 130; ++i) {
            DoTest<TReallyHugeData>(0, 300, i);
        }
    }

    Y_UNIT_TEST(HugeDataBig) {
        DoTest<TReallyHugeData>(0, 1000, 500);
    }

    Y_UNIT_TEST(HugeDataBitSmaller) {
        DoTest<TReallyHugeData>(0, 1000, 100);
    }

    Y_UNIT_TEST(HugeDataBitSmaller64BitSubWriter) {
        DoTest<TReallyHugeData>(0, 1000, 100, true);
    }

    Y_UNIT_TEST(HugeDataHuge) {
        DoTest<TReallyHugeData>(0, 3000, 3001);
    }
};
