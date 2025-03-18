#include "spravka.h"

#include "ar_utils.h"
#include "evp.h"
#include "keyring.h"
#include "spravka_key.h"

#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/scan/scan.h>

#include <util/digest/city.h>
#include <util/random/random.h>
#include <util/stream/str.h>
#include <util/string/hex.h>

#include <contrib/libs/openssl/include/openssl/rand.h>

#include <antirobot/idl/spravka_data.pb.h>

#include <array>


namespace {
    struct TSpravkaScanner {
        TStringBuf Time;
        TStringBuf Addr;
        TStringBuf Data;
        TStringBuf Uid;
        TStringBuf Sign;

        inline bool Valid() const noexcept {
            return !!Time && !!Addr && !!Uid && !!Sign;
        }

        inline void operator() (TStringBuf key, TStringBuf value) {
            if (key == "t"sv) {
                Time = value;
            } else if (key == "i"sv) {
                Addr = value;
            } else if (key == "D"sv) {
                Data = value;
            } else if (key == "u"sv) {
                Uid = value;
            } else if (key == "h"sv) {
                Sign = value;
            }
        }
    };
} // anonymous namespace

namespace NAntiRobot {

template <typename Iter>
TSpravka::ECookieParseResult ParseRange(Iter begin, Iter end, TStringBuf domain, TSpravka& result) {
    if (begin == end) {
        return TSpravka::ECookieParseResult::NotFound;
    }

    TSpravka bestSpravka;
    bool foundGoodSpravka = false;

    for (Iter toSpravka = begin; toSpravka != end; ++toSpravka) {
        TSpravka spravka;
        if (
            spravka.Parse(toSpravka->second, domain) &&
            TSpravka::GetTime(spravka.Uid) > TSpravka::GetTime(bestSpravka.Uid)
        ) {
            foundGoodSpravka = true;
            bestSpravka = spravka;
        }
    }

    if (!foundGoodSpravka) {
        return TSpravka::ECookieParseResult::Invalid;
    }

    result = bestSpravka;
    return TSpravka::ECookieParseResult::Valid;
}

TSpravka::ECookieParseResult TSpravka::ParseCookies(const THttpCookies& cookies, TStringBuf domain) {
    const THttpCookies::TConstIteratorPair range = cookies.EqualRange(NAMES[0]);
    return ParseRange(range.first, range.second, domain, *this);
}

TSpravka::ECookieParseResult TSpravka::ParseCGI(const TCgiParameters& cgi, TStringBuf domain) {
    auto result = TSpravka::ECookieParseResult::NotFound;
    for (auto NAME : NAMES) {
        auto range = cgi.equal_range(NAME);
        auto maybe = ParseRange(range.first, range.second, domain, *this);
        if (result == TSpravka::ECookieParseResult::NotFound || maybe == TSpravka::ECookieParseResult::Valid) {
            result = maybe;
        }
    }
    return result;
}

TSpravka TSpravka::Generate(
        const TAddr& addr,
        TStringBuf domain,
        TDegradation degradation) {
    return Generate(addr, Now(), domain, degradation);
}

TSpravka TSpravka::Generate(
        const TAddr& addr,
        TInstant gen,
        TStringBuf domain,
        TDegradation degradation) {
    const ui64 randomUid = ui64(gen.MilliSeconds()) * ui64(1000000) + RandomNumber<ui64>(1000000);

    return TSpravka(randomUid, addr, gen, domain, degradation);
}

TInstant TSpravka::GetTime(ui64 uid) {
    return TInstant::MicroSeconds(uid / 1000000 * 1000);
}

TString TSpravka::ToString() const {
    TStringStream ret;

    ret << "t=" << Time.Seconds() << ";"
        << "i=" << Addr << ";"
        << "D=" << EncryptData() << ";"
        << "u=" << Uid;

    const size_t lengthOfPublicBody = ret.Size();

    using TExtraField = std::pair<TStringBuf, const TString&>;
    const TExtraField extraFields[] = {
        TExtraField(TStringBuf("d"), Domain),
    };

    for (size_t i = 0; i < Y_ARRAY_SIZE(extraFields); ++i) {
        const TExtraField& extraField = extraFields[i];
        if (!extraField.second.empty()) {
            ret << ";" << extraField.first << "=" << extraField.second;
        }
    }

    const TString sign = TKeyRing::Instance()->SignHex(ret.Str());

    ret.Str().resize(lengthOfPublicBody); // has no information about Domain in body, only in signature

    ret << ";h=" << sign;

    return Base64Encode(ret.Str());
}

bool TSpravka::Parse(TStringBuf spravkaBuf, TStringBuf domain) {
    const size_t unescapedLen = CgiUnescapeBufLen(spravkaBuf.size());
    TTempBuf tmpBuf(unescapedLen + Base64DecodeBufSize(unescapedLen) + 50);
    TStringBuf sprDecoded;

    try {
        TStringBuf spravkaUnesc = CgiUnescape(tmpBuf.Data(), spravkaBuf);

        sprDecoded = Base64Decode(spravkaUnesc, (void*)spravkaUnesc.end());
    } catch (...) {
    }

    TSpravkaScanner p;
    ScanKeyValue<false, ';', '='>(sprDecoded, p);

    if (!p.Valid()) {
        return false;
    }

    TStringBuf signedData(sprDecoded.begin(), p.Uid.end());

    TStringStream extendedData;
    extendedData << signedData << ";d=" << domain;

    if (!TKeyRing::Instance()->IsSignedHex(extendedData.Str(), p.Sign)) {
        return false;
    }

    Domain = domain;
    Uid = FromStringWithDefault<ui64>(p.Uid);
    Addr.FromString(p.Addr);
    Time = TInstant::Seconds(FromStringWithDefault<ui32>(p.Time));
    if (!p.Data.empty()) {
        try {
            DecryptData(p.Data);
        } catch (...) {
            return false;
        }
    }

    return true;
}

TString TSpravka::AsCookie() const {
    TTempBufOutput output;
    output << TSpravka::NAMES[0] << '=' << ToString()
        << "; domain=." << Domain;

    struct tm tM;
    const TInstant expires(TInstant::Now() + TDuration::Days(30));
    output << "; path=/; expires=" << Strftime("%a, %d-%b-%Y %H:%M:%S GMT", expires.GmTime(&tM));

    return TString(output.Data(), output.Filled());
}

ui64 TSpravka::Hash() const {
    // D= - data is random every run.
    // Don't use it in hash.
    TStringStream ret;
    ret << "t=" << Time.Seconds() << ";"
        << "i=" << Addr << ";"
        << "u=" << Uid << ";"
        << "d=" << Domain << ";";

    return CityHash64(ret.Str());
}

TString TSpravka::EncryptData() const {
    std::array<ui8, EVP_RECOMMENDED_IV_LEN> iv;
    Y_ENSURE(
        RAND_bytes(iv.data(), iv.size()),
        "RAND_bytes failed"
    );

    NAntirobotSpravkaProto::TSpravkaData data;
    data.SetDegradationWeb(Degradation.Web);
    data.SetDegradationMarket(Degradation.Market);
    data.SetDegradationUslugi(Degradation.Uslugi);
    data.SetDegradationAutoru(Degradation.Autoru);

    TString encrypted(reinterpret_cast<char*>(iv.data()), iv.size());

    TEvpEncryptor encryptor(EEvpAlgorithm::Aes256Gcm, TSpravkaKey::Instance()->Get(), encrypted);
    encryptor.Encrypt(data.SerializeAsString(), &encrypted);

    return HexEncode(encrypted);
}

void TSpravka::DecryptData(TStringBuf hexData) {
    const TString encryptedData = HexDecode(hexData);

    Y_ENSURE(
        encryptedData.size() >= EVP_RECOMMENDED_IV_LEN,
        "spravka data too short"
    );

    TStringBuf iv, data;
    TStringBuf(encryptedData).SplitAt(EVP_RECOMMENDED_IV_LEN, iv, data);

    TEvpDecryptor decryptor(EEvpAlgorithm::Aes256Gcm, TSpravkaKey::Instance()->Get(), iv);

    const auto decryptedData = decryptor.Decrypt(data);

    NAntirobotSpravkaProto::TSpravkaData spravkaData;
    Y_ENSURE(
        spravkaData.ParseFromArray(decryptedData.Data(), decryptedData.Size()),
        "Invalid spravka data protobuf"
    );

    Degradation.Web = spravkaData.GetDegradationWeb();
    Degradation.Market = spravkaData.GetDegradationMarket();
    Degradation.Uslugi = spravkaData.GetDegradationUslugi();
    Degradation.Autoru = spravkaData.GetDegradationAutoru();
}

} // namespace NAntiRobot
