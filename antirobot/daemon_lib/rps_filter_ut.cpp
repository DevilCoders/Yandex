#include "rps_filter.h"

#include "uid.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TTestRpsFilter) {
        const size_t defaultFilterSize = 10;
        const float defaultEps = std::numeric_limits<float>::epsilon();
        const TDuration defaultRememberForInterval = TDuration::Minutes(1);

        float rpsAfterNRequests(size_t requests) {
            return static_cast<float>(requests) / defaultRememberForInterval.Seconds();
        }

        float recalcRequest(TRpsFilter& rpsFilter, TUid& id, TInstant arrivalTime = TInstant::Zero()) {
            rpsFilter.RecalcUser(id, arrivalTime);
            return rpsFilter.GetRpsById(id);
        }

        TRpsFilter CreateFilter(size_t size) {
            TDuration defaultSafeInterval = TDuration::Minutes(1);
            return TRpsFilter(size, defaultSafeInterval, defaultRememberForInterval);
        }

        Y_UNIT_TEST(TRpsFilterSingleEntry) {
            TRpsFilter rpsFilter = CreateFilter(defaultFilterSize);
            TUid id(TUid::ENameSpace::UNK, 1);

            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, id), rpsAfterNRequests(1), defaultEps);
            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, id), rpsAfterNRequests(2), defaultEps);
            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, id), rpsAfterNRequests(3), defaultEps);
        }

        Y_UNIT_TEST(TRpsFilterCooldownCheck) {
            TRpsFilter rpsFilter = CreateFilter(defaultFilterSize);
            TUid id(TUid::ENameSpace::UNK, 1);
            TInstant cooldownTime = TInstant::Seconds(defaultRememberForInterval.Seconds() + 1);

            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, id), rpsAfterNRequests(1), defaultEps);
            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, id, cooldownTime), rpsAfterNRequests(1), defaultEps);
        }

        Y_UNIT_TEST(TRpsFilterDecrementEntry) {
            TUid alienId(TUid::ENameSpace::UNK, defaultFilterSize + 1);
            TRpsFilter rpsFilter = CreateFilter(defaultFilterSize);

            for (size_t i = 0; i < defaultFilterSize; i++) {
                TUid id(TUid::ENameSpace::UNK, i);
                UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, id), rpsAfterNRequests(1), defaultEps);
                UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, id), rpsAfterNRequests(2), defaultEps);
            }

            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, alienId), rpsAfterNRequests(0), defaultEps);

            for (size_t i = 0; i < defaultFilterSize; i++) {
                TUid id(TUid::ENameSpace::UNK, i);
                UNIT_ASSERT_DOUBLES_EQUAL(rpsFilter.GetRpsById(id), rpsAfterNRequests(1), defaultEps);
            }
        }

        Y_UNIT_TEST(TRpsFilterShrinkCheck) {
            size_t filterSize = 1;
            TRpsFilter rpsFilter = CreateFilter(filterSize);
            TUid initialId(TUid::ENameSpace::UNK, rpsAfterNRequests(0));
            TUid substituteId(TUid::ENameSpace::UNK, 1);

            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, initialId), rpsAfterNRequests(1), defaultEps);
            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, substituteId), rpsAfterNRequests(0), defaultEps);

            UNIT_ASSERT_VALUES_EQUAL(rpsFilter.GetActualSize(), 0);
        }

        Y_UNIT_TEST(TRpsFilterImplicitClearCheck) {
            size_t filterSize = 1;
            TRpsFilter rpsFilter = CreateFilter(filterSize);
            TUid initialId(TUid::ENameSpace::UNK, 0), substituteId(TUid::ENameSpace::UNK, 1);
            TInstant cooldownTime = TInstant::Seconds(defaultRememberForInterval.Seconds() + 1);

            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, initialId), rpsAfterNRequests(1), defaultEps);
            UNIT_ASSERT_DOUBLES_EQUAL(recalcRequest(rpsFilter, substituteId, cooldownTime), rpsAfterNRequests(0), defaultEps);
            UNIT_ASSERT_DOUBLES_EQUAL(rpsFilter.GetRpsById(initialId), rpsAfterNRequests(0), defaultEps);
        }
    }
}
