#include "rsa.h"

#include <contrib/libs/openssl/include/openssl/bn.h>
#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/hmac.h>
#include <contrib/libs/openssl/include/openssl/rand.h>

#include <library/cpp/openssl/holders/evp.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

namespace NOpenssl {

    TString CalcSHA256(const TString& data) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256((const unsigned char*)data.data(), data.size(), digest);
        TStringBuf result((const char*)digest, SHA256_DIGEST_LENGTH);

        return TString(result);
    }

    TString CalcSHA1(const TString& data) {
        SHA_CTX ctx;
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, reinterpret_cast<const unsigned char*>(data.data()), data.size());

        unsigned char sha1[SHA_DIGEST_LENGTH];
        SHA1_Final(sha1, &ctx);
        TStringBuf result((const char*)sha1, SHA_DIGEST_LENGTH);
        return TString(result);
    }

    TString CalcHMACToken(const TString& secret, const TString& sign) {
        unsigned char hmacBuffer[EVP_MAX_MD_SIZE];
        unsigned int hmacLength;
        HMAC(EVP_sha1(), secret.data(), secret.size(), (const unsigned char*)sign.data(), sign.size(), hmacBuffer, &hmacLength);
        return Base64Encode(TStringBuf((const char*)hmacBuffer, hmacLength));
    }

    TRSAPtr GenerateRSA() {
        TRSAPtr rsa(RSA_new());
        BIGNUM *e = BN_new();
        BN_set_word(e, 7);
        RSA_generate_key_ex(rsa.Get(), 2048, e, nullptr);
        BN_free(e);
        return rsa;
    }

    TString BignumEncodeBase64(const BIGNUM* bn) {
        TString binData;
        int bits = BN_num_bits(bn);
        size_t bin_size = (bits + 7) / 8;
        binData.ReserveAndResize(bin_size);
        BN_bn2bin(bn, (unsigned char*)binData.begin());
        return Base64EncodeUrl(binData);
    }

    bool BignumFromBase64(TString base64, BIGNUM** result) {
        if (!base64) {
            return false;
        }

        const TString bin = Base64DecodeUneven(base64);
        *result = BN_bin2bn((const unsigned char*)bin.data(), (int)(bin.size()), nullptr);
        return *result;
    }

    bool TRSAPublicKey::Init(const TJwk& jwk) {
        if (jwk.GetKTy() != "RSA") {
            return false;
        }
        BIGNUM* tmp_e = nullptr;
        if (!BignumFromBase64(jwk.GetE(), &tmp_e)) {
            return false;
        }
        BIGNUM* tmp_n = nullptr;
        if (!BignumFromBase64(jwk.GetN(), &tmp_n)) {
            return false;
        }

        TRSAPtr rsa(RSA_new());
        if (!RSA_set0_key(rsa.Get(), tmp_n, tmp_e, nullptr)) {
            return false;
        }
        RSAKey = rsa;
        return true;
    }

    bool TRSAPublicKey::Encrypt(const TString& plain, TString& encrypt) const {
        CHECK_WITH_LOG(RSAKey);
        encrypt.resize(RSA_size(RSAKey.Get()));
        const int res = RSA_public_encrypt(plain.size(), (const unsigned char*)plain.Data(), (unsigned char*)encrypt.Data(), RSAKey.Get(), RSA_PKCS1_OAEP_PADDING);
        if (res <= 0) {
            return false;
        }
        encrypt.resize(res);
        return true;
    }
    bool TRSAPublicKey::Decrypt(const TString& encrypted, TString& plain) const {
        CHECK_WITH_LOG(RSAKey);
        plain.resize(RSA_size(RSAKey.Get()));
        const int res = RSA_public_decrypt(encrypted.size(), (const unsigned char*)encrypted.Data(), (unsigned char*)plain.Data(), RSAKey.Get(), RSA_PKCS1_OAEP_PADDING);
        if (res <= 0) {
            return false;
        }
        plain.resize(res);
        return true;
    }
    bool TRSAPrivateKey::Encrypt(const TString& plain, TString& encrypt) const {
        CHECK_WITH_LOG(RSAKey);
        encrypt.resize(RSA_size(RSAKey.Get()));
        const int res = RSA_private_encrypt(plain.size(), (const unsigned char*)plain.Data(), (unsigned char*)encrypt.Data(), RSAKey.Get(), RSA_PKCS1_OAEP_PADDING);
        if (res <= 0) {
            return false;
        }
        encrypt.resize(res);
        return true;
    }
    bool TRSAPrivateKey::Decrypt(const TString& encrypted, TString& plain) const {
        CHECK_WITH_LOG(RSAKey);
        plain.resize(RSA_size(RSAKey.Get()));
        const int res = RSA_private_decrypt(encrypted.size(), (const unsigned char*)encrypted.Data(), (unsigned char*)plain.Data(), RSAKey.Get(), RSA_PKCS1_OAEP_PADDING);
        if (res <= 0) {
            return false;
        }
        plain.resize(res);
        return true;
    }
}
