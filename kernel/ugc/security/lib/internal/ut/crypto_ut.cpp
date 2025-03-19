#include "crypto.h"

#include <kernel/ugc/security/lib/exception.h>
#include <kernel/ugc/security/proto/crypto_bundle.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/hex.h>
#include <util/system/byteorder.h>

using namespace NUgc::NSecurity::NInternal;

class TCryptoTest: public TTestBase {
public:
    UNIT_TEST_SUITE(TCryptoTest);
    UNIT_TEST(Rfc4231TestCase1);
    UNIT_TEST(Rfc4231TestCase2);
    UNIT_TEST(Rfc4231TestCase3);
    UNIT_TEST(Rfc4231TestCase4);
    UNIT_TEST(Rfc4231TestCase5);
    UNIT_TEST(Rfc4231TestCase6);
    UNIT_TEST(Rfc4231TestCase7);
    UNIT_TEST(Aes256GcmEncryptDecrypt);
    UNIT_TEST(Aes256GcmDecryptInvalidData);
    UNIT_TEST(Aes256GcmEncryptDecryptCase2);
    UNIT_TEST(Aes256GcmChecksKeyLength);
    UNIT_TEST(BlowfishChecksKeyLength);
    UNIT_TEST(BlowfishTestVectors);
    UNIT_TEST(ParseRsaPemPrivateKey);
    UNIT_TEST(RsaSha256Sign);
    UNIT_TEST_SUITE_END();

    void Rfc4231TestCase1() {
        TString key = HexDecode("0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B");
        TString data = "Hi There";
        TString expected =
            "87AA7CDEA5EF619D4FF0B4241A1D6CB02379F4E2CE4EC2787AD0B30545E17CDE"
            "DAA833B7D6B8A702038B274EAEA3F4E4BE9D914EEB61F1702E696C203A126854";

        CheckHmac(key, data, expected);
    }

    void Rfc4231TestCase2() {
        // Test with a key shorter than the length of the HMAC output.
        TString key = "Jefe";
        TString data = "what do ya want for nothing?";
        TString expected =
            "164B7A7BFCF819E2E395FBE73B56E0A387BD64222E831FD610270CD7EA250554"
            "9758BF75C05A994A6D034F65F8F0E6FDCAEAB1A34D4A6B4B636E070A38BCE737";

        CheckHmac(key, data, expected);
    }

