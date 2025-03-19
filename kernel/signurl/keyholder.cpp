#include "keyholder.h"

#include <contrib/libs/openssl/include/openssl/blowfish.h>

#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <utility>

namespace NSignUrl {
void InitKey(NSignUrl::TKey& key, const TString& base64decodedKey) {
    key.first = base64decodedKey;
    BF_set_key(&key.second, (int)key.first.size(), (const unsigned char*)key.first.data());
}

void TSignKeys::AddKey(const TString& keyId, const TString& key) {
    std::pair<TString, BF_KEY> newKey("", {{}, {}});
    InitKey(newKey, Base64Decode(key));
    Keys_[keyId] = newKey;
}

TSignKeys TSignKeys::CreateFromLines(const TString& lines) {
    Y_VERIFY(!lines.empty());
    TVector<TString> keys;
    StringSplitter(lines).Split('\n').SkipEmpty().Collect(&keys);
    TSignKeys result;
    size_t keyCount = 0;
    for (const auto& key: keys) {
        if (!key.empty()) {
            result.AddKey(ToString(keyCount), key);
            keyCount++;
        }
    }
    return result;
}

void TSignKeys::LoadKeysFromJson(const NJson::TJsonValue& jsonTree) {
    for (const auto& [serviceName, keys] : jsonTree.GetMap()) {
        for (const auto& [keyName, key] : keys.GetMap()) {
            const TString& keyValue = key["key"].GetString();
            AddKey(TString::Join(serviceName, "_", keyName), keyValue);
        }
    }
}

void TSignKeys::LoadKeysFromJsonFile(const TString& jsonFileName) {
    TString rawJsonFile = TFileInput{jsonFileName}.ReadAll();
    NJson::TJsonValue jsonTree;
    NJson::ReadJsonTree(rawJsonFile, &jsonTree, false);
    LoadKeysFromJson(jsonTree);
}

void TSignKeys::LoadKeysFromFile(const TString& fileName) {
    TString line;
    try {
        TFileInput kFile(fileName);
        size_t keyCount = 0;
        while (kFile.ReadLine(line)) {
            if (!line.empty()) {
                AddKey(ToString(keyCount), line);
                keyCount++;
            }
        }
    } catch (const yexception& e) {
        ythrow yexception() << "can't read key(s) from file: " << fileName << ": what=>" << e.what();
    }
}

TSignKeys::TSignKeys(const TString& keysfile) {
    LoadKeysFromFile(keysfile);
}

const TString& TSignKeys::GetKey(const TString& keyno) const {
    return GetKeyData(keyno).first;
}

const BF_KEY& TSignKeys::GetParsedKey(const TString& keyno) const {
    return GetKeyData(keyno).second;
}

const TKey& TSignKeys::GetKeyData(const TString& keyno) const {
    auto iter = Keys_.find(ToString(keyno));
    if (iter == Keys_.end()) {
        ythrow yexception() << "keyno=" << keyno << " is not found in key storage!";
    }
    return iter->second;
}


TSingleKeySignKeys::TSingleKeySignKeys(const TString& base64EncodedKey) {
    InitKey(Key_, Base64Decode(base64EncodedKey));
}

const TString& TSingleKeySignKeys::GetKey(const TString& /* keyno */) const {
    return Key_.first;
}

const BF_KEY& TSingleKeySignKeys::GetParsedKey(const TString& /* keyno */) const {
    return Key_.second;
}

const TKey& TSingleKeySignKeys::GetKeyData(const TString& /* keyno */) const {
    return Key_;
}
} // namespace NSignUrl

