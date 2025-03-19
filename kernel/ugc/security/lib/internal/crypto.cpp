#include "crypto.h"

#include <kernel/ugc/security/lib/exception.h>

#include <util/generic/buffer.h>

#include <contrib/libs/openssl/include/openssl/err.h>
#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/pem.h>

namespace NUgc {
    namespace NSecurity {
        namespace NInternal {
            namespace {
                typedef unsigned char uchar;

                const uchar* StringToCharbuf(const TString& data) {
                    return reinterpret_cast<const uchar*>(data.data());
                }

                uchar* StringToCharbuf(TString& data) {
                    return reinterpret_cast<uchar*>(&(*data.begin()));
                }

                const uchar* StringbufToCharbuf(TStringBuf data) {
                    return reinterpret_cast<const uchar*>(data.data());
                }

                class TOpenSslLastexception: public TInternalException {
                public:
                    TOpenSslLastexception() {
                        TBuffer buffer(256);
                        ERR_error_string_n(ERR_get_error(), buffer.Data(), buffer.Capacity());
                        *this << "OpenSSL error: [" << TStringBuf(buffer.Data(), buffer.Size())
                              << "]. ";
                    }
                };

                class TMdContext {
                public:
                    TMdContext()
                        : Ctx(EVP_MD_CTX_create())
                    {
                        if (Ctx == nullptr) {
                            ythrow TOpenSslLastexception() << "Failed to create EVP_MD_CTX.";
                        }
                    }

                    ~TMdContext() {
                        if (Ctx != nullptr) {
                            EVP_MD_CTX_destroy(Ctx);
                        }
                    }

                    EVP_MD_CTX* Ctx;
                };

                TEvpPkeyHolder MakeHmacKey(TStringBuf key) {
                    EVP_PKEY* evpKey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, StringbufToCharbuf(key), key.size());
                    if (evpKey == nullptr) {
                        ythrow TOpenSslLastexception() << "Failed to create EVP_PKEY.";
                    }
                    return TEvpPkeyHolder(evpKey);
                }

                class TCipherContext {
                public:
                    TCipherContext()
                        : Ctx(EVP_CIPHER_CTX_new()){
                        if (Ctx == nullptr) {
                            ythrow TOpenSslLastexception() << "Failed to create EVP_CIPHER_CTX.";
                        }
                    }

                    ~TCipherContext() {
                        EVP_CIPHER_CTX_free(Ctx);
                    }

                    EVP_CIPHER_CTX* Ctx;
                };

                struct TBioDestroyer {
                    static void Destroy(BIO* b) {
                        BIO_free(b);
                    }
                };

                void CheckKey(TStringBuf key) {
                    if (key.size() != GCM_KEY_LEN) {
                        ythrow TInternalException() << "Key length is " << key.size()
                            << ", it must be " << GCM_KEY_LEN;
                    }
                }

                void CheckNonce(const TString& nonce) {
                    if (nonce.size() != GCM_NONCE_LEN) {
                        ythrow TInternalException() << "Nonce length is " << nonce.size() <<
                        ", it must be " << GCM_NONCE_LEN;
                    }
                }

                void CheckTag(const TString& tag) {
                    if (tag.size() != GCM_TAG_LEN) {
                        ythrow TInternalException() << "Tag length is " << tag.size() <<
                        ", it must be " << GCM_NONCE_LEN;
                    }
                }

                TString DigestSign(const EVP_MD* md, EVP_PKEY* key, TStringBuf message) {
                    TMdContext mdContext;

                    if (EVP_DigestSignInit(mdContext.Ctx, nullptr, md, nullptr, key) != 1) {
                        ythrow TOpenSslLastexception() << "Failed to initialize signature.";
                    };

                    if (EVP_DigestSignUpdate(mdContext.Ctx, message.data(), message.size()) != 1) {
                        ythrow TOpenSslLastexception() << "Failed to process message.";
                    };

                    size_t actualSize = 0;
                    if (EVP_DigestSignFinal(mdContext.Ctx, nullptr, &actualSize) != 1) {
                        ythrow TOpenSslLastexception() << "Failed to determine actual size.";
                    };
                    TString result(actualSize, '\0');
                    uchar* buffer = StringToCharbuf(result);
                    if (EVP_DigestSignFinal(mdContext.Ctx, buffer, &actualSize) != 1) {
                        ythrow TOpenSslLastexception() << "Failed to produce signature.";
                    };

                    if (actualSize != result.size()) {
                        ythrow TInternalException() << "Unexpected signature size.";
                    }

                    return result;
                }

            } // namespace

