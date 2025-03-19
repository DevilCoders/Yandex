#pragma once

#include "oio.h"
#include "types.h"

#include <kernel/common_server/auth/jwt/jwk.h>

#include <library/cpp/logger/global/global.h>

#include <contrib/libs/openssl/include/openssl/rsa.h>

namespace NOpenssl {

    TString CalcSHA256(const TString& data);
    TString CalcSHA1(const TString& data);
    TString CalcHMACToken(const TString& secret, const TString& sign);
    TRSAPtr GenerateRSA();
    
    TString BignumEncodeBase64(const BIGNUM* bn);
    bool BignumFromBase64(TString base64, BIGNUM** result);

    class TRSAPublicKey {
        public:
            TRSAPublicKey() = default;
            TRSAPublicKey(TRSAPtr rsa)
                : RSAKey(rsa)
            {}

            bool Init(const TString& key) {
                RSAKey = GetRSAPublicKey(key);
                return !!RSAKey;
            }

            bool Init(const TJwk& jwk);

            bool VerifySHA(const TString& message, const TString& signature) {
                CHECK_WITH_LOG(RSAKey);
                TString shaMessage = CalcSHA256(message);
                return RSA_verify(NID_sha256,
                        (const unsigned char*) shaMessage.data(), shaMessage.size(),
                        (const unsigned char*) signature.data(), signature.size(), RSAKey.Get());
            }

            bool Verify(const TString& message, const TString& signature) {
                CHECK_WITH_LOG(RSAKey);
                return RSA_verify(NID_sha256WithRSAEncryption,
                        (const unsigned char*) message.data(), message.size(),
                        (const unsigned char*) signature.data(), signature.size(), RSAKey.Get());
            }

            bool Encrypt(const TString& plain, TString& encrypt) const;
            bool Decrypt(const TString& encrypted, TString& plain) const;

        private:
            TRSAPtr RSAKey;
    };

    class TRSAPrivateKey {
        public:
            TRSAPrivateKey() = default;
            TRSAPrivateKey(TRSAPtr rsa)
                : RSAKey(rsa)
            {}

            bool Init(const TString& key) {
                RSAKey = GetRSAPrivateKey(key);
                return !!RSAKey;
            }

            bool Sign(const TString& message, TString& signature) {
                CHECK_WITH_LOG(RSAKey);
                unsigned int outlen = 0;
                signature.resize(RSA_size(RSAKey.Get()));
                int result = RSA_sign(NID_sha256WithRSAEncryption,
                        (const unsigned char*) message.data(), message.size(),
                        (unsigned char*) signature.data(), &outlen, RSAKey.Get());
                if (result != 1 || outlen != signature.size()) {
                    return false;
                }
                return true;
            }

            bool SignSHA(const TString& message, TString& signature) {
                CHECK_WITH_LOG(RSAKey);
                unsigned int outlen = 0;
                const TString shaMessage = CalcSHA256(message);
                signature.resize(RSA_size(RSAKey.Get()));
                int result = RSA_sign(NID_sha256,
                        (const unsigned char*) shaMessage.data(), shaMessage.size(),
                        (unsigned char*) signature.data(), &outlen, RSAKey.Get());
                if (result != 1 || outlen != signature.size()) {
                    return false;
                }
                return true;
            }

            bool Encrypt(const TString& plain, TString& encrypt) const;
            bool Decrypt(const TString& encrypted, TString& plain) const;

        private:
            TRSAPtr RSAKey;
    };
}
