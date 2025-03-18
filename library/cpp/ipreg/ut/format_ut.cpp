#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/ipreg/util_helpers.h>

using namespace NIPREG;

Y_UNIT_TEST_SUITE(FormatTest) {
    Y_UNIT_TEST(AddAttr) {
        const TVector<TString>& includes = { "c" };

        const TString before1 = "{\"a\":1,\"b\":2}";
        const TString after1  = "{\"a\":1,\"b\":2,\"c\":1}";
        UNIT_ASSERT_STRINGS_EQUAL(AddJsonAttrs(includes, before1, TMaybe<TString>()), after1);

        const TString before2 = "{\"a\":1}";
        const TString after2  = "{\"a\":1,\"c\":1}";
        UNIT_ASSERT_STRINGS_EQUAL(AddJsonAttrs(includes, before2, TMaybe<TString>()), after2);
    }

    Y_UNIT_TEST(AddAttrWithVal) {
        const TVector<TString>& includes = { "c" };

        const TString before1 = "{\"a\":1,\"b\":2}";
        const TString after1  = "{\"a\":1,\"b\":2,\"c\":\"value\"}";
        UNIT_ASSERT_STRINGS_EQUAL(AddJsonAttrs(includes, before1, TMaybe<TString>("value")), after1);

        const TString before2 = "{\"a\":1}";
        const TString after2  = "{\"a\":1,\"c\":\"value\"}";
        UNIT_ASSERT_STRINGS_EQUAL(AddJsonAttrs(includes, before2, TMaybe<TString>("value")), after2);
    }

    Y_UNIT_TEST(RemoveAttr) {
        const TVector<TString>& excludes = { "b" };

        const TString before1 = "{\"a\":1,\"b\":2}";
        const TString after1  = "{\"a\":1}";
        UNIT_ASSERT_STRINGS_EQUAL(ExcludeJsonAttrs(excludes, before1), after1);

        const TString before2 = "{\"z\":9,\"b\":2,\"a\":1}";
        const TString after2  = "{\"a\":1,\"z\":9}";
        UNIT_ASSERT_STRINGS_EQUAL(SortJsonData(ExcludeJsonAttrs(excludes, before2)), after2);
    }
} // ReaderTest