            TEvpPkeyHolder CreateKeyFromPem(TStringBuf pemKey) {
                BIO* bio = BIO_new_mem_buf(pemKey.data(), static_cast<int>(pemKey.size()));
                if (bio == nullptr) {
                    ythrow TOpenSslLastexception() << "Failed to create BIO.";
                }
                THolder<BIO, TBioDestroyer> mem(bio);
                EVP_PKEY* key = PEM_read_bio_PrivateKey(mem.Get(), nullptr, nullptr, nullptr);
                if (key == nullptr) {
                    ythrow TOpenSslLastexception() << "Failed to create the key.";
                }
                return TEvpPkeyHolder(key);
            }

            TString Hmac(TStringBuf key, TStringBuf message) {

                const EVP_MD* md = EVP_sha512();
                if (md == nullptr) {
                    ythrow TOpenSslLastexception() << "Failed to instantiate SHA512.";
                }

                TEvpPkeyHolder hmacKey = MakeHmacKey(key);
                return DigestSign(md, hmacKey.Get(), message);
            }

            TString RsaSha256Sign(EVP_PKEY* privateKey, TStringBuf message) {
                Y_ENSURE_EX(EVP_PKEY_base_id(privateKey) == EVP_PKEY_RSA,
                    TOpenSslLastexception() << "'privateKey' is not RSA private key.");
                const EVP_MD* md = EVP_sha256();
                if (md == nullptr) {
                    ythrow TOpenSslLastexception() << "Failed to instantiate SHA512.";
                }
                return DigestSign(md, privateKey, message);
            }

            void Aes256GcmEncrypt(TStringBuf key, TCryptoBundle& cryptoBundle) {
                CheckKey(key);
                CheckNonce(cryptoBundle.GetNonce());

                TCipherContext cipherContext;

                if (EVP_EncryptInit_ex(cipherContext.Ctx, EVP_aes_256_gcm(),
                    nullptr, nullptr, nullptr) != 1) {

                    ythrow TOpenSslLastexception()
                        << "Failed to initialize the encryption operation.";
                }

                if (EVP_CIPHER_CTX_ctrl(
                    cipherContext.Ctx, EVP_CTRL_GCM_SET_IVLEN, GCM_NONCE_LEN, NULL) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to set nonce length.";
                }

                const uchar* keyArray = StringbufToCharbuf(key);
                const uchar* nonceArray = StringToCharbuf(cryptoBundle.GetNonce());

                if (EVP_EncryptInit_ex(cipherContext.Ctx, NULL, NULL, const_cast<uchar*>(keyArray),
                    const_cast<uchar*>(nonceArray)) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to initialize key and nonce.";
                }

                int bytesWritten = 0;
                int aadLen = cryptoBundle.GetAad().size();
                const uchar* aadArray = StringToCharbuf(cryptoBundle.GetAad());

                if (EVP_EncryptUpdate(cipherContext.Ctx, NULL, &bytesWritten,
                    const_cast<uchar*>(aadArray), aadLen) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to add AAD data.";
                }

                int plaintextLen = cryptoBundle.GetPlaintext().size();
                cryptoBundle.MutableCiphertext()->resize(plaintextLen, '\0');
                uchar* ciphertextArray = StringToCharbuf(*cryptoBundle.MutableCiphertext());
                const uchar* plaintextArray = StringToCharbuf(cryptoBundle.GetPlaintext());

                if (EVP_EncryptUpdate(
                    cipherContext.Ctx, ciphertextArray, &bytesWritten,
                    const_cast<uchar*>(plaintextArray), plaintextLen) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to encrypt the message.";
                }

                ciphertextArray += bytesWritten;

                if (EVP_EncryptFinal_ex(cipherContext.Ctx, ciphertextArray, &bytesWritten) != 1) {
                    ythrow TOpenSslLastexception() << "Failed to finalize the encryption.";
                }

                ciphertextArray += bytesWritten;

                cryptoBundle.MutableTag()->resize(GCM_TAG_LEN, '\0');
                uchar* tagArray = const_cast<uchar*>(StringToCharbuf(*cryptoBundle.MutableTag()));
                if (EVP_CIPHER_CTX_ctrl(
                    cipherContext.Ctx, EVP_CTRL_GCM_GET_TAG, GCM_TAG_LEN, tagArray) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to get the tag.";
                }

                cryptoBundle.ClearPlaintext();

                if (ciphertextArray - StringToCharbuf(*cryptoBundle.MutableCiphertext()) !=
                    plaintextLen) {

                    ythrow TOpenSslLastexception()
                        << "Plaintext length is not equal to ciphertext length.";
                }
            }

