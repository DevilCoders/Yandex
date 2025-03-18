#include "autoru_offer.h"

#include <library/cpp/digest/md5/md5.h>

#include <util/generic/strbuf.h>
#include <util/string/hex.h>

#include <array>

namespace {
    MD5 ComputeSignature(TStringBuf key, TStringBuf salt) {
        MD5 md5;
        md5.Update(key);
        md5.Update(salt);

        return md5;
    }
}

namespace NAntiRobot {

// const re2::RE2 TAutoruOfferDetector::OfferPattern = {R"(([a-f0-9]{8}))"};
const re2::RE2 TAutoruOfferDetector::OfferPattern = {R"(/.+/.+/sale/(\d+)-([a-f0-9]{8})/?)"};

TAutoruOfferDetector::TAutoruOfferDetector(const TStringBuf& salt)
    : Salt(salt) {
}

bool TAutoruOfferDetector::Process(TStringBuf doc) const {
    TString offerNumber;
    TString offerHash;
    if (!re2::RE2::FullMatch(doc, OfferPattern, &offerNumber, &offerHash)) {
        return false;
    }

    return IsSignedHex(offerNumber,offerHash);
}

bool TAutoruOfferDetector::IsSigned(TStringBuf data, TStringBuf signature) const {
    if (signature.size() != SignLen) {
        return false;
    }

    std::array<char, SignLen> buf{};
    TStringBuf bufStr(buf.data(), buf.size());
    constexpr TStringBuf emptySalt{""};
    for (TStringBuf salt : {emptySalt, Salt}) {
        auto md5 = ComputeSignature(data, salt);
        md5.Final(reinterpret_cast<unsigned char*>(buf.data()));
        if (bufStr.Trunc(SignHexLen/2) == signature.Trunc(SignHexLen/2)) {
            return true;
        }
    }
    return false;
}

bool TAutoruOfferDetector::IsSignedHex(TStringBuf data, TStringBuf signature) const {
    if (signature.size() != SignHexLen) {
        return false;
    }

    std::array<char, SignLen> rawSignature{};
    TStringBuf rawSignatureBuf(rawSignature.data(), rawSignature.size());
    try {
        HexDecode(signature.data(), signature.size(), rawSignature.data());
    } catch (...) {
        return false;
    }
    return IsSigned(data, rawSignatureBuf);
}

} // namespace NAntiRobot
