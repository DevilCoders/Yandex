#include "cross_enumerator.h"

#include <tools/clustermaster/common/make_vector.h>
#include <tools/clustermaster/common/vector_to_string.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(NCrossEnumeratorTest) {

    Y_UNIT_TEST(Test) {
        TCrossEnumerator en(MakeVector<TVector<TString> >(
                MakeVector<TString>("aa", "bb", "cc"),
                MakeVector<TString>("11", "22"),
                MakeVector<TString>("xx", "yy", "zz", "ww")));

        const char* fs[] = { "aa", "bb", "cc", nullptr };
        const char* ss[] = { "11", "22", nullptr };
        const char* ts[] = { "xx", "yy", "zz", "ww", nullptr };

        for (const char** f = fs; *f; ++f) {
            for (const char** s = ss; *s; ++s) {
                for (const char** t = ts; *t; ++t) {
                    UNIT_ASSERT(en.Next());
                    UNIT_ASSERT_VALUES_EQUAL(
                            ToString(MakeVector<TString>(*f, *s, *t)),
                            ToString(*en));
                }
            }
        }

        UNIT_ASSERT(!en.Next());
    }

}
