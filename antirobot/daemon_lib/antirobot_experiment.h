#pragma once

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/string/cast.h>

#include <quality/ab_testing/usersplit_lib/hash_calc/hash_calc.h>

namespace NAntiRobot {

    class TAntirobotExperiment {
    public:
        TAntirobotExperiment(ui32 testId1, ui32 testId2, TStringBuf salt, i64 threshold, i64 threshold2) 
            : HashCalculator(BUCKETS_COUNT, SLOTS_COUNT, salt)
            , TestId1(testId1)
            , TestId2(testId2)
            , Threshold(threshold)
            , Threshold2(threshold2)
        {}

        std::pair<ui32, ui32> GetTestId() const {
            return std::make_pair(TestId1, TestId2);
        }

        std::tuple<bool, ui32, NUserSplit::TBucket> GetExpInfo(const TStringBuf data) const {
            NUserSplit::TBucket bucket;
            NUserSplit::TSlot slot;

            HashCalculator.GetBucketAndSlot(data, bucket, slot);
            const bool inExperiment = slot < Threshold;
            const ui32 testId = inExperiment ? TestId1 : (slot < Threshold + Threshold2 ? TestId2 : 0);

            return std::make_tuple(inExperiment, testId, bucket);
        }

    private:
        static constexpr size_t BUCKETS_COUNT = 100;
        static constexpr size_t SLOTS_COUNT = 100;
        NUserSplit::THashCalculator HashCalculator;
        ui32 TestId1; // попадающий в выборку
        ui32 TestId2; // не попадающий в выборку
        i64 Threshold;
        i64 Threshold2;
    };

    struct TAntirobotDisableExperimentsFlag {
        TAtomic Enable = 0;
    };

} // namespace NAntiRobot
