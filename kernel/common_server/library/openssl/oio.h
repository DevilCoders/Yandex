#pragma once

#include "types.h"

#include <util/generic/array_ref.h>
#include <util/generic/string.h>

#include <functional>

namespace NOpenssl {
    TX509Ptr GetCertificate(TConstArrayRef<char> data);
    TX509Ptr GetCertificateFromFile(const TString& filename);

    using TPasswordCallback = std::function<TString(int /*flags*/)>;
    TEVP_PKEYPtr GetPrivateKey(TConstArrayRef<char> data, TPasswordCallback callback = nullptr);
    TEVP_PKEYPtr GetPrivateKeyFromFile(const TString& filename, TPasswordCallback callback);
    TEVP_PKEYPtr GetPrivateKeyFromFile(const TString& filename, const TString& password = {});

    TRSAPtr GetRSAPublicKey(TConstArrayRef<char> data, TPasswordCallback callback = nullptr);
    TRSAPtr GetRSAPublicKeyFromFile(const TString& filename, TPasswordCallback callback);
    TRSAPtr GetRSAPublicKeyFromFile(const TString& filename, const TString& password = {});

    TRSAPtr GetRSAPrivateKey(TConstArrayRef<char> data, TPasswordCallback callback = nullptr);
    TRSAPtr GetRSAPrivateKeyFromFile(const TString& filename, TPasswordCallback callback);
    TRSAPtr GetRSAPrivateKeyFromFile(const TString& filename, const TString& password = {});

    TPKCS7Ptr FormSignedPackage(TConstArrayRef<char> data, TX509Ptr certificate, TEVP_PKEYPtr key);
    TCMSPtr FormSignedCMS(TConstArrayRef<char> data, TX509Ptr certificate, TEVP_PKEYPtr key);

    TPKCS7Ptr ReadSignedData(TConstArrayRef<char> data);
    TCMSPtr ReadSignedCMS(TConstArrayRef<char> data);

    TStringBuf GetData(TPKCS7Ptr package);
    TStringBuf GetData(TCMSPtr package);
    TStringBuf GetData(const ASN1_OCTET_STRING* data);

    TString PrintPEM(TPKCS7Ptr package, TConstArrayRef<char> data);
    TString PrintPEM(TCMSPtr package, TConstArrayRef<char> data);
}
