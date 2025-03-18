#include <library/cpp/ssh_sign/sign.h>

#include <library/cpp/ssh/sign_key.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(Sign) {
    static const TString PrivateKeyName = ArcadiaSourceRoot() + "/library/cpp/ssh_sign/ut/test_key";

    static const THashMap<TStringBuf, TString> PrivKeyEtalon = {
        {TStringBuf("very_random_string"),
         Base64Decode(
             "fIHIitqCNc4mKvqbhax25DsIHE1YXmRH5plzKBsiKqJt7P4sNLzNhYssLjHwg25p+n6cQz/Hx+v1mYBlmKf/jO7ObWTW2UF4t0LGn411MmHx/NAvRthNet7yoBkkxUbulBDAHe0S6kbmCPdQoCSowwr/1Ft6sJp96dX8gAaYfx21gxneRQ5pXeDcALwy3B3/SObwv5+4S7Iu27PYQMUkOhOJkW6rQy1yZwZygs+9+zb94Iafbw9KK926S44IlUJByCKmYRXDhS4czSC8f4V7bJsfRjVW0MBnmxYvSR9Zd1qhTQgaaX10BZ6NZsxpWiZwHAcBbgAdK5guBSNdsc+3yw==")},
    };

    Y_UNIT_TEST(ByKey) {
        const TStringBuf data = "very_random_string";
        const TString etalonSignedData = PrivKeyEtalon.at(data);

        auto signer = NSS::SignByRsaKey(PrivateKeyName);
        UNIT_ASSERT(signer);

        std::optional<NSS::TResult> result = signer->SignNext(data);
        UNIT_ASSERT(result);

        const auto* err = std::get_if<NSS::TErrorMsg>(&result.value());
        UNIT_ASSERT_C(!err, (TString)*err);
        const NSS::TSignedData* signedData = std::get_if<NSS::TSignedData>(&result.value());
        UNIT_ASSERT_EQUAL(signedData->Sign, etalonSignedData);
        UNIT_ASSERT_EQUAL(signedData->Type, NSS::ESignType::KEY);

        UNIT_ASSERT(!signer->SignNext(data));
    }
}
