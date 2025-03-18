#include "hypocrisy.h"

#include "chacha.h"

#include <antirobot/lib/evp.h>

#include <library/cpp/json/json_reader.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/charset/wide.h>
#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/system/byteorder.h>
#include <util/system/unaligned_mem.h>

#include <array>


namespace NHypocrisy {


namespace {


constexpr size_t KEY_SIZE = 32;
constexpr size_t NONCE_SIZE = 12;


struct TMetadata {
    TString Key;
    TString Nonce;
    ui32 NumRounds = 0;
    ui32 Version = 0;
    TInstant GenerationTime;

    static TMaybe<TMetadata> Parse(TStringBuf key, TStringBuf s) {
        TString decodedStr;

        try {
            decodedStr = Base64StrictDecode(s);
        } catch (...) {
            return Nothing();
        }

        if (s.size() < NAntiRobot::EVP_RECOMMENDED_GCM_IV_LEN) {
            return Nothing();
        }

        TStringBuf iv;
        TStringBuf ciphertext;
        TStringBuf(decodedStr).SplitAt(NAntiRobot::EVP_RECOMMENDED_GCM_IV_LEN, iv, ciphertext);

        const auto cleartext = NAntiRobot::TEvpDecryptor(NAntiRobot::EEvpAlgorithm::Aes256Gcm, key, iv)
            .Decrypt(ciphertext);

        NJson::TJsonValue json;
        if (!NJson::ReadJsonTree(cleartext, &json)) {
            return Nothing();
        }

        TMetadata metadata{
            .NumRounds = static_cast<ui32>(json["numRounds"].GetUInteger()),
            .Version = static_cast<ui32>(json["version"].GetUInteger()),
            .GenerationTime = TInstant::Seconds(json["generationTime"].GetUInteger())
        };

        if (metadata.Version != 1) {
            return Nothing();
        }

        try {
            metadata.Key = Base64StrictDecode(json["key"].GetString());
            metadata.Nonce = Base64StrictDecode(json["nonce"].GetString());
        } catch (...) {
            return Nothing();
        }

        return metadata;
    }
};


ui32 ReadLittle32(const char* ptr) {
    return LittleToHost(ReadUnaligned<ui32>(ptr));
}


TMaybe<TString> DecryptValue(const TMetadata& metadata, TStringBuf s) {
    TStringBuf encodedStr;
    TStringBuf idxStr;
    if (!s.TrySplit(';', encodedStr, idxStr)) {
        return Nothing();
    }

    TVector<wchar16> decodedWideStr;
    decodedWideStr.resize((Base64DecodeBufSize(encodedStr.size()) + 1) / 2);

    size_t decodedLen = 0;

    try {
        decodedLen = Base64StrictDecode(encodedStr, decodedWideStr.data()).size();
    } catch (...) {
        return Nothing();
    }

    if (decodedLen % 2 != 0) {
        return Nothing();
    }

    decodedWideStr.resize(decodedLen / 2);

    ui32 idx;
    if (!TryFromString(idxStr, idx)) {
        return Nothing();
    }

    std::array<ui32, 16> params = {
        0x6f626675, 0x73636174, 0x65206772, 0x65656421
    };

    for (size_t i = 0; i < KEY_SIZE / 4; ++i) {
        params[4 + i] = ReadLittle32(&metadata.Key[i * 4]);
    }

    params[12] = idx;

    for (size_t i = 0; i < NONCE_SIZE / 4; ++i) {
        params[13 + i] = ReadLittle32(&metadata.Nonce[i * 4]);
    }

    std::array<ui32, 16> block = params;

    for (ui32 i = 0; i < metadata.NumRounds; ++i) {
        PerformChaChaDoubleRound(block.data());
    }

    for (size_t i = 0; i < 16; ++i) {
        block[i] += params[i];
    }

    for (size_t i = 0; i < decodedWideStr.size(); ++i) {
        const ui16 c = LittleToHost(decodedWideStr[i]);
        const ui16 x = block[(i / 2) % block.size()] >> (i % 2 * 16);
        decodedWideStr[i] = c ^ x;
    }

    return WideToUTF8(decodedWideStr.data(), decodedWideStr.size());
}


} // namespace


TMaybe<TFingerprintData> TFingerprintData::Decrypt(TStringBuf key, TStringBuf s) {
    NJson::TJsonValue json;
    if (!NJson::ReadJsonTree(s, &json) || !json.IsMap()) {
        return Nothing();
    }

    auto& map = json.GetMapSafe();

    const auto encodedMetadataIt = map.find("pgrd");
    if (encodedMetadataIt == map.end() || !encodedMetadataIt->second.IsString()) {
        return Nothing();
    }

    const auto metadata = TMetadata::Parse(key, encodedMetadataIt->second.GetString());
    if (!metadata) {
        return Nothing();
    }

    if (metadata->Key.size() != KEY_SIZE || metadata->Nonce.size() != NONCE_SIZE) {
        return Nothing();
    }

    map.erase(encodedMetadataIt);

    const auto encodedTimestampIt = map.find("pgrdt");
    if (encodedTimestampIt == map.end() || !encodedTimestampIt->second.IsString()) {
        return Nothing();
    }

    const auto timestampStr = DecryptValue(*metadata, encodedTimestampIt->second.GetString());
    if (!timestampStr) {
        return Nothing();
    }

    ui64 timestampSecs;
    if (!TryFromString(*timestampStr, timestampSecs)) {
        return Nothing();
    }

    map.erase(encodedTimestampIt);

    for (auto& [key, value] : map) {
        if (key == "v") {
            continue;
        }

        if (!value.IsString()) {
            return Nothing();
        }

        auto maybeValue = DecryptValue(*metadata, value.GetString());
        if (!maybeValue) {
            return Nothing();
        }

        value = std::move(*maybeValue);
    }

    return TFingerprintData{
        .Fingerprint = std::move(json),
        .FingerprintTimestamp = TInstant::Seconds(timestampSecs),
        .InstanceTimestamp = metadata->GenerationTime
    };
}

bool TFingerprintData::IsStale(
    const TBundle& bundle,
    TDuration instanceLeeway,
    TDuration fingerprintLifetime,
    TInstant now
) const {
    return
        (
            InstanceTimestamp >= bundle.GenerationTime ||
            (
                InstanceTimestamp >= bundle.PrevGenerationTime &&
                now < bundle.GenerationTime + instanceLeeway
            )
        ) &&
        now < FingerprintTimestamp + fingerprintLifetime;
}


TBundle TBundle::Load(const TString& path) {
    TBundle bundle;

    const TFsPath fsPath(path);
    const auto jsonPath = fsPath / "bundle.json";

    TFileInput input(jsonPath);
    NJson::TJsonValue json;
    NJson::ReadJsonTree(&input, &json, true);

    const auto& map = json.GetMapSafe();

    if (const auto prevGenerationTime = map.FindPtr("prev_generation_time")) {
        bundle.PrevGenerationTime = TInstant::Seconds(prevGenerationTime->GetUIntegerSafe());
    }

    bundle.GenerationTime = TInstant::Seconds(map.at("generation_time").GetUIntegerSafe());
    bundle.DescriptorKey = Base64StrictDecode(map.at("descriptor_key").GetStringSafe());

    TVector<TFsPath> filePaths;
    fsPath.List(filePaths);

    for (const auto& filePath : filePaths) {
        if (!filePath.IsFile() || filePath.GetExtension() != "js") {
            continue;
        }

        TFileInput instanceInput(filePath);
        bundle.Instances.push_back(instanceInput.ReadAll());
    }

    return bundle;
}


} // namespace NHypocrisy
