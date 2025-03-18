#include <library/cpp/testing/unittest/registar.h>

#include "autoru_tamper.h"


namespace NAntiRobot {


constexpr TStringBuf TAMPER_SALT = "topsecret";


Y_UNIT_TEST_SUITE(AutoRuTamper) {
    Y_UNIT_TEST(NoContentLengthNoParams) {
        THeadersMap headers = {
            {"X-Device-UID", "cheburek"},
            {"X-Timestamp", "d3a83974b71a36c000578ec8ad05aa06"}
        };

        TCgiParameters cgiParams;

        UNIT_ASSERT(CheckAutoRuTamper(headers, cgiParams, TAMPER_SALT));
    }

    Y_UNIT_TEST(NoContentLengthWithParams) {
        THeadersMap headers = {
            {"X-Device-UID", "foobar"},
            {"X-Timestamp", "119fb223ff9441cda60379a3950ace0c"}
        };

        TCgiParameters cgiParams = {
            {"category", "CARS"},
            {"mark", "NISSAN"},
            {"model", "QASHQAI"},
            {"page", "1"},
            {"page_size", "10"},
        };

        UNIT_ASSERT(CheckAutoRuTamper(headers, cgiParams, TAMPER_SALT));
    }

    Y_UNIT_TEST(WithContentLengthWithParams) {
        THeadersMap headers = {
            {"X-Device-UID", "quux"},
            {"X-Timestamp", "0a46c52d14eeadef11330c213e3204e3"}
        };

        TCgiParameters cgiParams = {
            {"woof", "bork"}
        };

        UNIT_ASSERT(CheckAutoRuTamper(headers, cgiParams, TAMPER_SALT));
    }
}


} // namespace NAntiRobot