    void Rfc4231TestCase3() {
        // Test with a combined length of key and data that is larger than 64 bytes
        // (= block-size of SHA-224 and SHA-256).
        TString key = HexDecode("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        TString data = HexDecode(
            "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
            "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
        TString expected =
            "FA73B0089D56A284EFB0F0756C890BE9B1B5DBDD8EE81A3655F83E33B2279D39"
            "BF3E848279A722C806B485A47E67C807B946A337BEE8942674278859E13292FB";

        CheckHmac(key, data, expected);
    }

    void Rfc4231TestCase4() {
        // Test with a combined length of key and data that is larger than 64 bytes
        // (= block-size of SHA-224 and SHA-256).
        TString key = HexDecode("0102030405060708090A0B0C0D0E0F10111213141516171819");
        TString data = HexDecode(
            "CDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCD"
            "CDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCD");
        TString expected =
            "B0BA465637458C6990E5A8C5F61D4AF7E576D97FF94B872DE76F8050361EE3DB"
            "A91CA5C11AA25EB4D679275CC5788063A5F19741120C4F2DE2ADEBEB10A298DD";

        CheckHmac(key, data, expected);
    }

    void Rfc4231TestCase5() {
        // Test with a truncation of output to 128 bits.
        TString key = HexDecode("0C0C0C0C0C0C0C0C0C0C0C0C0C0C0C0C0C0C0C0C");
        TString data = "Test With Truncation";
        TString expected = "415FAD6271580A531D4179BC891D87A6";

        TString result = Hmac(key, data);
        result.remove(16);
        result = HexEncode(result);

        UNIT_ASSERT_STRINGS_EQUAL(expected, result);
    }

    void Rfc4231TestCase6() {
        // Test with a key larger than 128 bytes (= block-size of SHA-384 and SHA-512).
        TString key = HexDecode(
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        TString data = "Test Using Larger Than Block-Size Key - Hash Key First";
        TString expected =
            "80B24263C7C1A3EBB71493C1DD7BE8B49B46D1F41B4AEEC1121B013783F8F352"
            "6B56D037E05F2598BD0FD2215D6A1E5295E64F73F63F0AEC8B915A985D786598";

        CheckHmac(key, data, expected);
    }

    void Rfc4231TestCase7() {
        // Test with a key and data that is larger than 128 bytes
        // (= block-size of SHA-384 and SHA-512).
        TString key = HexDecode(
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        TString data =
            "This is a test using a larger than block-size key and a larger than block-size data. "
            "The key needs to be hashed before being used by the HMAC algorithm.";
        TString expected =
            "E37B6A775DC87DBAA4DFA9F96E5E3FFDDEBD71F8867289865DF5A32D20CDC944"
            "B6022CAC3C4982B10D5EEB55C3E4DE15134676FB6DE0446065C97440FA8C6A58";

        CheckHmac(key, data, expected);
    }

    void CheckHmac(const TString& key, const TString& data, const TString& expected) {
        TString result = Hmac(key, data);
        result = HexEncode(result);

        UNIT_ASSERT_STRINGS_EQUAL(expected, result);
    }

    void Aes256GcmEncryptDecrypt() {
        TString key = HexDecode("37ccdba1d929d6436c16bba5b5ff34deec88ed7df3d15d0f4ddf80c0c731ee1f");
        TString nonce = HexDecode("5c1b21c8998ed6299006d3f9");
        TString plaintext = HexDecode("ad4260e3cdc76bcc10c7b2c06b80b3be948258e5ef20c508a81f51e96a518388");
        TString ciphertext = HexDecode("3b335f8b08d33ccdcad228a74700f1007542a4d1e7fc1ebe3f447fe71af29816");
        TString aad = HexDecode("22ed235946235a85a45bc5fad7140bfa");
        TString tag = HexDecode("1fbf49cc46f458bf6e88f6370975e6d4");

        NUgc::NSecurity::TCryptoBundle cryptoBundle;
        cryptoBundle.SetPlaintext(plaintext);
        cryptoBundle.SetNonce(nonce);
        cryptoBundle.SetAad(aad);
        Aes256GcmEncrypt(key, cryptoBundle);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetCiphertext(), ciphertext);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetTag(), tag);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetPlaintext(), ""); // Aes256GcmEncrypt must clear plaintext after encryption

        UNIT_ASSERT(Aes256GcmDecrypt(key, cryptoBundle));
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetPlaintext(), plaintext);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetCiphertext(), "");
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetTag(), "");
    }

    void Aes256GcmDecryptInvalidData() {
        TString key = HexDecode("37ccdba1d929d6436c16bba5b5ff34deec88ed7df3d15d0f4ddf80c0c731ee1f");
        TString nonce = HexDecode("5c1b21c8998ed6299006d3f9");
        TString plaintext = HexDecode("ad4260e3cdc76bcc10c7b2c06b80b3be948258e5ef20c508a81f51e96a518388");
        TString ciphertext = HexDecode("3b335f8b08d33ccdcad228a74700f1007542a4d1e7fc1ebe3f447fe71af29816");
        TString aad = HexDecode("22ed235946235a85a45bc5fad7140bfa");
        TString tag = HexDecode("1fbf49cc46f458bf6e88f6370975e6d4");

        NUgc::NSecurity::TCryptoBundle cryptoBundle;
        cryptoBundle.SetCiphertext(ciphertext);
        cryptoBundle.SetNonce(nonce);
        cryptoBundle.SetTag(tag);

        // try decrypting with invalid aad
        cryptoBundle.SetAad(HexDecode("22ed235946235a85a45bc5fad7140bfb"));
        UNIT_ASSERT(!Aes256GcmDecrypt(key, cryptoBundle));
        cryptoBundle.SetAad(aad);

        // try decrypting wotj invalid tag
        cryptoBundle.SetTag(HexDecode("1fbf49cc46f458bf6e88f6370975e6d5"));
        UNIT_ASSERT(!Aes256GcmDecrypt(key, cryptoBundle));
        cryptoBundle.SetTag(tag);

        // try decrypting with wrong key
        TString wrongKey = HexDecode("5fe01c4baf01cbe07796d5aaef6ec1f45193a98a223594ae4f0ef4952e82e331");
        UNIT_ASSERT(!Aes256GcmDecrypt(wrongKey, cryptoBundle));

        // try decrypting with invalid nonce
        cryptoBundle.SetNonce(HexDecode("bd587321566c7f1a5dd8652e"));
        UNIT_ASSERT(!Aes256GcmDecrypt(key, cryptoBundle));
        cryptoBundle.SetNonce(nonce);

        // try decrypting invalid ciphertext
        cryptoBundle.SetCiphertext(HexDecode("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
        UNIT_ASSERT(!Aes256GcmDecrypt(key, cryptoBundle));
        cryptoBundle.SetCiphertext(ciphertext);

        // try decrypting valid data
        UNIT_ASSERT(Aes256GcmDecrypt(key, cryptoBundle));
    }

    void Aes256GcmEncryptDecryptCase2() {
        TString key = HexDecode("148579a3cbca86d5520d66c0ec71ca5f7e41ba78e56dc6eebd566fed547fe691");
        TString nonce = HexDecode("b08a5ea1927499c6ecbfd4e0");
        // plaintext length is 51 byte
        TString plaintext = HexDecode("9d0b15fdf1bd595f91f8b3abc0f7dec927dfd4799935a1795d9ce00c9b8"
                                        "79434420fe42c275a7cd7b39d638fb81ca52b49dc41");
        TString ciphertext = HexDecode("2097e372950a5e9383c675e89eea1c314f999159f5611344b298cda45e"
                                        "62843716f215f82ee663919c64002a5c198d7878fd3f");
        TString aad = HexDecode("e4f963f015ffbb99ee3349bbaf7e8e8e6c2a71c230a48f9d59860a29091d2747e"
                                "01a5ca572347e247d25f56ba7ae8e05cde2be3c97931292c02370208ecd097ef6"
                                "92687fecf2f419d3200162a6480a57dad408a0dfeb492e2c5d");
        TString tag = HexDecode("adbecdb0d5c2224d804d2886ff9a5760");

        NUgc::NSecurity::TCryptoBundle cryptoBundle;
        cryptoBundle.SetPlaintext(plaintext);
        cryptoBundle.SetNonce(nonce);
        cryptoBundle.SetAad(aad);
        Aes256GcmEncrypt(key, cryptoBundle);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetCiphertext(), ciphertext);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetTag(), tag);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetPlaintext(), ""); // Aes256GcmEncrypt must clear plaintext after encryption

        UNIT_ASSERT(Aes256GcmDecrypt(key, cryptoBundle));
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetPlaintext(), plaintext);
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetCiphertext(), "");
        UNIT_ASSERT_STRINGS_EQUAL(cryptoBundle.GetTag(), "");
    }

    void Aes256GcmChecksKeyLength() {
        NUgc::NSecurity::TCryptoBundle cryptoBundle;
        cryptoBundle.SetPlaintext("plaintext");
        cryptoBundle.SetCiphertext("ciphertext");
        cryptoBundle.SetNonce("noncenonceno");

        UNIT_ASSERT_EXCEPTION_CONTAINS(
            Aes256GcmEncrypt("short-key", cryptoBundle),
            NUgc::NSecurity::TInternalException,
            "it must be 32");

        UNIT_ASSERT_EXCEPTION_CONTAINS(
            Aes256GcmDecrypt("short-key", cryptoBundle),
            NUgc::NSecurity::TInternalException,
            "it must be 32");
    }

    void BlowfishChecksKeyLength() {
        UNIT_ASSERT_EXCEPTION(TBlowfish("shrtkey"), NUgc::NSecurity::TApplicationException);
        UNIT_ASSERT_EXCEPTION(TBlowfish("12345678901234567890123456789012345678901234567890"),
                              NUgc::NSecurity::TApplicationException);
        UNIT_ASSERT_NO_EXCEPTION(TBlowfish("12345678901234567890"));
    }

    void BlowfishTestVectors() {
        struct TTestVector {
            const char* Key;
            ui64 Plaintext;
            ui64 Ciphertext;
        };
        TTestVector schneier[] = {
            {"0000000000000000", 0x0000000000000000ull, 0x4EF997456198DD78ull},
            {"FFFFFFFFFFFFFFFF", 0xFFFFFFFFFFFFFFFFull, 0x51866FD5B85ECB8Aull},
            {"3000000000000000", 0x1000000000000001ull, 0x7D856F9A613063F2ull},
            {"1111111111111111", 0x1111111111111111ull, 0x2466DD878B963C9Dull},
            {"0123456789ABCDEF", 0x1111111111111111ull, 0x61F9C3802281B096ull},
            {"1111111111111111", 0x0123456789ABCDEFull, 0x7D0CC630AFDA1EC7ull},
            {"0000000000000000", 0x0000000000000000ull, 0x4EF997456198DD78ull},
            {"FEDCBA9876543210", 0x0123456789ABCDEFull, 0x0ACEAB0FC6A0A28Dull},
            {"7CA110454A1A6E57", 0x01A1D6D039776742ull, 0x59C68245EB05282Bull},
            {"0131D9619DC1376E", 0x5CD54CA83DEF57DAull, 0xB1B8CC0B250F09A0ull},
            {"07A1133E4A0B2686", 0x0248D43806F67172ull, 0x1730E5778BEA1DA4ull},
            {"3849674C2602319E", 0x51454B582DDF440Aull, 0xA25E7856CF2651EBull},
            {"04B915BA43FEB5B6", 0x42FD443059577FA2ull, 0x353882B109CE8F1Aull},
            {"0113B970FD34F2CE", 0x059B5E0851CF143Aull, 0x48F4D0884C379918ull},
            {"0170F175468FB5E6", 0x0756D8E0774761D2ull, 0x432193B78951FC98ull},
            {"43297FAD38E373FE", 0x762514B829BF486Aull, 0x13F04154D69D1AE5ull},
            {"07A7137045DA2A16", 0x3BDD119049372802ull, 0x2EEDDA93FFD39C79ull},
            {"04689104C2FD3B2F", 0x26955F6835AF609Aull, 0xD887E0393C2DA6E3ull},
            {"37D06BB516CB7546", 0x164D5E404F275232ull, 0x5F99D04F5B163969ull},
            {"1F08260D1AC2465E", 0x6B056E18759F5CCAull, 0x4A057A3B24D3977Bull},
            {"584023641ABA6176", 0x004BD6EF09176062ull, 0x452031C1E4FADA8Eull},
            {"025816164629B007", 0x480D39006EE762F2ull, 0x7555AE39F59B87BDull},
            {"49793EBC79B3258F", 0x437540C8698F3CFAull, 0x53C55F9CB49FC019ull},
            {"4FB05E1515AB73A7", 0x072D43A077075292ull, 0x7A8E7BFA937E89A3ull},
            {"49E95D6D4CA229BF", 0x02FE55778117F12Aull, 0xCF9C5D7A4986ADB5ull},
            {"018310DC409B26D6", 0x1D9D5C5018F728C2ull, 0xD1ABB290658BC778ull},
            {"1C587F1C13924FEF", 0x305532286D6F295Aull, 0x55CB3774D13EF201ull},
            {"0101010101010101", 0x0123456789ABCDEFull, 0xFA34EC4847B268B2ull},
            {"1F1F1F1F0E0E0E0E", 0x0123456789ABCDEFull, 0xA790795108EA3CAEull},
            {"E0FEE0FEF1FEF1FE", 0x0123456789ABCDEFull, 0xC39E072D9FAC631Dull},
            {"0000000000000000", 0xFFFFFFFFFFFFFFFFull, 0x014933E0CDAFF6E4ull},
            {"FFFFFFFFFFFFFFFF", 0x0000000000000000ull, 0xF21E9A77B71C49BCull},
            {"0123456789ABCDEF", 0x0000000000000000ull, 0x245946885754369Aull},
            {"FEDCBA9876543210", 0xFFFFFFFFFFFFFFFFull, 0x6B5C5A9C5D9E0A5Aull},
        };

        for (const auto& vec : schneier) {
            TBlowfish bf(HexDecode(vec.Key));
            ui64 plaintext = SwapBytes(vec.Plaintext);
            ui64 ciphertext = SwapBytes(vec.Ciphertext);
            UNIT_ASSERT_EQUAL(ciphertext, bf.EcbEncrypt(plaintext));
            UNIT_ASSERT_EQUAL(plaintext, bf.EcbDecrypt(ciphertext));
        }
    }

    void ParseRsaPemPrivateKey() {
        TString testPemKey =
            "-----BEGIN RSA PRIVATE KEY-----\n"
            "MIICXQIBAAKBgQC36vn3cVW0vEkB/xpCtRFWRFyF3iPSg9BJn/SVhnRIKNrQBFg1\n"
            "RwAosOXWRx2YrCYrATRmEMVuQeLkjR+6Q5+Ly2YrK8Ovi85ksQiM2Kw+9J/Yms9E\n"
            "DQT414+Se9R1OpfdO4B18fGKku5YyfrIZWyuBr/qRqAntTDceXnQWWht4QIDAQAB\n"
            "AoGAUefBPmMoqf6X/N2g0khU2jhDhBJznZK6Na+YeuaP7nrTR4RHzCI8feKZ2J1/\n"
            "Hri7nrdAoJujcQDCjMoUcR0gdZzolwiptDiVVwqxKm8MvEyEi+PEmFWNC8/CiU5C\n"
            "v4TzvkgbWP5xf0eugg4LGB4rC8lrv4QyjkAmrgc7oKUjfikCQQDwnszGFXAh42JR\n"
            "E/xmAVFLNPs4uNCceaYWh+jli3254TK7RTnsFFskUyvS/AtTcPUGCnyAhz74MqUA\n"
            "+nX5SDhTAkEAw6xfAAywizfC/jAzDIYqJ8mOdLs17wy9tFxvz+UHOUBP+2tPKe1q\n"
            "J49so8w225u/R2jhyKzpoHFmc2DG+bFqewJBAOXQpgAjBaA4TvTlQ9IhPAW6qp74\n"
            "Vba9sVYfpN9opUJdxlh6u+GxZ4OANIEk3aRqZHvKlDMl+YyQwmv2y6q1waUCQATA\n"
            "D4JZzINktCgllWetbiKPIxU0YkfOYGCbid9bKQS7yfVJkp9q8xPIyJNlZsOIEWmz\n"
            "Yx/TCszpU9pjNBFlvDcCQQCzKV+KaT3OXZx1KVDPLkdHL+6rv+wFjP4TOi885XYZ\n"
            "V7TF7s9yRzV7QBEOqGx3ax58VfUwS6FE8Q7AQ77i7XAw\n"
            "-----END RSA PRIVATE KEY-----";

        TEvpPkeyHolder key = CreateKeyFromPem(testPemKey);
        UNIT_ASSERT(EVP_PKEY_base_id(key.Get()) == EVP_PKEY_RSA);
    }

    void RsaSha256Sign() {
        TString testPemKey =
            "-----BEGIN RSA PRIVATE KEY-----\n"
            "MIIBOgIBAAJBAK1Cs8Y5lhjKTX5KWepxzt1VLOAJJTgZ9CHICR6EVV2jKzjNu2tn\n"
            "r2/seS7ef1pVMJToNzW4lbDYWWtS7p+a+6ECAwEAAQJBAJ/0bLQchg3s9w4Y5loj\n"
            "J6/+6qcKymm4zEJDwueBlK4UaGcBFFJPhGLyzfeDtbA89UT+hN3xycZFt+ZsiMvV\n"
            "9IECIQDedK/V/puguaEXcndEZ8b+XO4jM7fr6hNKhdRzt2KSLQIhAMdi9sCvOfh/\n"
            "KGlxlwDHN7BMRS5kIvQTIp/jV8KZBNvFAiAoyH3AHsiLY9zbvpmNCfWahpEGFSI/\n"
            "9w8IV5bGjDVfFQIgRCO5Cj6YBCmIqQhtv5FVocVe+yyzmVAUzCmIq3NZ6rUCIFxK\n"
            "vO4XLXykbAFzTwHpwWSoJfVv+U7qFJ0W34I8gGAd\n"
            "-----END RSA PRIVATE KEY-----";

        TEvpPkeyHolder key = CreateKeyFromPem(testPemKey);

        TString signedMessage = ::RsaSha256Sign(key.Get(), "some message");

        UNIT_ASSERT_STRINGS_EQUAL(signedMessage,
            HexDecode("7762b15bf726ead0a96d5a390edbedd9989914e491def0de74b113110cda3ac365bcb6563ad6b7328620c75984e16f4531c72e4b3e06940416a26b96681c81cd"));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCryptoTest);
