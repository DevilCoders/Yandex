#pragma once

#include <stdexcept>
#include <openssl/err.h>
#include <util/generic/yexception.h>

#define SSL_ENSURE(CONDITION)                   \
    do {                                        \
        if (!(CONDITION)) {                     \
            throw NTokenAgent::ssl_exception(); \
        }                                       \
    } while (false)

namespace NTokenAgent {
    using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
    using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
    using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free_all)>;
    using EVP_MD_ptr = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;
    using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;

    class ssl_exception: public std::exception {
        unsigned long code;

    public:
        ssl_exception()
            : code(::ERR_get_error())
        {
        }

        [[nodiscard]] int lib() const _NOEXCEPT {
            return ERR_GET_LIB(code);
        }

        [[nodiscard]] int func() const _NOEXCEPT {
            return ERR_GET_FUNC(code);
        }

        [[nodiscard]] int reason() const _NOEXCEPT {
            return ERR_GET_REASON(code);
        }

        [[nodiscard]] const char* what() const _NOEXCEPT override {
            return ::ERR_error_string(code, nullptr);
        }
    };

    inline std::string SignatureRs256(const std::shared_ptr<EVP_PKEY>& pkey, std::string digest) {
        EVP_PKEY_ptr ctx(::EVP_PKEY_CTX_new(pkey.get(), nullptr), ::EVP_PKEY_CTX_free);
        SSL_ENSURE(ctx);

        size_t size = ::EVP_PKEY_size(pkey.get());
        SSL_ENSURE(::EVP_PKEY_sign_init(ctx.get()) > 0);
        SSL_ENSURE(::EVP_PKEY_CTX_set_signature_md(ctx.get(), ::EVP_sha256()) > 0);

        std::string res;
        res.resize(size);
        SSL_ENSURE(::EVP_PKEY_sign(ctx.get(), (ui8*)res.data(), &size,
                                   (const ui8*)digest.data(), digest.size()) > 0);
        res.resize(size);
        return res;
    }

    inline std::string SignaturePs256(const std::shared_ptr<EVP_PKEY>& pkey, const std::string& digest) {
        RSA_ptr key(::EVP_PKEY_get1_RSA(pkey.get()), ::RSA_free);
        size_t size = ::RSA_size(key.get());

        std::string padded;
        padded.resize(size);
        SSL_ENSURE(::RSA_padding_add_PKCS1_PSS(key.get(), (ui8*)padded.data(),
                                               (const ui8*)digest.c_str(), ::EVP_sha256(), RSA_PSS_SALTLEN_DIGEST) > 0);

        std::string res;
        res.resize(size);
        SSL_ENSURE(::RSA_private_encrypt(size,
                                         (const ui8*)padded.data(), (ui8*)res.data(), key.get(), RSA_NO_PADDING) > 0);
        return res;
    }

    inline std::shared_ptr<EVP_PKEY> ReadPrivateKeyFromFile(const std::string& path, const std::string& password) {
        BIO_ptr bio(::BIO_new_file(path.c_str(), "r"), ::BIO_free_all);
        SSL_ENSURE(bio);
        std::shared_ptr<EVP_PKEY> key(::PEM_read_bio_PrivateKey(bio.get(),
                                                                nullptr, nullptr, (void*)password.data()),
                                      ::EVP_PKEY_free);
        SSL_ENSURE(key);
        return key;
    }

    inline std::shared_ptr<RSA> ReadPublicKeyFromFile(const std::string& path) {
        BIO_ptr bio(::BIO_new_file(path.c_str(), "r"), ::BIO_free_all);
        SSL_ENSURE(bio);
        std::shared_ptr<RSA> key(::PEM_read_bio_RSAPublicKey(bio.get(),
                                                             nullptr, nullptr, nullptr),
                                 ::RSA_free);
        SSL_ENSURE(key);
        return key;
    }

    inline std::string GetKey(RSA* rsa, std::function<int(BIO*, RSA*)> const& getter) {
        BIO_ptr key_bio(BIO_new(BIO_s_mem()), ::BIO_free_all);
        SSL_ENSURE(key_bio);
        SSL_ENSURE(getter(key_bio.get(), rsa));

        BUF_MEM* res{};
        SSL_ENSURE(BIO_get_mem_ptr(key_bio.get(), &res));

        return {res->data, res->length - 1};
    }

    inline std::string Sha256Hash(const std::string& data) {
        EVP_MD_ptr ctx(::EVP_MD_CTX_new(), ::EVP_MD_CTX_free);
        SSL_ENSURE(::EVP_DigestInit(ctx.get(), ::EVP_sha256()) > 0);
        SSL_ENSURE(::EVP_DigestUpdate(ctx.get(), data.data(), data.size()) > 0);

        ui32 size = ::EVP_MD_CTX_size(ctx.get());
        std::string res;
        res.resize(size);
        SSL_ENSURE(::EVP_DigestFinal(ctx.get(), (ui8*)res.data(), &size) > 0);
        res.resize(size);
        return res;
    }

}