            bool Aes256GcmDecrypt(TStringBuf key, TCryptoBundle& cryptoBundle) {
                CheckKey(key);
                CheckNonce(cryptoBundle.GetNonce());
                CheckTag(cryptoBundle.GetTag());

                TCipherContext cipherContext;

                if (EVP_DecryptInit_ex(cipherContext.Ctx, EVP_aes_256_gcm(),
                    nullptr, nullptr, nullptr) != 1) {

                    ythrow TOpenSslLastexception()
                        << "Failed to initialize the decryption operation.";
                }

                if (EVP_CIPHER_CTX_ctrl(
                    cipherContext.Ctx, EVP_CTRL_GCM_SET_IVLEN, GCM_NONCE_LEN, NULL) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to set nonce length.";
                }

                const uchar* keyArray = StringbufToCharbuf(key);
                const uchar* nonceArray = StringToCharbuf(cryptoBundle.GetNonce());
                if (EVP_DecryptInit_ex(cipherContext.Ctx, NULL, NULL, const_cast<uchar*>(keyArray),
                    const_cast<uchar*>(nonceArray)) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to initialise key and nonce.";
                }

                int bytesWritten = 0;
                int aadLen = cryptoBundle.GetAad().size();
                const uchar* aadArray = StringToCharbuf(cryptoBundle.GetAad());
                if (EVP_DecryptUpdate(cipherContext.Ctx, NULL, &bytesWritten,
                    const_cast<uchar*>(aadArray), aadLen) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to set AAD data.";
                }

                int ciphertextLen = cryptoBundle.GetCiphertext().size();
                cryptoBundle.MutablePlaintext()->resize(ciphertextLen, '\0');
                uchar* plaintextArray = StringToCharbuf(*cryptoBundle.MutablePlaintext());
                const uchar* ciphertextArray = StringToCharbuf(cryptoBundle.GetCiphertext());

                if (EVP_DecryptUpdate(
                    cipherContext.Ctx, plaintextArray, &bytesWritten,
                    const_cast<uchar*>(ciphertextArray), ciphertextLen) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to decrypt the message.";
                }

                plaintextArray += bytesWritten;

                const uchar* tagArray = StringToCharbuf(cryptoBundle.GetTag());
                if (EVP_CIPHER_CTX_ctrl(
                    cipherContext.Ctx, EVP_CTRL_GCM_SET_TAG, GCM_TAG_LEN,
                    const_cast<uchar*>(tagArray)) != 1) {

                    ythrow TOpenSslLastexception() << "Failed to set the tag.";
                }

                int ret = EVP_DecryptFinal_ex(cipherContext.Ctx, plaintextArray, &bytesWritten);
                plaintextArray += bytesWritten;

                if (ret <= 0) {
                    return false;
                }

                cryptoBundle.ClearCiphertext();
                cryptoBundle.ClearTag();
                if (plaintextArray - StringToCharbuf(*cryptoBundle.MutablePlaintext()) !=
                    ciphertextLen) {

                    ythrow TOpenSslLastexception()
                        << "Plaintext length is not equal to ciphertext length.";
                }

                return true;
            }

            TBlowfish::TBlowfish(const TStringBuf key) {
                Y_ENSURE_EX(key.size() >= 8 && key.size() <= 40,
                    TApplicationException() << "Insane blowfish key size: " << key.size());
                BF_set_key(&Key, key.size(), StringbufToCharbuf(key));
            }

            ui64 TBlowfish::EcbEncrypt(const ui64 value) const {
                ui64 result;
                BF_ecb_encrypt(
                    reinterpret_cast<const uchar*>(&value), reinterpret_cast<uchar*>(&result),
                    &Key, BF_ENCRYPT);
                return result;
            }

            ui64 TBlowfish::EcbDecrypt(const ui64 value) const {
                ui64 result;
                BF_ecb_encrypt(
                    reinterpret_cast<const uchar*>(&value), reinterpret_cast<uchar*>(&result),
                    &Key, BF_DECRYPT);
                return result;
            }
        } // namespace NInternal
    } // namespace NSecurity
} // namespace NUgc
