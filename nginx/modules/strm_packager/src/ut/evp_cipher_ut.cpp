#include <nginx/modules/strm_packager/src/common/evp_cipher.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>

using namespace NStrm::NPackager;

Y_UNIT_TEST_SUITE(EvpCipher) {

    const TStringBuf key = "keykeykeykeykeykeykeykeykeykeykey";
    const TStringBuf iv = "iviviviviviviviviviviviviviviviviv";
    const TStringBuf data[2] = {
        "data-2242910e1c4d40a889c324973dad9188265d5cdbd87a4a13b715ef4e39db56f2480495eb9aa54a2183da8a6b8ae836b4a14e7e2757fb487cad1047e7320066bb37bd92e8c3df477db31f76259c66b3f5a1d58b99a3a34546aa91c5911f5",
        "data-2242910e1c4d40a889c324973dad9188265d5cdbd87a4a13b715ef4e39db56f2480495eb9aa54a2183da8a6b8ae836b4a14e7e2757fb487cad1047e7320066bb37bd92e8c3df477db31f76259c66b3f5a1d58b99a3a34546aa91c5911f5be7d0",
    };

    EVP_CIPHER const* const types[] = {
        EVP_enc_null(),
        EVP_aes_128_cbc(),
        EVP_aes_128_cfb(),
        EVP_aes_128_ctr(),
        EVP_aes_128_ecb(),
        EVP_aes_128_ofb(),
        EVP_aes_192_cbc(),
        EVP_aes_192_cfb(),
        EVP_aes_192_ctr(),
        EVP_aes_192_ecb(),
        EVP_aes_192_ofb(),
    };

    Y_UNIT_TEST(TestNullInit) {
        UNIT_ASSERT_EXCEPTION(TEvpCipher(nullptr), yexception);
    } // Y_UNIT_TEST(TestNullInit)

    Y_UNIT_TEST(TestEncryptDecrypt) {
        UNIT_ASSERT_EXCEPTION(TEvpCipher(nullptr), yexception);

        // test encrypt / decrypt
        for (const auto type : types) {
            TEvpCipher cipher(type);
            UNIT_ASSERT_LT(cipher.KeyLength(), key.Size());
            UNIT_ASSERT_LT(cipher.IVLength(), iv.Size());
            UNIT_ASSERT(cipher.BlockSize() > 0);

            UNIT_ASSERT(data[0].size() % cipher.BlockSize() == 0);
            UNIT_ASSERT(data[1].size() % cipher.BlockSize() != 0 || cipher.BlockSize() == 1);

            // test with and without padding two times
            //  on second test skip EncryptFinal/DecryptFinal call in no-padding mode
            for (int counter = 0; counter < 4; ++counter) {
                const int padding = counter % 2;
                const auto& text = data[padding];
                cipher.EncryptInit((ui8 const*)key.Data(), (ui8 const*)iv.Data(), padding);

                TVector<ui8> enc(text.size() + cipher.BlockSize());
                TVector<ui8> dec(text.size() + cipher.BlockSize());

                size_t encSize = 0, finalSize = 0;
                encSize += cipher.EncryptUpdate(enc.data() + encSize, (ui8 const*)text.Data(), 16);
                encSize += cipher.EncryptUpdate(enc.data() + encSize, (ui8 const*)text.Data() + 16, 16);
                encSize += cipher.EncryptUpdate(enc.data() + encSize, (ui8 const*)text.Data() + 32, text.size() - 32);
                if (padding || counter < 2) {
                    encSize += (finalSize = cipher.EncryptFinal(enc.data() + encSize));
                }
                UNIT_ASSERT_LE(encSize, enc.size());
                UNIT_ASSERT_LE(text.size(), encSize);
                if (!padding) {
                    UNIT_ASSERT_VALUES_EQUAL(0, finalSize);
                }

                cipher.DecryptInit((ui8 const*)key.Data(), (ui8 const*)iv.Data(), padding);

                size_t decSize = 0;
                decSize += cipher.DecryptUpdate(dec.data(), enc.data(), encSize);
                if (padding || counter < 2) {
                    decSize += cipher.DecryptFinal(dec.data() + decSize);
                }

                UNIT_ASSERT_VALUES_EQUAL(text, TStringBuf((char const*)dec.data(), decSize));
            }
        }
    } // Y_UNIT_TEST(TestEncryptDecrypt)

    Y_UNIT_TEST(TestEncryptSizePrediction) {
        for (const auto type : types) {
            TEvpCipher cipher(type);

            for (size_t padding = 1; padding < 2; ++padding) {
                const TStringBuf& input = data[1];

                for (size_t inputSize = 0; inputSize <= input.Size(); ++inputSize) {
                    if (!padding && 0 != inputSize % cipher.BlockSize()) {
                        continue;
                    }

                    for (size_t inputBlockSize = 1; inputBlockSize <= inputSize; ++inputBlockSize) {
                        cipher.EncryptInit((ui8 const*)key.Data(), (ui8 const*)iv.Data(), padding);

                        TVector<ui8> enc(input.Size() + cipher.BlockSize());

                        const size_t predictedEncryptedSize = cipher.PredictEncryptedSize(inputSize);
                        size_t actualEncrypedSize = 0;

                        for (size_t i = 0; i < inputSize;) {
                            const size_t thisSize = Min(inputSize - i, inputBlockSize);

                            const size_t predictedUpdateSize = cipher.PredictEncryptUpdate(thisSize);
                            const size_t updateSize = cipher.EncryptUpdate(enc.data() + actualEncrypedSize, (ui8 const*)input.data() + i, thisSize);

                            UNIT_ASSERT_VALUES_EQUAL(updateSize, predictedUpdateSize);
                            actualEncrypedSize += updateSize;

                            i += thisSize;
                        }

                        const size_t predictedEncryptFinalSize = cipher.PredictEncryptFinal();
                        const size_t encryptFinalSize = cipher.EncryptFinal(enc.data() + actualEncrypedSize);

                        UNIT_ASSERT_VALUES_EQUAL(encryptFinalSize, predictedEncryptFinalSize);

                        actualEncrypedSize += encryptFinalSize;

                        UNIT_ASSERT_VALUES_EQUAL(actualEncrypedSize, predictedEncryptedSize);

                        // check decrypt data just to be sure in all this partial encriptions
                        {
                            cipher.DecryptInit((ui8 const*)key.Data(), (ui8 const*)iv.Data(), padding);
                            TVector<ui8> dec(enc.size());

                            size_t decSize = 0;
                            decSize += cipher.DecryptUpdate(dec.data(), enc.data(), actualEncrypedSize);
                            decSize += cipher.DecryptFinal(dec.data() + decSize);

                            UNIT_ASSERT_VALUES_EQUAL(TStringBuf(input.data(), inputSize), TStringBuf((char const*)dec.data(), decSize));
                        }
                    }
                }
            }
        }

    } // Y_UNIT_TEST(TestEncryptSizePrediction)

    Y_UNIT_TEST(TestPadding) {
        TEvpCipher cipher(EVP_aes_128_cbc());

        const TStringBuf& input = data[1];

        for (size_t inputSize = 0; inputSize <= input.size(); ++inputSize) {
            cipher.EncryptInit((ui8 const*)key.Data(), (ui8 const*)iv.Data(), /* padding = */ true);

            TVector<ui8> enc(inputSize + cipher.BlockSize());

            size_t encSize = 0;
            if (inputSize > 0) {
                encSize += cipher.EncryptUpdate(enc.data(), (ui8 const*)input.Data(), inputSize);
            }
            encSize += cipher.EncryptFinal(enc.data() + encSize);

            UNIT_ASSERT_LT(inputSize, encSize);

            cipher.DecryptInit((ui8 const*)key.Data(), (ui8 const*)iv.Data(), /* padding = */ false);

            TVector<ui8> dec(encSize);
            size_t decSize = 0;
            decSize += cipher.DecryptUpdate(dec.data(), enc.data(), encSize);
            decSize += cipher.DecryptFinal(dec.data() + decSize);

            UNIT_ASSERT_VALUES_EQUAL(decSize, encSize);

            TVector<ui8> dec2(input.data(), input.data() + inputSize);
            const ui8 b = encSize - inputSize;
            UNIT_ASSERT(b);
            while (dec2.size() < decSize) {
                dec2.push_back(b);
            }

            UNIT_ASSERT_VALUES_EQUAL(
                TStringBuf((char const*)dec2.data(), dec2.size()),
                TStringBuf((char const*)dec.data(), dec.size()));
        }
    } // Y_UNIT_TEST(TestPadding)

} // Y_UNIT_TEST_SUITE(EvpCipher)
