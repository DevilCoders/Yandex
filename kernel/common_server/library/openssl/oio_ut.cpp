#include "oio.h"

#include <library/cpp/openssl/init/init.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
namespace {
    const TString CERT_PATH = SRC_("ut/CA.pem");
    const TString PK_PATH = SRC_("ut/PK.key");
}

Y_UNIT_TEST_SUITE(OpenSSLTestSuite) {
    Y_UNIT_TEST(PEM_PKCS7) {
        InitOpenSSL();
        TString data = "42";
        auto certificate = NOpenssl::GetCertificateFromFile(CERT_PATH);
        UNIT_ASSERT(certificate);

        auto key = NOpenssl::GetPrivateKeyFromFile(PK_PATH);
        UNIT_ASSERT(key);

        auto package = NOpenssl::FormSignedPackage(data, certificate, key);
        UNIT_ASSERT(package);

        TString pkcs7 = NOpenssl::PrintPEM(package, data);
        UNIT_ASSERT(pkcs7);

        auto package2 = NOpenssl::ReadSignedData(pkcs7);
        UNIT_ASSERT(package2);

        TStringBuf data2 = NOpenssl::GetData(package2);
        UNIT_ASSERT_VALUES_EQUAL(data, data2);
    }

    Y_UNIT_TEST(PEM_CMS) {
        InitOpenSSL();
        TString data = "42";

        auto certificate = NOpenssl::GetCertificateFromFile(CERT_PATH);
        UNIT_ASSERT(certificate);

        auto key = NOpenssl::GetPrivateKeyFromFile(PK_PATH);
        UNIT_ASSERT(key);

        auto package = NOpenssl::FormSignedCMS(data, certificate, key);
        UNIT_ASSERT(package);

        TString pkcs7 = NOpenssl::PrintPEM(package, data);
        UNIT_ASSERT(pkcs7);

        auto package2 = NOpenssl::ReadSignedCMS(pkcs7);
        UNIT_ASSERT(package2);

        TStringBuf data2 = NOpenssl::GetData(package2);
        UNIT_ASSERT_VALUES_EQUAL(data, data2);
    }
}
