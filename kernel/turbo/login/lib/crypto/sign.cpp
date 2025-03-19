#include "sign.h"

#include <contrib/libs/openssl/include/openssl/hmac.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/hex.h>

namespace {
    TString GetHash(const TVector<ui8>& key, TStringBuf data, TStringBuf timestamp) {
        if (key.size() == 0) {
            return TString();
        }

        TString dataToSign;
        dataToSign.reserve(data.size() + 1 + timestamp.size());
        dataToSign.insert(dataToSign.end(), data.begin(), data.end());
        dataToSign.push_back(':');
        dataToSign.insert(dataToSign.end(), timestamp.begin(), timestamp.end());

        const EVP_MD* hashAlgorithm = ::EVP_sha256();
        TString hmac;
        unsigned int hmacSize = ::EVP_MD_size(hashAlgorithm);
        hmac.resize(hmacSize);

        const unsigned char* result = ::HMAC(
            hashAlgorithm,
            key.data(), key.size(),
            reinterpret_cast<const unsigned char*>(dataToSign.data()), dataToSign.size(),
            reinterpret_cast<unsigned char*>(hmac.begin()), &hmacSize
        );
        if (result == nullptr) {
            hmac.clear();
        }
        return hmac;
    }
}

namespace NTurboLogin {

    TString Sign(
        const TVector<ui8>& key,
        TStringBuf data,
        TInstant now
    ) {
        TString timestamp = ToString(now.Seconds());
        TString sign = HexEncode(GetHash(key, data, timestamp));
        sign.to_lower();
        if (sign.Empty()) {
            return TString();
        }

        return TStringBuilder() << sign << ":" << timestamp;

    }

    TString Sign(
        const TKeyProvider* keyProvider,
        TStringBuf data,
        TInstant now
    ) {
        if (keyProvider == nullptr) {
            return TString();
        }
        return Sign(keyProvider->GetSignatureKey(), data, now);
    }

    bool ValidateSign(
        const TVector<ui8>& key,
        TStringBuf data,
        TStringBuf sign,
        TInstant now,
        TDuration maxAge
    ) {
        try {
            if (key.empty()) {
                return false;
            }

            TStringBuf signature, timestamp;
            if (!sign.TrySplit(':', signature, timestamp)) {
                return false;
            }

            ui64 signTimestamp = 0;
            ui64 minAllowedTimestamp = (now - maxAge).Seconds();
            if ((!TryIntFromString<10>(timestamp, signTimestamp)) || (signTimestamp < minAllowedTimestamp)) {
                return false;
            }

            if (signature.size() % 2 != 0) {
                return false;
            }
            return GetHash(key, data, timestamp) == HexDecode(signature);
        }
        catch (yexception& /*exp*/) {
            return false;
        }
    }

    bool ValidateSign(
        const TKeyProvider* keyProvider,
        TStringBuf data,
        TStringBuf sign,
        TInstant now,
        TDuration maxAge
    ) {
        if (keyProvider == nullptr) {
            return false;
        }
        return ValidateSign(keyProvider->GetSignatureKey(), data, sign, now, maxAge);
    }
}
