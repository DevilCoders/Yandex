#include <util/stream/file.h>
#include <util/string/hex.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/openssl/crypto/sha.h>
#include <util/system/byteorder.h>
#include <util/generic/yexception.h>
#include <util/generic/singleton.h>

#include <contrib/libs/taocrypt/include/rsa.hpp>

using TRSAVerifier = TaoCrypt::RSA_Encryptor<TaoCrypt::RSA_BlockType1>;

#include "fuid.h"

static inline TString LoadKey(IInputStream& in) {
    TString encodedKey;
    TString buf;

    while (in.ReadLine(buf)) {
        if (buf != "-----BEGIN PUBLIC KEY-----" && buf != "-----END PUBLIC KEY-----") {
            encodedKey.append(buf);
        }
    }

    return Base64Decode(encodedKey).substr(25);
}

static inline TString LoadKey(const TString& file) {
    TFileInput keyfile(file);

    return LoadKey(keyfile);
}

template <class T>
static inline bool Read(TStringBuf& s, T& t) {
    const size_t len = sizeof(t) * 2;

    if (len > s.size()) {
        return false;
    }

    if (HexDecode(s.data(), len, &t) != &t + 1) {
        return false;
    }

    s.Skip(len);

    return true;
}

class TFuidChecker::TImpl {
public:
    inline TImpl(const TString& key) {
        TaoCrypt::Integer mod;
        mod.Decode((unsigned char*)key.data(), 96);
        TaoCrypt::Integer exp(65537);
        pubKey.Initialize(mod, exp);
        RSAVerifier.Reset(new TRSAVerifier(pubKey));
    }

    inline bool Parse(const TStringBuf& uid, TFuid& fuid) const {
        TStringBuf data;
        TStringBuf sign;

        if (!uid.TrySplit('.', data, sign)) {
            return false;
        }

        if (!Read(data, fuid.Time)) {
            return false;
        }

        if (!Read(data, fuid.Rand)) {
            return false;
        }

        const TString decodedSign = Base64Decode(sign) + TString((size_t)16, 0);
        NOpenSsl::NSha1::TCalcer hash;

        hash.UpdateWithPodValue(fuid.Time);
        hash.UpdateWithPodValue(fuid.Rand);

        const auto shasum = hash.Final();

        fuid.Time = InetToHost(fuid.Time);
        fuid.Rand = InetToHost(fuid.Rand);

        return RSAVerifier->SSL_VerifyFuid(shasum.data(), shasum.size(), (const unsigned char*)decodedSign.data());
    }

private:
    THolder<TRSAVerifier> RSAVerifier;
    TaoCrypt::RSA_PublicKey pubKey;
};

TFuidChecker::TFuidChecker(const TString& file)
    : Impl_(new TImpl(LoadKey(file)))
{
}

TFuidChecker::TFuidChecker(IInputStream& in)
    : Impl_(new TImpl(LoadKey(in)))
{
}

bool TFuidChecker::Parse(const TStringBuf& signature, TFuid& fuid) const {
    try {
        return Impl_->Parse(signature, fuid);
    } catch (...) {
    }

    return false;
}

TFuidChecker::~TFuidChecker() = default;

namespace {
    struct TChecker: public THolder<TFuidChecker> {
        inline TChecker() {
            const TStringBuf key =
                "-----BEGIN PUBLIC KEY-----\n"
                "MHwwDQYJKoZIhvcNAQEBBQADawAwaAJhAOic0tmjQnu3RJm1jh4OXxjpYjS4sHbO\n"
                "voYLKpOZ2x8PjFfEql5Cpzp3IWZY3vwHk87ikq2a4LKX2Fyoij5OxIBEbDnsUcUJ\n"
                "pA1ewuxv7nTgM5bAQ+dCH3VqFRo4ty0RJwIDAQAB\n"
                "-----END PUBLIC KEY-----";

            TMemoryInput mi(key.data(), key.size());

            Reset(new TFuidChecker(mi));
        }
    };
}

const TFuidChecker& FuidChecker() {
    return *Singleton<TChecker>()->Get();
}
