#include "calcer.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/dirut.h>

class TDocTitleRangesTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TDocTitleRangesTest);
        UNIT_TEST(TestRangeCalcer);
        UNIT_TEST(TestRangeSerializer);
        UNIT_TEST(TestScoreCalcer);
    UNIT_TEST_SUITE_END();

    THolder<TDocTitleRangesCalcer> Calcer;

private:
    size_t CountNonTrivialRanges(const TDocTitleRanges& ranges) {
        size_t count = 0;

        for (size_t i = 0; i != ranges.size(); ++i) {
            if (!ranges[i].Empty()) {
                count += 1;
            }
        }

        return count;
    }

public:
    void SetUp() override {
        Calcer.Reset(new TDocTitleRangesCalcer(nullptr));
    }

    void TestRangeCalcer() {
        bool found;

        TDocTitleRanges ranges;
        found = Calcer->CalcRanges(u"", ranges);
        UNIT_ASSERT(!found);
        UNIT_ASSERT_EQUAL(CountNonTrivialRanges(ranges), 0);

        found = Calcer->CalcRanges(u"сезон 1 эпизод 8", ranges);
        UNIT_ASSERT(found);
        UNIT_ASSERT_EQUAL(CountNonTrivialRanges(ranges), 2);
        UNIT_ASSERT_EQUAL(ranges[DRT_SEASON], TDocTitleRange(1, 1));
        UNIT_ASSERT_EQUAL(ranges[DRT_EPISODE], TDocTitleRange(8, 8));

        found = Calcer->CalcRanges(u"5 сезон эпизоды с 1 по 9", ranges);
        UNIT_ASSERT(found);
        UNIT_ASSERT_EQUAL(CountNonTrivialRanges(ranges), 2);
        UNIT_ASSERT_EQUAL(ranges[DRT_SEASON], TDocTitleRange(5, 5));
        UNIT_ASSERT_EQUAL(ranges[DRT_EPISODE], TDocTitleRange(1, 9));

        found = Calcer->CalcRanges(u"физика 7-9 класс", ranges);
        UNIT_ASSERT(found);
        UNIT_ASSERT_EQUAL(CountNonTrivialRanges(ranges), 1);
        UNIT_ASSERT_EQUAL(ranges[DRT_CLASS], TDocTitleRange(7, 9));

        found = Calcer->CalcRanges(u"химия класс 8 - 11", ranges);
        UNIT_ASSERT(found);
        UNIT_ASSERT_EQUAL(CountNonTrivialRanges(ranges), 1);
        UNIT_ASSERT_EQUAL(ranges[DRT_CLASS], TDocTitleRange(8, 11));
    }

    void TestRangeSerializer() {

        typedef std::pair<TDocTitleRange, EDocTitleRangeType> TRangePair;

        TDocTitleRange rangeAll;
        rangeAll.SetAll();

        TVector<TRangePair> samples = {TRangePair(TDocTitleRange(), DRT_SEASON),
                                       TRangePair(TDocTitleRange(5,5), DRT_EPISODE),
                                       TRangePair(rangeAll, DRT_CLASS)};

        for (size_t i = 0; i != samples.size(); ++i) {
            TDocTitleRange range;
            EDocTitleRangeType rangeType;

            ui32 code = SerializeDocTitleRangeAndType(samples[i].first, samples[i].second);
            DeserializeDocTitleRangeAndType(code, range, rangeType);

            UNIT_ASSERT_EQUAL(samples[i].first, range);
            UNIT_ASSERT_EQUAL(samples[i].second, rangeType);
        }

        TDocTitleRange range;
        EDocTitleRangeType rangeType;

        UNIT_ASSERT_EXCEPTION(SerializeDocTitleRangeAndType(TDocTitleRange(), DRT_NUM_ELEMENTS), yexception);

        UNIT_ASSERT_NO_EXCEPTION(DeserializeDocTitleRangeAndType(0x0FFFFF00, range, rangeType));
        UNIT_ASSERT_EXCEPTION(DeserializeDocTitleRangeAndType(0xFFFFFF00, range, rangeType), yexception);
    }

    void TestScoreCalcer() {
        const float epsilon = 0.01;
        const float alpha = 0.1;

        {
            TDocTitleRanges queryRanges;
            TDocTitleRanges titleRanges;

            float classScore = GetTitleRangesClassScore(queryRanges);
            float matchingScore = GetTitleRangesMatchingScore(queryRanges, titleRanges);

            UNIT_ASSERT(classScore < epsilon);
            UNIT_ASSERT(matchingScore < epsilon);
        }

        {
            TDocTitleRanges queryRanges;
            queryRanges[DRT_EPISODE] = TDocTitleRange(5, 6);
            TDocTitleRanges titleRanges;

            float classScore = GetTitleRangesClassScore(queryRanges);
            float matchingScore = GetTitleRangesMatchingScore(queryRanges, titleRanges);

            UNIT_ASSERT(classScore > alpha);
            UNIT_ASSERT(matchingScore < epsilon);
        }

        {
            TDocTitleRanges queryRanges;
            queryRanges[DRT_EPISODE] = TDocTitleRange(5, 6);
            queryRanges[DRT_SEASON].SetAll();
            TDocTitleRanges titleRanges;
            titleRanges[DRT_EPISODE] = TDocTitleRange(1, 3);

            float matchingScore = GetTitleRangesMatchingScore(queryRanges, titleRanges);

            UNIT_ASSERT(matchingScore < epsilon);
        }

        {
            TDocTitleRanges queryRanges;
            queryRanges[DRT_EPISODE] = TDocTitleRange(5, 6);
            queryRanges[DRT_SEASON] = TDocTitleRange(8, 9);
            TDocTitleRanges titleRanges;
            titleRanges[DRT_EPISODE] = TDocTitleRange(1, 3);
            titleRanges[DRT_SEASON] = TDocTitleRange(7, 8);

            float matchingScore = GetTitleRangesMatchingScore(queryRanges, titleRanges);

            UNIT_ASSERT(matchingScore < epsilon);

            titleRanges[DRT_EPISODE] = TDocTitleRange(3, 10);

            float matchingScore2 = GetTitleRangesMatchingScore(queryRanges, titleRanges);

            UNIT_ASSERT(matchingScore2 > alpha);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TDocTitleRangesTest);
