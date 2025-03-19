#include <util/generic/vector.h>
#include <util/generic/string.h>

#include <library/cpp/testing/unittest/registar.h>

#include "url_to_positions.h"

class TUrlToPositionsTest : public TTestBase {
    UNIT_TEST_SUITE(TUrlToPositionsTest);
        UNIT_TEST(TestSimple);
    UNIT_TEST_SUITE_END();

public:
    void TestSimple();
};

void TUrlToPositionsTest::TestSimple() {
    TVector<TString> urls = {"www1", "www2", "www1", "www43", "fdsfd", "fdsfd", "fdsfd"}; // ("")

    TUrl2Positions url2positions;
    MakeUrl2Positions(urls, &url2positions);

    const TString filename = "testUrl2Positions";
    WriteUrl2Positions(urls.size(), url2positions, filename);

    TUrl2PositionsReader reader(filename);
    UNIT_ASSERT_EQUAL(reader.GetNUrls(), urls.size());
    for (size_t i = 0; i < urls.size(); ++i) {
        TPositions positions;
        if (url2positions[urls[i]].size()) {
            UNIT_ASSERT(reader.Get(urls[i], &positions));
            UNIT_ASSERT_EQUAL(positions, url2positions[urls[i]]);
        } else {
            UNIT_ASSERT(!reader.Get(urls[i], &positions));
        }
    }
    TPositions positions;
    UNIT_ASSERT(!reader.Get("gfdgdf", &positions));
    unlink(filename.data());
}

UNIT_TEST_SUITE_REGISTRATION(TUrlToPositionsTest);
