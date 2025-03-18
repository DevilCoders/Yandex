#include <library/cpp/testing/unittest/registar.h>

#include "page_template.h"

namespace NAntiRobot {

    Y_UNIT_TEST_SUITE(TTestPageTemplate) {
        Y_UNIT_TEST(TestCorrectGenHtml) {
            const TString pageTemplate =
                "Test %VAR1% abcdef %VAR2% %VAR1% %VAR2% bcda\n"
                "fgh %%%VAR2%%% %VAR1% %%%%\n"
                "xs: 5%; ??";

            TPageTemplate templ(pageTemplate, "ru");

            {
                THashMap<TStringBuf, TStringBuf> params;

                params["VAR1"] = "var1";

                UNIT_ASSERT_VALUES_EQUAL(templ.Gen(params),
                    "Test var1 abcdef  var1  bcda\n"
                    "fgh %% var1 %%\n"
                    "xs: 5%; ??"
                );
            }

            {
                THashMap<TStringBuf, TStringBuf> params;

                params["VAR1"] = "var1";
                params["VAR2"] = "var2\"'\n";

                UNIT_ASSERT_VALUES_EQUAL(templ.Gen(params),
                    "Test var1 abcdef var2&quot;&#39;\n var1 var2&quot;&#39;\n bcda\n"
                    "fgh %var2&quot;&#39;\n% var1 %%\n"
                    "xs: 5%; ??"
                );
            }
        }

        Y_UNIT_TEST(TestCorrectGenJson) {
            TPageTemplate templ(
                "{\"xyz\": \"%XYZ_A%\"}",
                "ru",
                TPageTemplate::EEscapeMode::Json
            );

            THashMap<TStringBuf, TStringBuf> params = {
                {"XYZ_A", "aaa\"\\\x13"}
            };

            UNIT_ASSERT_VALUES_EQUAL(templ.Gen(params),
                "{\"xyz\": \"aaa\\\"\\\\\\u13\"}"
            );
        }
    };
}
