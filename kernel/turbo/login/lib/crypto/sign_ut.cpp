#include "key_gen.h"
#include "key_provider.h"
#include "sign.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/datetime/base.h>

using namespace NTurboLogin;

Y_UNIT_TEST_SUITE(TTurboSignTest) {

    Y_UNIT_TEST(CheckIsUrlSignatureValid) {
        TKeyProvider keyProvider(GenerateSecret(1));

        TString data = "https://kp.ru/moscow";
        TString data2 = "https://kp.ru/moscow2";

        TInstant now = TInstant::Seconds(1600000000);
        TString sign = Sign(&keyProvider, data, now);
        TString sign2 = Sign(&keyProvider, data2, now);

        UNIT_ASSERT(sign.size() > 0);

        UNIT_ASSERT(
            ValidateSign(
                &keyProvider,
                data,
                sign,
                now + TDuration::Hours(12),
                TDuration::Hours(12)
            )
        );

        UNIT_ASSERT(
            !ValidateSign(
                &keyProvider,
                data,
                sign2,
                now + TDuration::Hours(12),
                TDuration::Hours(12)
            )
        );

        UNIT_ASSERT(
            !ValidateSign(
                &keyProvider,
                data,
                sign,
                now + TDuration::Hours(12) + TDuration::Seconds(1),
                TDuration::Hours(12)
            )
        );
    }

    Y_UNIT_TEST(MalformedSign) {
        TKeyProvider keyProvider(GenerateSecret(1));
        TString data = "https://kp.ru/moscow";
        UNIT_ASSERT(
            !ValidateSign(
                &keyProvider,
                data,
                "12xy:1600000000",
                TInstant::Seconds(1600000001)
            )
        );
        UNIT_ASSERT(
            !ValidateSign(
                &keyProvider,
                data,
                "abcd",
                TInstant::Seconds(1600000001)
            )
        );
        UNIT_ASSERT(
            !ValidateSign(
                &keyProvider,
                data,
                "abcd:",
                TInstant::Seconds(1600000001)
            )
        );
        UNIT_ASSERT(
            !ValidateSign(
                &keyProvider,
                data,
                "abcde:",
                TInstant::Seconds(1600000001)
            )
        );
    }

    Y_UNIT_TEST(CheckSignature) {
        const TVector<ui8> signatureKey = {1, 2, 3, 4, 5};

        UNIT_ASSERT_STRINGS_EQUAL(
            Sign(signatureKey, "example.com", TInstant::Seconds(1584089060)),
            "854b52b2e8b9535e7443f2f57812a0ab02a6965f0451e70f1200061b8bb77030:1584089060"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            Sign(signatureKey, "example.com", TInstant::Seconds(1577836800)),
            "b2d48a71bc8fd882cccddbd43fc99389d4fe3a3debe73636c37ef3a47517be47:1577836800"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            Sign(signatureKey, "https://example.com/some_path", TInstant::Seconds(1584089060)),
            "4a594b7d9a994c702513e9a6da2864ab5c7fd4dffa2c7669c0de26a6db933a42:1584089060"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            Sign(signatureKey, "https://example.com/some_path", TInstant::Seconds(1577836800)),
            "65fd8bc18543ef8767f04bd46efc54779715653a5f92e9ce0d560d597e39ffeb:1577836800"
        );
    }

    Y_UNIT_TEST(EmptyKey) {
        TString data = "https://kp.ru/moscow";
        TString data2 = "https://kp.ru/moscow2";

        TInstant now = TInstant::Seconds(1600000000);
        TString sign = Sign(TVector<ui8>(), data, now);
        UNIT_ASSERT(sign.empty());
    }
}
