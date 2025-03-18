#include <library/cpp/testing/unittest/registar.h>

#include "fields_io.h"

Y_UNIT_TEST_SUITE(TFieldsIOTest) {
    void AssertBufferOutputContents(TBufferOutput & stream, const char* canon) {
        stream.Finish();
        TString s;
        stream.Buffer().AsString(s);
        UNIT_ASSERT_STRINGS_EQUAL(s, canon);
    }

    Y_UNIT_TEST(TestAddField) {
        TBufferOutput out;
        AddField(out, "n1", "v1");
        AddField(out, "n2", (const char*)nullptr);
        AddField(out, "n3", 12);

        TMaybe<double> m;
        AddField(out, "n4", m);

        m = 0.85;
        AddField(out, "n5", m);

        AssertBufferOutputContents(out, "n1=v1\tn3=12\tn5=0.85");
    }
}
