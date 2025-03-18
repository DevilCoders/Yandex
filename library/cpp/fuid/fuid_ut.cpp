#include <library/cpp/testing/unittest/registar.h>

#include "fuid.h"

Y_UNIT_TEST_SUITE(TFuid) {
    Y_UNIT_TEST(Verify) {
        struct TTest {
            TString Cookie;
            TString Result;
        };

        const TTest tests[] = {
            {"4b4b70bd000658a9.Gy0uIW0eY9mqDvGgIs7KAoduolxyjZR3dPY74o7nhid9lAKXdwYn9oAHclr41V5bRuXEsJNgDSq0pn04NWTUlD1n-mp5CEdfDoudWne2mcrhODPv2GmSeDUWocprRlBW", "4159131263235261"}, {"4baa7bd60d05c6d2.KQ9mI2e88fMYYtPtkT7pUmCMANff9fj4w3Pc6IXOK4TlBSwgx9Lz-uNO0kUfowsNqpbqO2z8T2mMbY0pnj91K2qfQlnjwUvt9qzjamdpPYSozSX2TOqTcp85SwV-zZRE", "2184823861269464022"}, {"4b58a59708bb2225.F86-02Xgb2BSf1hZHSMoUClzpuz_ItMaMXidy7kv70jS_hag7vHmRKEwB-HlDlcupAfYyTJnPRc43lXKVzzDHHUqxTm7gG2DfZ_WTToaXrEsPbORyie6BqvEJhO_XPAY", "1464817011264100759"}, {"52943b397cae8d34.qdHVYCD3CM4_1f2NNHUoSzAvdehSJSRztfvoHNDISD_3CPdMI0WhFKIJtKUI1E2RcwJ2wVlTxlDlYl0KyqIw5OSf3kync7mmhgeSpmUTji1NWl1NMQMj848V6EX-5i5-", "20918141961385446201"}};

        for (size_t i = 0; i < Y_ARRAY_SIZE(tests); ++i) {
            TString res = "";

            UNIT_ASSERT(FuidChecker().VerifyFuid(tests[i].Cookie, &res));
            Cerr << "i=" << i << "\n";
            Cerr << "res =" << res << "\n";
            UNIT_ASSERT_VALUES_EQUAL(res, tests[i].Result);
        }
    }

    Y_UNIT_TEST(Invalid) {
        TString res;

        UNIT_ASSERT(!FuidChecker().VerifyFuid("4b58a59708bb2225.F86-02Xgb2BSf1hZHSMoUClzpuz_ItMaMXidy7kv70jS_hag7vHmRKEwB-HlDlcupAfYyTJnPRc43lXKVzzDHHUqxTm7gG2DfZ_WTToaXrEsPbORyie6BqvEJhO_", &res));
    }
}
