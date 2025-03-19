#include "oio.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <util/stream/file.h>

namespace {
    int PasswordCallback(char *buf, int size, int rwflag, void *userdata) {
        const auto& call = *static_cast<NOpenssl::TPasswordCallback*>(userdata);
        TString password = call(rwflag);
        if (!password) {
            return 0;
        }
        if (size < static_cast<int>(password.size())) {
            return -1;
        }

        return password.copy(buf, size, 0);
    }
}

NOpenssl::TX509Ptr NOpenssl::GetCertificate(TConstArrayRef<char> data) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    TX509Ptr x509(PEM_read_bio_X509_AUX(bio.Get(), nullptr, nullptr, nullptr));
    Y_ENSURE(x509, "cannot read X509: " << ERR_GET_REASON(ERR_get_error()));
    return x509;
}

NOpenssl::TX509Ptr NOpenssl::GetCertificateFromFile(const TString& filename) {
    TIFStream input(filename);
    return GetCertificate(input.ReadAll());
}

NOpenssl::TEVP_PKEYPtr NOpenssl::GetPrivateKey(TConstArrayRef<char> data, TPasswordCallback callback) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    TEVP_PKEYPtr key(PEM_read_bio_PrivateKey(bio.Get(), nullptr, PasswordCallback, &callback));
    Y_ENSURE(key, "cannot read PrivateKey: " << ERR_GET_REASON(ERR_get_error()));
    return key;
}

NOpenssl::TEVP_PKEYPtr NOpenssl::GetPrivateKeyFromFile(const TString& filename, TPasswordCallback callback) {
    TIFStream input(filename);
    return GetPrivateKey(input.ReadAll(), callback);
}

NOpenssl::TEVP_PKEYPtr NOpenssl::GetPrivateKeyFromFile(const TString& filename, const TString& password) {
    return GetPrivateKeyFromFile(filename, [&password](int rwflag) {
        Y_UNUSED(rwflag);
        return password;
    });
}

NOpenssl::TRSAPtr NOpenssl::GetRSAPublicKey(TConstArrayRef<char> data, TPasswordCallback callback /*= nullptr*/) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    return PEM_read_bio_RSA_PUBKEY(bio.Get(), nullptr, PasswordCallback, &callback);
}

NOpenssl::TRSAPtr NOpenssl::GetRSAPublicKeyFromFile(const TString& filename, TPasswordCallback callback) {
    TIFStream input(filename);
    return GetRSAPublicKey(input.ReadAll(), callback);
}

NOpenssl::TRSAPtr NOpenssl::GetRSAPublicKeyFromFile(const TString& filename, const TString& password) {
    return GetRSAPublicKeyFromFile(filename, [&password](int rwflag) {
        Y_UNUSED(rwflag);
        return password;
    });
}

NOpenssl::TRSAPtr NOpenssl::GetRSAPrivateKey(TConstArrayRef<char> data, TPasswordCallback callback /*= nullptr*/) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    return PEM_read_bio_RSAPrivateKey(bio.Get(), nullptr, PasswordCallback, &callback);
}

NOpenssl::TRSAPtr NOpenssl::GetRSAPrivateKeyFromFile(const TString& filename, TPasswordCallback callback) {
    TIFStream input(filename);
    return GetRSAPrivateKey(input.ReadAll(), callback);
}

NOpenssl::TRSAPtr NOpenssl::GetRSAPrivateKeyFromFile(const TString& filename, const TString& password) {
    return GetRSAPrivateKeyFromFile(filename, [&password](int rwflag) {
        Y_UNUSED(rwflag);
        return password;
    });
}

NOpenssl::TPKCS7Ptr NOpenssl::FormSignedPackage(TConstArrayRef<char> data, TX509Ptr certificate, TEVP_PKEYPtr key) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    TPKCS7Ptr package(PKCS7_sign(certificate.Get(), key.Get(), nullptr, bio.Get(), 0));
    Y_ENSURE(package, "cannot sign: " << ERR_GET_REASON(ERR_get_error()));
    return package;
}

NOpenssl::TCMSPtr NOpenssl::FormSignedCMS(TConstArrayRef<char> data, TX509Ptr certificate, TEVP_PKEYPtr key) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    TCMSPtr cms(CMS_sign(certificate.Get(), key.Get(), nullptr, bio.Get(), 0));
    Y_ENSURE(cms, "cannot sign: " << ERR_GET_REASON(ERR_get_error()));
    return cms;
}

NOpenssl::TPKCS7Ptr NOpenssl::ReadSignedData(TConstArrayRef<char> data) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    TPKCS7Ptr package(PEM_read_bio_PKCS7(bio.Get(), nullptr, nullptr, nullptr));
    Y_ENSURE(package, "cannot read signed data: " << ERR_GET_REASON(ERR_get_error()));
    return package;
}

NOpenssl::TCMSPtr NOpenssl::ReadSignedCMS(TConstArrayRef<char> data) {
    TBIOPtr bio(BIO_new_mem_buf(data.data(), data.size()));
    TCMSPtr cms(PEM_read_bio_CMS(bio.Get(), nullptr, nullptr, nullptr));
    Y_ENSURE(cms, "cannot read signed data: " << ERR_GET_REASON(ERR_get_error()));
    return cms;
}

TStringBuf NOpenssl::GetData(TPKCS7Ptr package) {
    Y_ENSURE(PKCS7_type_is_signed(package.Get()), "package is not signed");
    const auto content = package->d.sign->contents;
    Y_ENSURE(PKCS7_type_is_data(content));
    return GetData(content->d.data);
}

TStringBuf NOpenssl::GetData(TCMSPtr package) {
    auto p = CMS_get0_content(package.Get());
    Y_ENSURE(p, "package has no data");
    return GetData(*p);
}

TStringBuf NOpenssl::GetData(const ASN1_OCTET_STRING* data) {
    Y_ENSURE(data, "no data is found");
    return{ reinterpret_cast<char*>(data->data), static_cast<size_t>(data->length) };
}

TString NOpenssl::PrintPEM(TPKCS7Ptr package, TConstArrayRef<char> data) {
    TBIOPtr input(BIO_new_mem_buf(data.data(), data.size()));
    TBIOPtr output(BIO_new(BIO_s_mem()));
    Y_ENSURE(PEM_write_bio_PKCS7_stream(output.Get(), package.Get(), input.Get(), 0), "cannot print pkcs7 PEM: " << ERR_GET_REASON(ERR_get_error()));
    char* d = nullptr;
    size_t s = BIO_get_mem_data(output.Get(), &d);
    return { d, s };
}

TString NOpenssl::PrintPEM(TCMSPtr package, TConstArrayRef<char> data) {
    TBIOPtr input(BIO_new_mem_buf(data.data(), data.size()));
    TBIOPtr output(BIO_new(BIO_s_mem()));
    Y_ENSURE(PEM_write_bio_CMS_stream(output.Get(), package.Get(), input.Get(), 0), "cannot print CMS PEM: " << ERR_GET_REASON(ERR_get_error()));
    char* d = nullptr;
    size_t s = BIO_get_mem_data(output.Get(), &d);
    return { d, s };
}
