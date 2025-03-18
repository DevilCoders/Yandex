#include <library/cpp/testing/unittest/registar.h>

#include "autoru_offer.h"

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(TTestAutoruOfferDetector) {
    Y_UNIT_TEST(TestEmptySalt) {
        TAutoruOfferDetector offerDetector("foo");


        TVector<TString> urlsValid = {{
            "/foo/bar/sale/1101235346-ff93fae1",
            "/bar/baz/sale/1088678192-39dc322b/",
            "/0123/foo/sale/1105899088-94e107c0",
            "/bar/2341123/sale/1105743433-fca71aea/",
        }};

        TVector<TString> urlsInvalid = {{
            "/foo/bar/sale/1101235346-ff93fae1/foo",
            "/bar/baz/sale/1088678192-39dc322e/",
            "/0123/foo/sale/1105899088-94e107c1",
            "/bar/2341123/sale/110574433-fca71aea/",
        }};


        for (const auto& url : urlsValid) {
            UNIT_ASSERT(offerDetector.Process(url));
        }

        for (const auto& url : urlsInvalid) {
            UNIT_ASSERT(!offerDetector.Process(url));
        }
    }
}

} // namespace NAntiRobot
