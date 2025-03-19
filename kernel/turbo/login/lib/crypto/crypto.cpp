#include "crypto.h"
#include "crypto_arc.h"
#include "key_provider.h"

#include <contrib/libs/libsodium/include/sodium/crypto_secretbox.h>
#include <contrib/libs/libsodium/include/sodium/randombytes.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/url/url.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

#include <string>

namespace {
    const TString TURBO_IC_PARAM = "turbo_ic";
}

namespace NTurboLogin {

    using TKeyId = ui16;
    constexpr size_t KEY_ID_SIZE = 16 / 8;
    static_assert(sizeof(TKeyId) == KEY_ID_SIZE, "binary TKeyId size must equal to 2 bytes");

    using TVersion = ui16;
    constexpr size_t VERSION_SIZE = 16 / 8;
    static_assert(sizeof(TVersion) == VERSION_SIZE, "binary TVersion size must equal to 2 bytes");

    using TTimestamp = ui32;
    static const constexpr size_t TIMESTAMP_SIZE = 32 / 8;
    static_assert(sizeof(TTimestamp) == TIMESTAMP_SIZE, "binary timestamp size must equal to 4 bytes");

    TTimestamp GetCurrentTime() {
        const auto now = Now();
        return now.Seconds();
    }

    template<class TInputStringType, class TOutputStringType>
    bool EncryptCryptoBox(const TKeyProvider* keyProvider, TKeyId keyId, const TInputStringType& plainText, TOutputStringType& result) {
        TKey key;
        if (keyProvider == nullptr || !keyProvider->GetKey(keyId, key)) {
            return false;
        }
        TVector<unsigned char> encrypted;
        encrypted.resize(plainText.size() + crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES + KEY_ID_SIZE);
        unsigned char* keyIdStart = encrypted.data();
        unsigned char* nonceStart = keyIdStart + KEY_ID_SIZE;
        unsigned char* messageStart = nonceStart + crypto_secretbox_NONCEBYTES;

        memcpy(keyIdStart, &keyId, KEY_ID_SIZE);
        randombytes_buf(nonceStart, crypto_secretbox_NONCEBYTES);

        int encryptCode = crypto_secretbox_easy(
            messageStart,
            (unsigned char*)plainText.data(),
            plainText.size(),
            nonceStart,  // nonce
            key.data()
        );
        if (encryptCode != 0) {
            return false;
        }

        result.resize(Base64EncodeBufSize(encrypted.size()));
        size_t resultSize = Base64EncodeUrl(result.begin(), encrypted.data(), encrypted.size()) - result.data();
        result.resize(resultSize);
        return true;
    }

    template<class TInputStringType, class TOutputStringType>
    bool DecryptCryptoBox(const TKeyProvider* keyProvider, const TInputStringType& encryptedB64, TOutputStringType& result) {
        if (encryptedB64.length() % 4 != 0) {
            return false;
        }
        TVector<unsigned char> encrypted;
        encrypted.resize(Base64DecodeBufSize(encryptedB64.size()));
        size_t encryptedSize = Base64Decode(encrypted.data(), encryptedB64.data(), encryptedB64.data() + encryptedB64.size());
        encrypted.resize(encryptedSize);

        if (encryptedSize < crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES + KEY_ID_SIZE) {
            return false;
        }

        unsigned char* keyIdStart = encrypted.data();
        unsigned char* nonceStart = keyIdStart + KEY_ID_SIZE;
        unsigned char* messageStart = nonceStart + crypto_secretbox_NONCEBYTES;
        size_t messageSize = encryptedSize - (crypto_secretbox_NONCEBYTES + KEY_ID_SIZE);
        size_t openTextSize = messageSize - crypto_secretbox_MACBYTES;

        TKeyId keyId;
        memcpy(&keyId, keyIdStart, KEY_ID_SIZE);
        TKey key;
        if (keyProvider == nullptr || !keyProvider->GetKey(keyId, key)) {
            return false;
        }

        result.resize(openTextSize);
        int decryptCode = crypto_secretbox_open_easy(
            (unsigned char*)result.data(),
            messageStart,
            messageSize,
            nonceStart,
            key.data()
        );
        return decryptCode == 0;
    }

