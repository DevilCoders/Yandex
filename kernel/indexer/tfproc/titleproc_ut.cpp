#include "titleproc.h"

#include <library/cpp/testing/unittest/registar.h>


Y_UNIT_TEST_SUITE(TitleProc) {

    Y_UNIT_TEST(TokenNormalizedTitleUTF8) {
        UNIT_ASSERT_NO_DIFF(
            TTitleHandler::GetTokenNormalizedTitleUTF8Impl(u"саур-могила", 40).Result,
            "саур могила"
        );
        UNIT_ASSERT_NO_DIFF(
            TTitleHandler::GetTokenNormalizedTitleUTF8Impl(u"А.Б.", 40).Result,
            "а б"
        );
    }
}
