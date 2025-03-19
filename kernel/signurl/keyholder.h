#pragma once

#include <contrib/libs/openssl/include/openssl/blowfish.h>

#include <library/cpp/json/writer/json_value.h>

#include <util/string/vector.h>
#include <util/stream/file.h>
#include <util/generic/hash.h>
#include <util/generic/yexception.h>

#include <utility>

namespace NSignUrl {
    using TKey = std::pair<TString, BF_KEY>;
    using TKeys = TVector<TKey>;

    void InitKey(TKey& key, const TString& base64DecodedKey);

    class ISignKeys {
    public:
        virtual ~ISignKeys() = default;
        virtual const TString& GetKey(const TString& keyno) const = 0;
        virtual const BF_KEY& GetParsedKey(const TString& keyno) const = 0;
        virtual const TKey& GetKeyData(const TString& keyno) const = 0;
    };

    class TSignKeys : public ISignKeys {
    public:
        TSignKeys() = default;
        TSignKeys(const TString& keysfile);
        virtual ~TSignKeys() = default;

        static TSignKeys CreateFromLines(const TString& lines);
        void LoadKeysFromFile(const TString& fileName);
        void LoadKeysFromJson(const NJson::TJsonValue& jsonTree);
        void LoadKeysFromJsonFile(const TString& jsonFileName);

        void AddKey(const TString& keyId, const TString& key);
        const TString& GetKey(const TString& keyno) const override;
        const BF_KEY& GetParsedKey(const TString& keyno) const override;
        const TKey& GetKeyData(const TString& keyno) const override;

    private:
        THashMap<TString, TKey> Keys_;
    };

    class TSingleKeySignKeys : public ISignKeys { // Return same key for all keyno's
    public:
        TSingleKeySignKeys(const TString& base64EncodedKey);
        const TString& GetKey(const TString& keyno) const override;
        const BF_KEY& GetParsedKey(const TString& keyno) const override;
        const TKey& GetKeyData(const TString& keyno) const override;

    private:
        TKey Key_;
    };
}
