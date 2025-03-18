#include "autoru_tamper.h"

#include <library/cpp/digest/md5/md5.h>

#include <util/generic/algorithm.h>
#include <util/string/builder.h>

#include <array>


namespace NAntiRobot {


namespace {
    constexpr TStringBuf TAMPER_HEADER = "X-Timestamp"_sb;

    void ComputeAutoRuTamperRaw(
        const THeadersMap& headers,
        const TCgiParameters& cgiParams,
        TStringBuf salt,
        char* out
    ) {
        MD5 hasher;

        TVector<TString> sortedParams;

        for (const auto& [key, value] : cgiParams) {
            if (!key.empty()) {
                sortedParams.push_back(TString::Join(key, '=', value));
            }
        }

        Sort(sortedParams);

        for (auto& param : sortedParams) {
            param.to_lower();
            hasher.Update(param);
        }

        hasher.Update(headers.Get("X-Device-UID"));
        hasher.Update(salt);

        if (size_t contentLength; TryFromString(headers.Get("Content-Length"), contentLength)) {
            hasher.Update(ToString(contentLength));
        } else {
            hasher.Update("0");
        }

        hasher.Final(reinterpret_cast<unsigned char*>(out));
    }
} // anonymous namespace


bool CheckAutoRuTamper(
    const THeadersMap& headers,
    const TCgiParameters& cgiParams,
    TStringBuf salt
) {
    const auto& inputTamper = headers.Get(TAMPER_HEADER);

    if (inputTamper.size() != 32) {
        return false;
    }

    TString decodedInputTamper;

    try {
        decodedInputTamper = HexDecode(inputTamper);
    } catch (...) {
        return false;
    }

    std::array<char, 16> expectedTamper;
    ComputeAutoRuTamperRaw(headers, cgiParams, salt, expectedTamper.data());

    return decodedInputTamper == TStringBuf(expectedTamper.data(), expectedTamper.size());
}


} // namespace NAntiRobot
