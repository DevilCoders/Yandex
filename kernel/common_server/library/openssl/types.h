#pragma once

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/cms.h>
#include <openssl/ossl_typ.h>
#include <openssl/pkcs7.h>

#include <util/generic/ptr.h>

namespace NOpenssl {
    struct TBIODestroyer {
        static void Destroy(BIO* p) noexcept;
    };
    using TBIOPtr = TAtomicSharedPtr<BIO, TBIODestroyer>;

    struct TCMS_ContentInfoDestroyer {
        static void Destroy(CMS_ContentInfo* p) noexcept;
    };
    using TCMSPtr = TAtomicSharedPtr<CMS_ContentInfo, TCMS_ContentInfoDestroyer>;

    struct TX509Destroyer {
        static void Destroy(X509* p) noexcept;
    };
    using TX509Ptr = TAtomicSharedPtr<X509, TX509Destroyer>;

    struct TEVP_PKEYDestroyer {
        static void Destroy(EVP_PKEY* p) noexcept;
    };
    using TEVP_PKEYPtr = TAtomicSharedPtr<EVP_PKEY, TEVP_PKEYDestroyer>;

    struct TPKCS7Destroyer {
        static void Destroy(PKCS7* p) noexcept;
    };
    using TPKCS7Ptr = TAtomicSharedPtr<PKCS7, TPKCS7Destroyer>;

    struct TRSADestroyer {
        static void Destroy(RSA* p) noexcept;
    };
    using TRSAPtr = TAtomicSharedPtr<RSA, TRSADestroyer>;
}
