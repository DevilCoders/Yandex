#include <kernel/ycookie/ycookie.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/writer/json.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>

class TYCookieTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TYCookieTest);
    UNIT_TEST(TestParser);
    UNIT_TEST_SUITE_END();

// Methods
public:
    void TestParser();
};

UNIT_TEST_SUITE_REGISTRATION(TYCookieTest);

static inline void TestParser(const TString cookieName, const TString cookieValue, const TString expectedResult) {
    NJson::TJsonValue result;

    if (!NYCookie::TryParse(cookieName, cookieValue, result)) {
        UNIT_FAIL(TString("NYCookie::TryParse() doesn't know how to parse ") + cookieName);
    }

    NJsonWriter::TBuf buf;
    buf.WriteJsonValue(&result, /* sortKeys = */true);
    const auto& resultStr = buf.Str();

    if (resultStr != expectedResult) {
        TString diff = "\n";
        diff.append("Result:   ").append(resultStr).append("\n");
        diff.append("Expected: ").append(expectedResult).append("\n");
        UNIT_FAIL(diff);
    }
}

void TYCookieTest::TestParser() {
    ::TestParser(
        "ys",
        "ybzcc.ru#def_bro.1#wlp.custogray",
        "{\"def_bro\":\"1\",\"wlp\":\"custogray\",\"ybzcc\":\"ru\"}"
    );

    ::TestParser(
        "yp",
        "1531021657.cld.1955450#1531021657.gd.pvE04GDJXGGaKg1pjgLeCA7y/n8Fj14auLnwdi789dT0RMKGE5ESsbOpmPE6qmVX7WdzHQ%3D%3D#1515322301.szm.2:1280x800:1279x704#1502274767.los.1",
        "{\"cld\":{\"expires\":\"1531021657\",\"value\":\"1955450\"},\"gd\":{\"expires\":\"1531021657\",\"value\":\"pvE04GDJXGGaKg1pjgLeCA7y\\/n8Fj14auLnwdi789dT0RMKGE5ESsbOpmPE6qmVX7WdzHQ==\"},\"los\":{\"expires\":\"1502274767\",\"value\":\"1\"},\"szm\":{\"expires\":\"1515322301\",\"value\":\"2:1280x800:1279x704\"}}"
    );
}
