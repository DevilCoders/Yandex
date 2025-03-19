#include "component_api.h"

#include <library/cpp/testing/unittest/registar.h>
#include <array>
using namespace NKnnService;

Y_UNIT_TEST_SUITE(KnnServiceComponentApi) {
    Y_UNIT_TEST(SetAndGetDistance) {
        NMetaProtocol::TDocument d;
        NKnnService::TResponseTraits::SetDistanceFromQuery(d, 0.5);
        UNIT_ASSERT(d.HasSRelevance());
        UNIT_ASSERT_DOUBLES_EQUAL(NKnnService::TResponseTraits::GetDistanceFromResultToQuery(
            std::as_const(d)
        ), 0.5, 1e-5);
    }
}
