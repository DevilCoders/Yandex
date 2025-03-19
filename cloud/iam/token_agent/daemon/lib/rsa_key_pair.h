#pragma once

#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <util/generic/yexception.h>
#include "ssl_util.h"

namespace NTokenAgent {
    class TRsaKeyPair {
    public:
        [[nodiscard]] std::string GetPublicKey() const {
            return GetKey(Rsa.get(), PEM_write_bio_RSA_PUBKEY);
        }

        void Write(const std::string& path, const std::string& password) {
            BIO_ptr bio_private(::BIO_new_file(path.c_str(), "w"), ::BIO_free_all);
            SSL_ENSURE(bio_private);
            SSL_ENSURE(PEM_write_bio_RSAPrivateKey(bio_private.get(), Rsa.get(),
                                                   EVP_des_ede3_cbc(), nullptr, 0, nullptr, (void*)password.c_str()));

            BIO_ptr bio_public(::BIO_new_file((path + ".pub").c_str(), "w"), ::BIO_free_all);
            SSL_ENSURE(bio_public);
            SSL_ENSURE(PEM_write_bio_RSAPublicKey(bio_public.get(), Rsa.get()));
        }

        static TRsaKeyPair Create(int bits) {
            return TRsaKeyPair(bits);
        }

    private:
        explicit TRsaKeyPair(int bits)
            : Rsa(RSA_new(), ::RSA_free)
        {
            BN_ptr big_number(BN_new(), ::BN_free);
            SSL_ENSURE(big_number);
            SSL_ENSURE(BN_set_word(big_number.get(), RSA_F4));
            SSL_ENSURE(Rsa);
            SSL_ENSURE(RSA_generate_key_ex(Rsa.get(), bits, big_number.get(), nullptr));
        }

        RSA_ptr Rsa;
    };
}
