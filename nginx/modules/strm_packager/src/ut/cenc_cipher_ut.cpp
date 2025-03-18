#include <nginx/modules/strm_packager/src/common/cenc_cipher.h>
#include <nginx/modules/strm_packager/src/common/evp_cipher.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>

using namespace NStrm::NPackager;

Y_UNIT_TEST_SUITE(CencCipher) {
    const TStringBuf key = "keykeykeykeykeykeykeykeykeykeykey";

    TVector<ui8> PrepareData(size_t len) {
        TVector<ui8> result(len);
        for (size_t i = 0; i < len; ++i) {
            result[i] = i * (23123 + i) + i + 166;
        }
        return result;
    }

    Y_UNIT_TEST(TestGetIV) {
        TVector<ui8> data = PrepareData(1024 * 1024);

        TCencCipherAes128Ctr cencctr;
        TCencCipherAes128Cbc cenccbc;

        UNIT_ASSERT_VALUES_EQUAL(16, cenccbc.KeyLength());
        UNIT_ASSERT_VALUES_EQUAL(16, cenccbc.IVLength());
        UNIT_ASSERT_VALUES_EQUAL(16, cenccbc.BlockSize());

        UNIT_ASSERT_VALUES_EQUAL(16, cencctr.KeyLength());
        UNIT_ASSERT_VALUES_EQUAL(8, cencctr.IVLength());
        UNIT_ASSERT_VALUES_EQUAL(1, cencctr.BlockSize());

        TCencCipher* const cencCiphers[2] = {&cencctr, &cenccbc};
        TEvpCipher evpCiphers[2] = {TEvpCipher(EVP_aes_128_ctr()), TEvpCipher(EVP_aes_128_cbc())};

        for (int ci = 0; ci < 2; ++ci) {
            TEvpCipher& evpCipher = evpCiphers[ci];
            TCencCipher& cencCipher = *cencCiphers[ci];

            const ui8 startIV[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

            cencCipher.EncryptInit((ui8 const*)key.data(), startIV);

            ui8 const* dataPtr = data.data();

            for (size_t i = 1;; ++i, cencCipher.NextSample()) {
                ui8 iv[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                cencCipher.GetIV(iv);

                const size_t encSize = i * cencCipher.BlockSize();

                if (dataPtr - data.data() + encSize > data.size()) {
                    break;
                }

                TString enc1(encSize, '\0');
                TString enc2(encSize, '\0');

                cencCipher.Encrypt((ui8*)enc1.data(), dataPtr, encSize);

                evpCipher.EncryptInit((ui8 const*)key.data(), iv, /*padding = */ false);

                UNIT_ASSERT_VALUES_EQUAL(encSize, evpCipher.EncryptUpdate((ui8*)enc2.data(), dataPtr, encSize));

                // UNIT_ASSERT_VALUES_EQUAL(TStringBuf((char const*)enc1, encSize), TStringBuf((char const*)enc2, encSize));
                UNIT_ASSERT_VALUES_EQUAL(enc1, enc2);

                dataPtr += encSize;
            }
        }
    } // Y_UNIT_TEST(TestGetIV)
} // Y_UNIT_TEST_SUITE(CencCipher)
