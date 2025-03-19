#include "requests_inflight.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TRequestsInFlightTest)
{
    Y_UNIT_TEST(ShouldRejectIntersectingRequests)
    {
        TRequestsInFlight requestsInFlight;

        UNIT_ASSERT_VALUES_EQUAL(
            true,
            requestsInFlight.TryAddRequest(1, { 10, 20 }));

        UNIT_ASSERT_VALUES_EQUAL(
            true,
            requestsInFlight.TryAddRequest(2, { 80, 100 }));

        UNIT_ASSERT_VALUES_EQUAL(
            true,
            requestsInFlight.TryAddRequest(3, { 40, 50 }));

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(4, { 5, 15 }));

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(5, { 25, 45 }));

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(6, { 35, 55 }));

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(7, { 50, 120 }));

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(8, { 40, 50 }));

        requestsInFlight.RemoveRequest(3);

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(9, { 15, 75 }));

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(10, { 30, 85 }));

        requestsInFlight.RemoveRequest(2);

        UNIT_ASSERT_VALUES_EQUAL(
            false,
            requestsInFlight.TryAddRequest(11, { 5, 95 }));

        requestsInFlight.RemoveRequest(1);

        UNIT_ASSERT_VALUES_EQUAL(
            true,
            requestsInFlight.TryAddRequest(12, { 20, 30 }));
    }
}

}   // namespace NCloud::NBlockStore::NStorage
