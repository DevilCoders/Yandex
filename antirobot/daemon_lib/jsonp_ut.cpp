#include <library/cpp/testing/unittest/registar.h>

#include "jsonp.h"

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TTestJSONP) {
        Y_UNIT_TEST(CheckCallbackCorrectness) {
            const std::pair<TString, bool> CALLBACKS[] = {
              {"aaa_BBB_1234567890", true},
              {"__z_Z___",           true},
              {"",                   false},
              {".",                  false},
              {"qwerty%",            false},
              {"/abc",               false},
              {"(12345)",            false},
              {"func()",             false},
              {"obj->t",             false},
              {"{ref}&",             false}
            };

            for (const auto& value : CALLBACKS) {
                UNIT_ASSERT_VALUES_EQUAL(IsCorrectJsonpCallback(TCgiParameters("&callback=" + value.first)), value.second);
            }
        }
    }
}
