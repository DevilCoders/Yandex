#include "encodexml.h"

#include <library/cpp/testing/unittest/registar.h>

const TString str = "It's a \"test\" & <TEST>";
const TString quoted = "It&#39;s a &quot;test&quot; &amp; &lt;TEST&gt;";
const TString numbered = "It&#39;s a &#34;test&#34; &#38; &#60;TEST&#62;";

Y_UNIT_TEST_SUITE(TEncodeXml) {
    Y_UNIT_TEST(TestEncodeXML) {
        UNIT_ASSERT_VALUES_EQUAL(EncodeXML(str.data()), quoted);
    }

    Y_UNIT_TEST(TestEncodeXMLString) {
        // old const char* version
        UNIT_ASSERT_VALUES_EQUAL(EncodeXMLString(str.data()), numbered);
        // new TString-based version
        UNIT_ASSERT_VALUES_EQUAL(EncodeXMLString(str), numbered);
    }
}
