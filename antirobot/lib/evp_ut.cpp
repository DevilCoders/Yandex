#include "evp.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/hex.h>


namespace NAntiRobot {


Y_UNIT_TEST_SUITE(Evp) {
    const TString MSG = "Sphinx of black quartz, judge my vow";

    const TString KEY = HexDecode(
        "4a1faf3281028650e82996f37aada9d97ef7c7c4f3c71914a7cc07a1cbb02d00"
    );

    const TString IV12 = HexDecode(
        "0dcaa77c591c23845635e710"
    );

    Y_UNIT_TEST(EncryptDecrypt) {
        TEvpEncryptor encryptor(EEvpAlgorithm::Aes256Gcm, KEY, IV12);
        const auto encrypted = encryptor.Encrypt(MSG);

        TEvpDecryptor decryptor(EEvpAlgorithm::Aes256Gcm, KEY, IV12);
        const auto decrypted = decryptor.Decrypt(encrypted);

        UNIT_ASSERT_VALUES_EQUAL(decrypted, MSG);
    }

    Y_UNIT_TEST(EncryptTamperDecrypt) {
        TEvpEncryptor encryptor(EEvpAlgorithm::Aes256Gcm, KEY, IV12);
        auto encrypted = encryptor.Encrypt(MSG);
        encrypted[4] = 0x42;

        TEvpDecryptor decryptor(EEvpAlgorithm::Aes256Gcm, KEY, IV12);

        UNIT_ASSERT_EXCEPTION(decryptor.Decrypt(encrypted), yexception);
    }

    Y_UNIT_TEST(PrefixEncryptDecrypt) {
		constexpr TStringBuf prefix{"prefix"};
		TString encrypted{prefix};

        TEvpEncryptor encryptor(EEvpAlgorithm::Aes256Gcm, KEY, IV12);
        encryptor.Encrypt(MSG, &encrypted);

		TStringBuf encryptedPrefix, data;
		TStringBuf(encrypted).SplitAt(prefix.size(), encryptedPrefix, data);

		UNIT_ASSERT_VALUES_EQUAL(encryptedPrefix, prefix);

        TEvpDecryptor decryptor(EEvpAlgorithm::Aes256Gcm, KEY, IV12);
        const auto decrypted = decryptor.Decrypt(data);

        UNIT_ASSERT_VALUES_EQUAL(decrypted, MSG);
	}
}


} // namespace NAntiRobot
