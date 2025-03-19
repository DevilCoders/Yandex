#include "compaction_map.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TCompactionMapTest)
{
    Y_UNIT_TEST(ShouldKeepRangeStats)
    {
        TCompactionMap compactionMap;

        compactionMap.Update(0, 10, 100);
        compactionMap.Update(1, 11, 101);
        compactionMap.Update(10000, 20, 200);
        compactionMap.Update(20000, 5, 50);

        auto stats = compactionMap.Get(0);
        UNIT_ASSERT_VALUES_EQUAL(10, stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(100, stats.DeletionsCount);

        stats = compactionMap.Get(1);
        UNIT_ASSERT_VALUES_EQUAL(11, stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(101, stats.DeletionsCount);

        stats = compactionMap.Get(10000);
        UNIT_ASSERT_VALUES_EQUAL(20, stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(200, stats.DeletionsCount);

        stats = compactionMap.Get(20000);
        UNIT_ASSERT_VALUES_EQUAL(5, stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(50, stats.DeletionsCount);

        stats = compactionMap.Get(30000);
        UNIT_ASSERT_VALUES_EQUAL(0, stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(0, stats.DeletionsCount);
    }

    Y_UNIT_TEST(ShouldSelectTopRangeByCompactionScore)
    {
        TCompactionMap compactionMap;

        compactionMap.Update(0, 10, 100);
        compactionMap.Update(1, 11, 101);
        compactionMap.Update(10000, 20, 200);
        compactionMap.Update(20000, 5, 50);

        auto counter = compactionMap.GetTopCompactionScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 10000);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 20);

        compactionMap.Update(10000, 1, 200);

        counter = compactionMap.GetTopCompactionScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 1);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 11);

        compactionMap.Update(1, 1, 101);

        counter = compactionMap.GetTopCompactionScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 0);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 10);

        compactionMap.Update(0, 1, 100);

        counter = compactionMap.GetTopCompactionScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 20000);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 5);

        compactionMap.Update(20000, 1, 50);

        counter = compactionMap.GetTopCompactionScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 1);
    }

    Y_UNIT_TEST(ShouldSelectTopRangeByCleanupScore)
    {
        TCompactionMap compactionMap;

        compactionMap.Update(0, 10, 100);
        compactionMap.Update(1, 11, 101);
        compactionMap.Update(10000, 20, 200);
        compactionMap.Update(20000, 5, 50);

        auto counter = compactionMap.GetTopCleanupScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 10000);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 200);

        compactionMap.Update(10000, 20, 0);

        counter = compactionMap.GetTopCleanupScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 1);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 101);

        compactionMap.Update(1, 11, 0);

        counter = compactionMap.GetTopCleanupScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 0);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 100);

        compactionMap.Update(0, 10, 0);

        counter = compactionMap.GetTopCleanupScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.first, 20000);
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 50);

        compactionMap.Update(20000, 5, 0);

        counter = compactionMap.GetTopCleanupScore();
        UNIT_ASSERT_VALUES_EQUAL(counter.second, 0);
    }

    Y_UNIT_TEST(ShouldSelectTopRangesForMonitoring)
    {
        TCompactionMap compactionMap;

        compactionMap.Update(0, 10, 444);
        compactionMap.Update(1, 11, 222);
        compactionMap.Update(10000, 20, 111);
        compactionMap.Update(20000, 5, 333);

        auto topRanges = compactionMap.GetTopRangesByCompactionScore(3);
        UNIT_ASSERT_VALUES_EQUAL(3, topRanges.size());
        UNIT_ASSERT_VALUES_EQUAL(10000, topRanges[0].RangeId);
        UNIT_ASSERT_VALUES_EQUAL(20, topRanges[0].Stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(111, topRanges[0].Stats.DeletionsCount);
        UNIT_ASSERT_VALUES_EQUAL(1, topRanges[1].RangeId);
        UNIT_ASSERT_VALUES_EQUAL(11, topRanges[1].Stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(222, topRanges[1].Stats.DeletionsCount);
        UNIT_ASSERT_VALUES_EQUAL(0, topRanges[2].RangeId);
        UNIT_ASSERT_VALUES_EQUAL(10, topRanges[2].Stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(444, topRanges[2].Stats.DeletionsCount);

        topRanges = compactionMap.GetTopRangesByCleanupScore(3);
        UNIT_ASSERT_VALUES_EQUAL(3, topRanges.size());
        UNIT_ASSERT_VALUES_EQUAL(0, topRanges[0].RangeId);
        UNIT_ASSERT_VALUES_EQUAL(10, topRanges[0].Stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(444, topRanges[0].Stats.DeletionsCount);
        UNIT_ASSERT_VALUES_EQUAL(20000, topRanges[1].RangeId);
        UNIT_ASSERT_VALUES_EQUAL(5, topRanges[1].Stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(333, topRanges[1].Stats.DeletionsCount);
        UNIT_ASSERT_VALUES_EQUAL(1, topRanges[2].RangeId);
        UNIT_ASSERT_VALUES_EQUAL(11, topRanges[2].Stats.BlobsCount);
        UNIT_ASSERT_VALUES_EQUAL(222, topRanges[2].Stats.DeletionsCount);
    }
}

}   // namespace NCloud::NFileStore::NStorage
