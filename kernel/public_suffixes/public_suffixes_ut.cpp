#include "public_suffixes.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/str.h>

Y_UNIT_TEST_SUITE(Real2LDTest) {
    Y_UNIT_TEST(SimpleTest) {
        TStringStream rules;

        rules <<
            "// comment line 1\n"
            "ua\n"
            "com.ua\n"
            "// comment line 2\n"
            "*.uk\n"
            "!bl.uk\n";

        TPublicSuffixStorage suffixStorage;
        suffixStorage.FillFromStream(rules);

        UNIT_ASSERT_EQUAL(GetHostOwner("3.2.1.ua",     suffixStorage), "1.ua");
        UNIT_ASSERT_EQUAL(GetHostOwner("3.2.1.com.ua", suffixStorage), "1.com.ua");
        UNIT_ASSERT_EQUAL(GetHostOwner("3.2.1.ss.uk",  suffixStorage), "1.ss.uk");
        UNIT_ASSERT_EQUAL(GetHostOwner("3.2.1.bl.uk",  suffixStorage), "bl.uk");
    }
}
