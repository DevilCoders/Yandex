#pragma once

#include <kernel/ugc/security/proto/crypto_bundle.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <contrib/libs/openssl/include/openssl/blowfish.h>
#include <contrib/libs/openssl/include/openssl/evp.h>

namespace NUgc {
    namespace NSecurity {
        namespace NInternal {

            struct TEvpPkeyDestroyer {
                static void Destroy(EVP_PKEY* b) {
                    EVP_PKEY_free(b);
                }
            };

            using TEvpPkeyHolder = THolder<EVP_PKEY, TEvpPkeyDestroyer>;
            using TEvpPkeyPtr = TAtomicSharedPtr<EVP_PKEY, TEvpPkeyDestroyer>;

            const int GCM_KEY_LEN = 32;
            const int GCM_TAG_LEN = 16;
            const int GCM_NONCE_LEN = 12;
            TEvpPkeyHolder CreateKeyFromPem(TStringBuf pemKey);
            // Computes HMAC-SHA512.
            TString Hmac(TStringBuf key, TStringBuf message);
            // Computes sign using RSA-SHA256 digest. 'privateKey' must be RSA private key
            TString RsaSha256Sign(EVP_PKEY* privateKey, TStringBuf message);
            // Encrypts Plaintext into Ciphertext, clears Plaintext, authenticates Aad, sets Tag.
            void Aes256GcmEncrypt(TStringBuf key, TCryptoBundle& cryptoBundle);
            // Validates Tag and returns false on failure. Decrypts Ciphertext into Plaintext, clears Ciphertext.
            bool Aes256GcmDecrypt(TStringBuf key, TCryptoBundle& cryptoBundle);

            class TBlowfish {
            public:
                explicit TBlowfish(const TStringBuf key);
                // Yes, ECB. Needed for encryption of passport ids. Don't use for longer streams.
                // That's also the reason for Blowfish: need a 64-bit blocksize.
                ui64 EcbEncrypt(const ui64 value) const;
                ui64 EcbDecrypt(const ui64 value) const;

            private:
                BF_KEY Key;
            };
        } // namespace NInternal
    } // namespace NSecurity
} // namespace NUgc