    void SerializeYuidV0(const TPlainText& data, TString& message) {
        size_t timestampStart = message.size();

        message.resize(message.size() + TIMESTAMP_SIZE);
        TTimestamp timestamp = data.GetTimestamp();
        memcpy(message.begin() + timestampStart, &timestamp, TIMESTAMP_SIZE);

        message.append(data.GetUid());
        message.push_back(',');
        message.append(data.GetDomain());
        message.push_back(',');
        message.append(ToString(data.GetFingerPrint()));
    }

    void SerializeYuidV1(const TPlainText& data, TString& message) {
        message += data.SerializeAsString();
    }

    template<class TOutputStringType>
    bool EncryptYuid(const TKeyProvider* keyProvider, TKeyId keyId, const TPlainText& data, TOutputStringType& encrypted, TVersion version = 0) {
        if (keyProvider == nullptr) {
            return false;
        }

        TString message;
        message.resize(VERSION_SIZE);
        char* versionStart = message.begin();
        memcpy(versionStart, &version, VERSION_SIZE);

        if (version == 0) {
            SerializeYuidV0(data, message);
        } else if (version == 1) {
            SerializeYuidV1(data, message);
        } else {
            return false;
        }

        if (!EncryptCryptoBox(keyProvider, keyId, message, encrypted)) {
            return false;
        }
        return true;
    }

    bool ParseYuidDataV0(TStringBuf serialized, TPlainText& data) {
        if (serialized.size() < TIMESTAMP_SIZE) {
            return false;
        }
        TTimestamp timestamp;
        memcpy(&timestamp, serialized.data(), TIMESTAMP_SIZE);
        TStringBuf tok = serialized.substr(TIMESTAMP_SIZE);
        data.Clear();
        data.SetTimestamp(timestamp);
        data.SetUid(TString(tok.NextTok(',')));
        data.SetDomain(TString(tok.NextTok(',')));
        TStringBuf fingerPrintStr = tok.NextTok(',');
        ui64 fingerPrint = 0;
        if (fingerPrintStr && TryFromString(fingerPrintStr, fingerPrint)) {
            data.SetFingerPrint(fingerPrint);
        }
        return true;
    }

    bool ParseYuidDataV1(TStringBuf serialized, TPlainText& data) {
        return data.ParseFromArray(serialized.data(), serialized.size());
    }

    template<class TInputStringType>
    bool DecryptYuid(const TKeyProvider* keyProvider, const TInputStringType& encrypted, TPlainText& data) {
        if (keyProvider == nullptr) {
            return false;
        }
        TString message;
        if (!DecryptCryptoBox(keyProvider, encrypted, message)) {
            return false;
        }
        if (message.size() < VERSION_SIZE) {
            return false;
        }
        const char* versionStart = message.data();
        const char* serializedStart = versionStart + VERSION_SIZE;
        size_t serializedSize = message.size() - VERSION_SIZE;

        TStringBuf serialized(serializedStart, serializedSize);

        TVersion messageVersion;
        memcpy(&messageVersion, versionStart, VERSION_SIZE);
        if (messageVersion == 0) {
            return ParseYuidDataV0(serialized, data);
        } else if (messageVersion == 1) {
            return ParseYuidDataV1(serialized, data);
        }
        return false;
    }

    bool EncryptCryptoBox(const TKeyProvider* keyProvider, TKeyId keyId, const std::string& plainText, std::string& result) {
        return EncryptCryptoBox<std::string, std::string>(keyProvider, keyId, plainText, result);
    }

    bool DecryptCryptoBox(const TKeyProvider* keyProvider, const std::string& encryptedB64, std::string& result) {
        return DecryptCryptoBox<std::string, std::string>(keyProvider, encryptedB64, result);
    }

