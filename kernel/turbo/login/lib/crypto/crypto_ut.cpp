#include "crypto.h"
#include "crypto_arc.h"
#include "key_gen.h"
#include "key_provider.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/datetime/base.h>
#include <util/stream/file.h>
#include <util/generic/string.h>

using namespace NTurboLogin;


const std::string Yuid = "12345678901234567890";
const TString YuidArc = "12345678901234567890";
const ui64 FingerPrint = 995298059012907471LL;

Y_UNIT_TEST_SUITE(TTurboHeaderTest) {

    Y_UNIT_TEST(Decrypt) {
        TKeyProvider keyProvider(GenerateSecret(1));
        std::string text;
        for (int i = 0; i < 150; ++i) {
            std::string encrypted;
            UNIT_ASSERT(EncryptCryptoBox(&keyProvider, 0, text, encrypted));
            std::string decrypted;
            UNIT_ASSERT(DecryptCryptoBox(&keyProvider, encrypted, decrypted));
            UNIT_ASSERT_EQUAL(text, decrypted);
            text.push_back('a');
        }
    }

    Y_UNIT_TEST(BrokenHmac) {
        TKeyProvider keyProvider(GenerateSecret(1));
        std::string text;
        text.resize(32, 'a');
        std::string encrypted;
        UNIT_ASSERT(EncryptCryptoBox(&keyProvider, 0, text, encrypted));
        TString ctext = Base64Decode(TString(encrypted));
        TString broken;
        broken.push_back(ctext[0] + 1);
        broken.append(ctext.substr(1));

        TString brokenEncrypted = Base64Encode(broken);
        std::string brokenStr(brokenEncrypted.data(), brokenEncrypted.size());
        std::string decrypted;
        UNIT_ASSERT(!DecryptCryptoBox(&keyProvider, brokenStr, decrypted));
    }

    Y_UNIT_TEST(DecryptYuid) {
        std::string encrypted0;
        std::string encrypted1;
        std::string encrypted2;
        TKeyProvider keyProvider(GenerateSecret(2));
        UNIT_ASSERT(EncryptYuid(&keyProvider, 0, Yuid, "domain.ru", encrypted0));
        UNIT_ASSERT(EncryptYuid(&keyProvider, 1, Yuid, "domain.ru", encrypted1));
        UNIT_ASSERT(!EncryptYuid(&keyProvider, 2, Yuid, "domain.ru", encrypted2));

        std::string decryptedYuid;
        std::string decryptedDomain;
        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted0, decryptedYuid, decryptedDomain));
        UNIT_ASSERT_STRINGS_EQUAL(decryptedYuid, Yuid);
        UNIT_ASSERT_STRINGS_EQUAL(decryptedDomain, "domain.ru");

        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted1, decryptedYuid, decryptedDomain));
        UNIT_ASSERT_STRINGS_EQUAL(decryptedYuid, Yuid);
        UNIT_ASSERT_STRINGS_EQUAL(decryptedDomain, "domain.ru");
    }

    Y_UNIT_TEST(DecryptYuidArc) {
        TString encrypted0;
        TString encrypted1;
        TString encrypted2;
        TString encrypted3;
        TKeyProvider keyProvider(GenerateSecret(2));
        UNIT_ASSERT(EncryptYuid(&keyProvider, 0, YuidArc, "domain.ru", encrypted0));
        UNIT_ASSERT(EncryptYuid(&keyProvider, 1, YuidArc, "domain.ru", encrypted1));
        UNIT_ASSERT(!EncryptYuid(&keyProvider, 2, YuidArc, "domain.ru", encrypted2));

        const TInstant& timestamp = Now();
        TPlainText plaintext;
        plaintext.SetTimestamp(timestamp.Seconds());
        plaintext.SetUid(YuidArc);
        plaintext.SetDomain("domain.ru");
        UNIT_ASSERT(EncryptYuid(&keyProvider, 0, plaintext, encrypted3));

        TString decryptedYuid;
        TString decryptedDomain;
        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted0, decryptedYuid, decryptedDomain));
        UNIT_ASSERT_STRINGS_EQUAL(decryptedYuid, YuidArc);
        UNIT_ASSERT_STRINGS_EQUAL(decryptedDomain, "domain.ru");

        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted1, decryptedYuid, decryptedDomain));
        UNIT_ASSERT_STRINGS_EQUAL(decryptedYuid, YuidArc);
        UNIT_ASSERT_STRINGS_EQUAL(decryptedDomain, "domain.ru");

        TPlainText decrypted;
        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted3, decrypted));
        UNIT_ASSERT_STRINGS_EQUAL(decrypted.GetUid(), YuidArc);
        UNIT_ASSERT_STRINGS_EQUAL(decrypted.GetDomain(), "domain.ru");
        UNIT_ASSERT_EQUAL(decrypted.GetTimestamp(), timestamp.Seconds());
    }

    Y_UNIT_TEST(CreateKeyProvider) {
        TKeyProvider* keyProvider = CreateKeyProvider(GenerateSecret(1));
        std::string encrypted;
        UNIT_ASSERT(EncryptYuid(keyProvider, 0, Yuid, "domain.ru", encrypted));
        std::string decryptedYuid;
        std::string decryptedDomain;
        UNIT_ASSERT(DecryptYuid(keyProvider, encrypted, decryptedYuid, decryptedDomain));
        UNIT_ASSERT_STRINGS_EQUAL(decryptedYuid, Yuid);
        UNIT_ASSERT_STRINGS_EQUAL(decryptedDomain, "domain.ru");
        DestroyKeyProvider(keyProvider);
    }

    Y_UNIT_TEST(BrokenMessage) {
        std::string decryptedYuid;
        std::string decryptedDomain;
        TKeyProvider keyProvider(GenerateSecret(1));
        std::string validMessage;
        UNIT_ASSERT(EncryptYuid(&keyProvider, 0, Yuid, "domain.ru", validMessage));
        UNIT_ASSERT(DecryptYuid(&keyProvider, validMessage, decryptedYuid, decryptedDomain));

        for (size_t i = 1; i <= validMessage.length(); ++i) {
            std::string brokenMessage = validMessage.substr(0, validMessage.length() - i);
            UNIT_ASSERT(!DecryptYuid(&keyProvider, brokenMessage, decryptedYuid, decryptedDomain));
        }
    }

    Y_UNIT_TEST(BrokenOpenText) {
        TString text = TStringBuilder() << char(0) << char(0) << "tttt12345,ab.com,qq";
        TString key;
        TKeyProvider keyProvider(GenerateSecret(1));
        TString encrypted;
        EncryptCryptoBox(&keyProvider, 0, text, encrypted);
        TPlainText plainText;
        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted, plainText));
        UNIT_ASSERT(!plainText.HasFingerPrint());
    }

    Y_UNIT_TEST(BrokenSecret) {
        std::string brokenSecret1 = "{,";
        std::string brokenSecret2 = "{}";
        std::string brokenSecret3 = R"({"keys":["cJL0"]})";
        UNIT_CHECK_GENERATED_EXCEPTION(TKeyProvider(brokenSecret1), yexception);
        UNIT_ASSERT(CreateKeyProvider(brokenSecret1) == nullptr);
        UNIT_CHECK_GENERATED_EXCEPTION(TKeyProvider(brokenSecret2), yexception);
        UNIT_ASSERT(CreateKeyProvider(brokenSecret2) == nullptr);
        UNIT_CHECK_GENERATED_EXCEPTION(TKeyProvider(brokenSecret3), yexception);
        UNIT_ASSERT(CreateKeyProvider(brokenSecret3) == nullptr);
    }

    Y_UNIT_TEST(SecretOffset) {
        TString secret = GenerateSecret(2, 5);  // two keys with offset 5
        NJson::TJsonValue newSecretValue;
        NJson::ReadJsonTree(secret, &newSecretValue);
        NJson::TJsonValue newKeys;
        newKeys.AppendValue(newSecretValue["keys"][1]);
        newSecretValue["start_offset"] = 6;
        newSecretValue["keys"] = newKeys;
        TString newSecret = NJson::WriteJson(newSecretValue); // one secret with offset 6

        std::string encrypted0;
        std::string encrypted1;
        TKeyProvider keyProvider(secret);
        TKeyProvider newKeyProvider(newSecret);
        UNIT_ASSERT(EncryptYuid(&keyProvider, 5, Yuid, "domain.ru", encrypted0));
        UNIT_ASSERT(EncryptYuid(&keyProvider, 6, Yuid, "domain.ru", encrypted1));

        std::string decryptedYuid;
        std::string decryptedDomain;
        UNIT_ASSERT(!DecryptYuid(&newKeyProvider, encrypted0, decryptedYuid, decryptedDomain));
        UNIT_ASSERT(DecryptYuid(&newKeyProvider, encrypted1, decryptedYuid, decryptedDomain));
        UNIT_ASSERT_STRINGS_EQUAL(decryptedYuid, Yuid);
        UNIT_ASSERT_STRINGS_EQUAL(decryptedDomain, "domain.ru");
    }

    Y_UNIT_TEST(GetICookie) {
        TKeyProvider keyProvider(GenerateSecret(2));
        TString icookie1 = "12345";
        TString icookie2 = "56789";
        TString encrypted1;
        TString encrypted2;
        UNIT_ASSERT(EncryptYuid(&keyProvider, 0, icookie1, "domain.ru", encrypted1));
        UNIT_ASSERT(EncryptYuid(&keyProvider, 0, icookie2, "domain.ru", encrypted2));

        TMaybe<TString> result;
        result = GetICookie(TMaybe<TStringBuf>(), "", keyProvider);
        UNIT_ASSERT(!result.Defined());

        result = GetICookie(encrypted1, "", keyProvider);
        UNIT_ASSERT_STRINGS_EQUAL(result.GetOrElse(""), icookie1);

        result = GetICookie(TMaybe<TStringBuf>(), "/turbo/s/rg.ru/abc?turbo_ic=" + CGIEscapeRet(encrypted2), keyProvider);
        UNIT_ASSERT_STRINGS_EQUAL(result.GetOrElse(""), icookie2);

        result = GetICookie(TMaybe<TStringBuf>(), "https:rg-ru.turbopages.org/turbo/s/rg.ru/abc?turbo_ic=" + CGIEscapeRet(encrypted2) + "#abc", keyProvider);
        UNIT_ASSERT_STRINGS_EQUAL(result.GetOrElse(""), icookie2);

        result = GetICookie(encrypted1, "/turbo/s/rg.ru/abc?turbo_ic=" + CGIEscapeRet(encrypted2), keyProvider);
        UNIT_ASSERT_STRINGS_EQUAL(result.GetOrElse(""), icookie2);
    }

    Y_UNIT_TEST(EncryptV1) {
        TKeyProvider keyProvider(GenerateSecret(2));

        TInstant timestamp = Now();
        TPlainText plaintext;
        plaintext.SetTimestamp(timestamp.Seconds());
        plaintext.SetUid(YuidArc);
        plaintext.SetDomain("domain.ru");
        plaintext.SetFingerPrint(FingerPrint);

        TString encrypted0;
        UNIT_ASSERT(EncryptYuidV1(&keyProvider, 0, plaintext, encrypted0));

        TString decryptedYuid;
        TString decryptedDomain;
        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted0, decryptedYuid, decryptedDomain));
        UNIT_ASSERT_STRINGS_EQUAL(decryptedYuid, YuidArc);
        UNIT_ASSERT_STRINGS_EQUAL(decryptedDomain, "domain.ru");

        TPlainText decrypted;
        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted0, decrypted));
        UNIT_ASSERT_STRINGS_EQUAL(decrypted.GetUid(), YuidArc);
        UNIT_ASSERT_STRINGS_EQUAL(decrypted.GetDomain(), "domain.ru");
        UNIT_ASSERT_EQUAL(decrypted.GetFingerPrint(), FingerPrint);
    }

    Y_UNIT_TEST(FingerPrintV0) {
        TKeyProvider keyProvider(GenerateSecret(2));

        TInstant timestamp = Now();
        TPlainText plaintext;
        plaintext.SetTimestamp(timestamp.Seconds());
        plaintext.SetUid(YuidArc);
        plaintext.SetDomain("domain.ru");
        plaintext.SetFingerPrint(FingerPrint);

        TString encrypted0;
        UNIT_ASSERT(EncryptYuid(&keyProvider, 0, plaintext, encrypted0));

        TPlainText decrypted;
        UNIT_ASSERT(DecryptYuid(&keyProvider, encrypted0, decrypted));
        UNIT_ASSERT_STRINGS_EQUAL(decrypted.GetUid(), YuidArc);
        UNIT_ASSERT_STRINGS_EQUAL(decrypted.GetDomain(), "domain.ru");
        UNIT_ASSERT_EQUAL(decrypted.GetFingerPrint(), FingerPrint);
    }
}
