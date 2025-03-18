#include <library/cpp/html/html5/text_normalize.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/strip.h>

static const char hello_nobom[] = "\xd0\xbf\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82\x21";               // \x21";
static const char hello_utf8bom[] = "\xef\xbb\xbf\xd0\xbf\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82\x21"; // \x21";
static const char hello_utf16le[] = "\xff\xfe\x3f\x04\x40\x04\x38\x04\x32\x04\x35\x04\x42\x04\x21\x00";
static const char hello_utf16be[] = "\xfe\xff\x04\x3f\x04\x40\x04\x38\x04\x32\x04\x35\x04\x42\x00\x21";

static const char serp_nobom[] = "\xe2\x98\xad";
static const char serp_utf16le[] = "\xff\xfe\x2d\x26";
static const char serp_utf16be[] = "\xfe\xff\x26\x2d";

class TTextNormTest: public TTestBase {
    UNIT_TEST_SUITE(TTextNormTest);
    UNIT_TEST(BOM);
    UNIT_TEST_SUITE_END();

public:
    TString NormString(const char* str, size_t len) {
        TBuffer sample;
        sample.Assign(str, len);
        NHtml::NormalizeUtfInput(&sample, true);
        TString ret;
        sample.AsString(ret);
        StripInPlace(ret);
        return ret;
    }
    void BOM() {
        UNIT_ASSERT_VALUES_EQUAL(NormString(hello_utf8bom, sizeof(hello_utf8bom) - 1), TString(hello_nobom));
        UNIT_ASSERT_VALUES_EQUAL(NormString(hello_utf16be, sizeof(hello_utf16be) - 1), TString(hello_nobom));
        UNIT_ASSERT_VALUES_EQUAL(NormString(hello_utf16le, sizeof(hello_utf16le) - 1), TString(hello_nobom));

        UNIT_ASSERT_VALUES_EQUAL(NormString(serp_utf16be, sizeof(serp_utf16be) - 1), TString(serp_nobom));
        UNIT_ASSERT_VALUES_EQUAL(NormString(serp_utf16le, sizeof(serp_utf16le) - 1), TString(serp_nobom));
    }
};
UNIT_TEST_SUITE_REGISTRATION(TTextNormTest);
