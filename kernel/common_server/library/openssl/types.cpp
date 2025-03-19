#include "types.h"

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

void NOpenssl::TBIODestroyer::Destroy(BIO* p) noexcept {
    if (p) {
        BIO_free(p);
    }
}

void NOpenssl::TCMS_ContentInfoDestroyer::Destroy(CMS_ContentInfo* p) noexcept {
    if (p) {
        CMS_ContentInfo_free(p);
    }
}

void NOpenssl::TX509Destroyer::Destroy(X509* p) noexcept {
    if (p) {
        X509_free(p);
    }
}

void NOpenssl::TEVP_PKEYDestroyer::Destroy(EVP_PKEY* p) noexcept {
    if (p) {
        EVP_PKEY_free(p);
    }
}

void NOpenssl::TPKCS7Destroyer::Destroy(PKCS7* p) noexcept {
    if (p) {
        PKCS7_free(p);
    }
}

void NOpenssl::TRSADestroyer::Destroy(RSA* p) noexcept {
    if (p) {
        RSA_free(p);
    }
}