    bool EncryptYuid(const TKeyProvider* keyProvider, TKeyId keyId, const std::string& yuid, const std::string& domain, std::string& encrypted) {
        TPlainText message;
        message.SetUid(TString(yuid.data(), yuid.size()));
        message.SetDomain(TString(domain.data(), domain.size()));
        message.SetTimestamp(GetCurrentTime());
        return EncryptYuid<std::string>(keyProvider, keyId, message, encrypted);
    }

    bool DecryptYuid(const TKeyProvider* keyProvider, const std::string& encrypted, std::string& yuid, std::string& domain) {
        TPlainText parsed;
        if (!DecryptYuid<std::string>(keyProvider, encrypted, parsed)) {
            return false;
        }
        yuid = parsed.GetUid();
        domain = parsed.GetDomain();
        return true;
    }

    bool EncryptYuid(const TKeyProvider* keyProvider, TKeyId keyId, const TPlainText& message, TString& encrypted) {
        if (!message.GetTimestamp()) {
            TPlainText copy = message;
            copy.SetTimestamp(GetCurrentTime());
            return EncryptYuid<TString>(keyProvider, keyId, copy, encrypted);
        }
        return EncryptYuid<TString>(keyProvider, keyId, message, encrypted);
    }

    bool EncryptYuid(const TKeyProvider* keyProvider, TKeyId keyId, const TStringBuf& yuid, const TStringBuf& domain, TString& encrypted) {
        TPlainText message;
        message.SetUid(TString(yuid));
        message.SetDomain(TString(domain));
        message.SetTimestamp(GetCurrentTime());
        return EncryptYuid<TString>(keyProvider, keyId, message, encrypted);
    }

    bool EncryptYuidV1(const TKeyProvider* keyProvider, TKeyId keyId, const TPlainText& message, TString& encrypted) {
        if (!message.GetTimestamp()) {
            TPlainText copy = message;
            copy.SetTimestamp(GetCurrentTime());
            return EncryptYuid<TString>(keyProvider, keyId, copy, encrypted, /*version=*/ 1);
        }
        return EncryptYuid<TString>(keyProvider, keyId, message, encrypted, /*version=*/ 1);
    }

    bool DecryptYuid(const TKeyProvider* keyProvider, const TStringBuf& encrypted, TPlainText& parsed) {
        return DecryptYuid<TStringBuf>(keyProvider, encrypted, parsed);
    }

    bool DecryptYuid(const TKeyProvider* keyProvider, const TStringBuf& encrypted, TString& yuid, TString& domain) {
        TPlainText parsed;
        if (!DecryptYuid<TStringBuf>(keyProvider, encrypted, parsed)) {
            return false;
        }
        yuid = parsed.GetUid();
        domain = parsed.GetDomain();
        return true;
    }

    TKeyProvider* CreateKeyProvider(const std::string& secret) {
        try {
            return new TKeyProvider(secret);
        } catch(yexception) {
            return nullptr;
        }
    }

    void DestroyKeyProvider(TKeyProvider* keyProvider) {
        delete keyProvider;
    }

    TMaybe<TString> GetICookie(const TMaybe<TStringBuf>& cookieIc, const TStringBuf& request, const TKeyProvider& keyProvider) {
        TStringBuf url, query, fragment;
        SeparateUrlFromQueryAndFragment(request, url, query, fragment);
        TCgiParameters params(query);
        TString turboIc;
        auto it = params.find(TURBO_IC_PARAM);
        if (it != params.end()) {
            turboIc = it->second;
        } else if (cookieIc.Defined()) {
            turboIc = cookieIc.GetRef();
        }
        TString icookie;
        TString domain;
        if (DecryptYuid(&keyProvider, turboIc, icookie, domain)) {
            return icookie;
        }
        return TMaybe<TString>();
    }
}
