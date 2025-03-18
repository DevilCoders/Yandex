#include "static_hash.h"
#include "static_hash_map.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/dirut.h>
#include <util/generic/xrange.h>
#include <util/system/defaults.h>
#include <util/str_stl.h>

Y_UNIT_TEST_SUITE(TStaticHashTest) {
    using THash = THashMap<ui32, ui64>;
    using THashMM = THashMultiMap<ui32, ui64>;
    using TStHash = sthash<ui32, ui64, ::hash<ui32>>;
    using TStHashMM = sthash_mm<ui32, ui64, ::hash<ui32>>;
    using TMappedHash = sthash_mapped<ui32, ui64, ::hash<ui32>>;

    static const std::pair<ui32, ui64> TEST_DATA[] = {
        {0, 0},
        {2, 20},
        {8, 80}};

    template <class TMapType>
    static void FillHash(TMapType & map) {
        for (const auto& test : TEST_DATA) {
            map.insert(test);
        }
    }

    template <class TMapType>
    static void CheckHash(const TMapType& map) {
        UNIT_ASSERT_VALUES_EQUAL(map.size(), Y_ARRAY_SIZE(TEST_DATA));
        for (const auto& test : TEST_DATA) {
            UNIT_ASSERT_VALUES_EQUAL(map.find(test.first).Value(), test.second);
        }
    }

    static void FillHashMM(THashMM & map) {
        for (const auto& test : TEST_DATA) {
            for (ui32 i : xrange(test.first)) {
                map.insert(std::make_pair(test.first, test.second + i));
            }
        }
    }

    static std::vector<std::pair<ui32, ui64>> GetEqualRangeAsSortedVector(
        const TStHashMM& map, ui32 key) {
        std::vector<std::pair<ui32, ui64>> rangeValues;
        auto range = map.equal_range(key);
        for (auto it = range.first; it != range.second; ++it) {
            rangeValues.push_back(std::make_pair(it.Key(), it.Value()));
        }
        std::sort(rangeValues.begin(), rangeValues.end());
        return rangeValues;
    }

    static void CheckHashMM(const TStHashMM& map) {
        size_t expectedSize = 0;
        for (const auto& test : TEST_DATA) {
            expectedSize += test.first;
        }
        UNIT_ASSERT_VALUES_EQUAL(expectedSize, map.size());

        for (const auto& test : TEST_DATA) {
            const auto rangeValues = GetEqualRangeAsSortedVector(map, test.first);
            UNIT_ASSERT_VALUES_EQUAL(rangeValues.size(), test.first);
            for (ui32 i : xrange(test.first)) {
                UNIT_ASSERT_VALUES_EQUAL(rangeValues[i], std::make_pair(test.first, test.second + i));
            }
        }
    }

    Y_UNIT_TEST(TestSthash) {
        THash map;
        FillHash(map);

        TBuffer buffer;
        SaveHashToBuffer(map, buffer);

        buffer.ShrinkToFit();

        const TStHash* sthash = (const TStHash*)(buffer.data());

        CheckHash(*sthash);
    }

    Y_UNIT_TEST(TestSthashMM) {
        THashMM map;
        FillHashMM(map);

        TBuffer buffer;
        SaveHashToBuffer(map, buffer);

        buffer.ShrinkToFit();

        const TStHashMM* sthash = (const TStHashMM*)(buffer.data());

        CheckHashMM(*sthash);
    }

    Y_UNIT_TEST(TestMap) {
        THash map;
        FillHash(map);

        TString filename = GetSystemTempDir() + "/TStaticHashTest-TestMap";
        SaveHashToFile(map, filename.data());

        TMappedHash mapped(filename.data(), true);

        CheckHash(*mapped.GetSthash());

        unlink(filename.data());
    }
}
