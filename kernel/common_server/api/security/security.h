#pragma once
#include <kernel/common_server/api/history/event.h>
#include <kernel/common_server/api/history/manager.h>
#include <kernel/common_server/api/history/cache.h>
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/util/accessor.h>

class ITagsHistoryContext;


class TPublicKeyInfo {
public:
    using TId = TString;

    class TDecoder : public TBaseDecoder {
        RTLINE_ACCEPTOR(TDecoder, OwnerId, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, PublicKey, i32, -1);

    public:
        TDecoder() = default;

        TDecoder(const TMap<TString, ui32>& decoderBase) {
            OwnerId = GetFieldDecodeIndex("owner_id", decoderBase);
            PublicKey = GetFieldDecodeIndex("public_key", decoderBase);
        }
    };

    TPublicKeyInfo() = default;

    NStorage::TTableRecord SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        result.Set("owner_id", OwnerId);
        result.Set("public_key", PublicKey);
        return result;
    }

    bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, OwnerId);
        READ_DECODER_VALUE(decoder, values, PublicKey);
        return true;
    }

    static TString GetHistoryTableName() {
        return "public_keys_history";
    }

    static TString GetTableName() {
        return "public_keys";
    }

    static TString GetIdFieldName() {
        return "owner_id";
    }

    using TObjectId = TString;

    TObjectId GetInternalId() const {
        return OwnerId;
    }

    bool operator!() const {
        return true;
    }

private:
    RTLINE_READONLY_ACCEPTOR(OwnerId, TString, "");
    RTLINE_READONLY_ACCEPTOR(PublicKey, TString, "");
};


class TPublicKeysCache : public TDBEntitiesCache<TPublicKeyInfo> {
private:
    using TBase = TDBEntitiesCache<TPublicKeyInfo>;
public:
    using TBase::TBase;
};

enum class ESecretType {
    HandlerKey = 1 /* "handler_key" */
};

class TSecretKeyInfo {
public:
    using TId = ui64;
    using TObjectId = ui64;

    class TDecoder : public TBaseDecoder {
        RTLINE_ACCEPTOR(TDecoder, SecretName, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, SecretData, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, SecretType, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, SecretId, i32, -1);

    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase);
    };

    TSecretKeyInfo() = default;

    NStorage::TTableRecord SerializeToTableRecord() const;
    bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

    static TString GetHistoryTableName() {
        return "drive_secrets_history";
    }

    static TString GetIdFieldName() {
        return "secret_id";
    }

    static TString GetTableName() {
        return "drive_secrets";
    }

    TObjectId GetInternalId() const {
        return SecretId;
    }

    ui64 GetVersion() const {
        return SecretId;
    }

    bool operator!() const {
        return false;
    }

    TMaybe<ui64> GetRevisionMaybe() const {
        return Nothing();
    }

    NJson::TJsonValue GetReport() const;

private:
    RTLINE_READONLY_ACCEPTOR(SecretId, ui64, 0);
    RTLINE_READONLY_ACCEPTOR(SecretData, TString, "");
    RTLINE_READONLY_ACCEPTOR(SecretName, TString, "");
    RTLINE_READONLY_ACCEPTOR(SecretType, ESecretType, ESecretType::HandlerKey);
};


class TSecretsCache : public TDBEntitiesCache<TSecretKeyInfo> {
private:
    using TBase = TDBEntitiesCache<TSecretKeyInfo>;
public:
    using TBase::TBase;
};

class TSecretsManagerConfig : public TDBEntitiesManagerConfig {
public:
    using TDBEntitiesManagerConfig::TDBEntitiesManagerConfig;
};

class TSecretsManager : public TDBEntitiesManager<TSecretKeyInfo> {
private:
    using TBase = TDBEntitiesManager<TSecretKeyInfo>;

public:
    TSecretsManager(const IBaseServer& /*server*/, const IHistoryContext& context,
        const TSecretsManagerConfig& config)
        : TBase(context, config)
    {}

protected:
    virtual TSecretKeyInfo PrepareForUsage(const TSecretKeyInfo& evHistory) const override {
        return evHistory;
    }

public:
    using TBase::TBase;
};

