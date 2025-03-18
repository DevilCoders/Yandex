#include "keyring.h"

#include <antirobot/lib/ar_utils.h>

#include <library/cpp/digest/md5/md5.h>

#include <util/string/hex.h>

#include <array>


namespace NAntiRobot {


namespace {
    MD5 ComputeSignature(TStringBuf key, TStringBuf data) {
        MD5 md5;
        md5.Update(data);
        md5.Update(key);

        return md5;
    }
}


void TSecretKey::Sign(TStringBuf data, char* out) const {
    auto md5 = ComputeSignature(Bytes, data);
    md5.Final(reinterpret_cast<unsigned char*>(out));
}

void TSecretKey::SignHex(TStringBuf data, char* out) const {
    std::array<char, SignLen> signature;
    Sign(data, signature.data());

    LowerHexEncode(TStringBuf(signature.begin(), signature.size()), out);
}

bool TKeyRing::IsSigned(TStringBuf data, TStringBuf signature) const {
    Y_ENSURE(
        signature.size() == TSecretKey::SignLen,
        "Invalid signature length"
    );

    std::array<char, TSecretKey::SignLen> buf;
    TStringBuf bufStr(buf.data(), buf.size());

    // Look through all the keys, to be able to check data signed with outdated key
    for (const auto& key : Keys) {
        key.Sign(data, buf.data());

        if (bufStr == signature) {
            return true;
        }
    }

    return false;
}

bool TKeyRing::IsSignedHex(TStringBuf data, TStringBuf signature) const {
    if (signature.size() != TSecretKey::SignHexLen) {
        return false;
    }

    std::array<char, TSecretKey::SignLen> rawSignature;
    try {
        HexDecode(signature.data(), signature.size(), rawSignature.data());
    } catch (...) {
        return false;
    }
    return IsSigned(data, TStringBuf(rawSignature.data(), rawSignature.size()));
}

void TKeyRing::Sign(TStringBuf data, char* tmp) const {
    Get().Sign(data, tmp);
}

TString TKeyRing::SignHex(TStringBuf data) const {
    std::array<char, TSecretKey::SignHexLen> buf;
    Get().SignHex(data, buf.data());
    return TString(buf.data(), buf.size());
}

const TSecretKey& TKeyRing::Get() const {
    Y_ENSURE(
        !Keys.empty(),
        "TKeyRing keys haven't been loaded"
    );

    // Keys are rotating on a daily basis. Currently there are 30 keys.
    // Each day, first one (Keys[0]) is deleted and new one (Keys.back()) goes at the end of the list.
    // To be always able to check any signature, we should not sign data with newest key.
    // In case it's not delivered to some instance yet.
    // Conversely, we should not use the oldest one, because it will be deleted during next day's rotation.

    // Allow difference between key files on any two instances no more than a week
    const size_t keyIndex = Keys.size() < 7 ? Keys.size() - 1 : Keys.size() - 7;
    return Keys[keyIndex];
}


} // namespace NAntiRobot
