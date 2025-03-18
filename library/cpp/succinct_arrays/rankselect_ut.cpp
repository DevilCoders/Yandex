#include "rankselect.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/pop_count/popcount.h>
#include <library/cpp/select_in_word/select_in_word.h>

#include <util/generic/bitops.h>
#include <util/random/random.h>

using namespace NSuccinctArrays;

class TRankSelectTest: public TTestBase {
    UNIT_TEST_SUITE(TRankSelectTest);
    UNIT_TEST(TestEmpty);
    UNIT_TEST(TestValue);
    UNIT_TEST(TestRandom);
    UNIT_TEST(TestSizes);
    UNIT_TEST(TestLarge);
    UNIT_TEST(TestSelect);
    UNIT_TEST_SUITE_END();

private:
    bool TestBit(const ui64* bits, ui64 pos) {
        return bits[pos >> 6] & static_cast<ui64>(1) << (pos & 63);
    }
    void SetBit(ui64* bits, ui64 pos) {
        bits[pos >> 6] |= static_cast<ui64>(1) << (pos & 63);
    }
    ui64 CountBits(const ui64* bits, ui64 numBits) {
        ui64 count = 0;
        for (ui64 i = 0; i < (numBits + 63) / 64; ++i) {
            count += ::PopCount(bits[i]);
        }
        return count;
    }
    void AssertRank(const ui64* bits, ui64 numBits) {
        TRank rank(bits, numBits);
        for (ui64 j = 0, i = 0; i < numBits; i++) {
            UNIT_ASSERT_EQUAL(j, rank.Rank(bits, i));
            if (TestBit(bits, i))
                j++;
        }
    }
    void AssertSelect(const ui64* bits, ui64 numBits) {
        TRankSelect select(bits, numBits);
        for (ui64 j = 0, i = 0; i < numBits; i++) {
            if (TestBit(bits, i)) {
                UNIT_ASSERT_EQUAL(i, select.Select(bits, j));
                j++;
            }
        }
    }
    void AssertRankSelect(const ui64* bits, ui64 numBits) {
        TRankSelect select(bits, numBits);
        ui64 count = CountBits(bits, numBits);
        for (ui64 j = 0, i = 0; i < numBits; ++i) {
            UNIT_ASSERT_EQUAL(j, select.Rank(bits, i));
            if (TestBit(bits, i)) {
                UNIT_ASSERT_EQUAL(i, select.Select(bits, j));
                j++;
            }
        }

        for (ui64 i = 0; i < select.Rank(bits, numBits); ++i)
            UNIT_ASSERT_EQUAL(select.Rank(bits, select.Select(bits, i)), i);

        for (ui64 i = 0; i < numBits && !count; ++i)
            UNIT_ASSERT_EQUAL(TRankSelect::npos, select.Select(bits, i));
        UNIT_ASSERT_EQUAL(count, select.Rank(bits, numBits));
        UNIT_ASSERT_EQUAL(TRankSelect::npos, select.Select(bits, count));
        UNIT_ASSERT_EQUAL(TRankSelect::npos, select.Select(bits, count + 1));
    }

public:
    void TestEmpty() {
        {
            ui64 bits[] = {0ULL};
            AssertRankSelect(bits, 0);
        }
        {
            ui64 bits[] = {0ULL};
            AssertRankSelect(bits, 1);
        }
        {
            ui64 bits[] = {0ULL};
            AssertRankSelect(bits, 64);
        }
        {
            ui64 bits[] = {0ULL, 0ULL};
            AssertRankSelect(bits, 128);
        }
        {
            ui64 bits[] = {0ULL};
            AssertRankSelect(bits, 63);
        }
        {
            ui64 bits[] = {0ULL, 0ULL};
            AssertRankSelect(bits, 65);
        }
        {
            ui64 bits[] = {0ULL, 0ULL, 0ULL};
            AssertRankSelect(bits, 129);
        }
    }
    void TestValue() {
        {
            ui64 bits[] = {1ULL};
            AssertRankSelect(bits, 64);
        }
        {
            ui64 bits[] = {1ULL << 63};
            AssertRankSelect(bits, 64);
        }
        {
            ui64 bits[] = {1ULL, 0ULL};
            AssertRankSelect(bits, 65);
        }
        {
            ui64 bits[] = {1ULL << 63, 0ULL};
            AssertRankSelect(bits, 65);
        }
        {
            ui64 bits[] = {1ULL, 0ULL};
            AssertRankSelect(bits, 128);
        }
        {
            ui64 bits[] = {1ULL << 63, 0ULL};
            AssertRankSelect(bits, 128);
        }
        {
            ui64 bits[] = {1ULL, 0ULL, 0ULL};
            AssertRankSelect(bits, 129);
        }
        {
            ui64 bits[] = {1ULL << 63, 0ULL, 0ULL};
            AssertRankSelect(bits, 129);
        }
    }
    void TestRandom() {
        TVector<ui64> bits((4096 + 63) / 64, 0ULL);
        for (size_t i = 0; i < 4096; ++i)
            if (RandomNumber<ui8>(2))
                SetBit(&bits[0], i);
        AssertRankSelect(&bits[0], bits.size() * 64);
    }
    void TestSizes() {
        for (size_t size = 1; size <= 4096; ++size) {
            TVector<ui64> bits((size + 63) / 64, 0ULL);
            for (size_t i = (size + 1) / 2; i-- != 0;)
                SetBit(&bits[0], i * 2);
            TRankSelect select(&bits[0], size);
            for (ui64 i = size + 1; i-- != 0;) {
                UNIT_ASSERT_VALUES_EQUAL((i + 1) / 2, select.Rank(&bits[0], i));
            }
            for (ui64 i = size / 2; i-- != 0;)
                UNIT_ASSERT_VALUES_EQUAL(i * 2, select.Select(&bits[0], i));
        }
    }
    void TestLarge() {
        ui64 numBits = 44000000ULL;
        TVector<ui64> bits((numBits + 63) / 64, 0x5555555555555555ULL);
        TRankSelect select(&bits[0], numBits);
        for (ui64 i = 0; i < numBits / 2; i++) {
            UNIT_ASSERT_EQUAL(i * 2ULL, select.Select(&bits[0], i));
            UNIT_ASSERT_EQUAL(i, select.Rank(&bits[0], i * 2ULL));
        }
    }

    void TestSelect() {
        UNIT_ASSERT_VALUES_EQUAL(0, SelectInWord(1, 0));
        for (int i = 0; i < 64; ++i)
            UNIT_ASSERT_VALUES_EQUAL(i, SelectInWord(0xFFFFFFFFFFFFFFFFULL, i));
        for (int i = 1; i < 32; ++i)
            UNIT_ASSERT_VALUES_EQUAL(i * 2 + 1, SelectInWord(0xAAAAAAAAAAAAAAAAULL, i));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TRankSelectTest);
