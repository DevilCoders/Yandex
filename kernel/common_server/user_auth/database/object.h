#pragma once

#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>

class TDBAuthUserLink: public TAuthUserLink {
private:
    using TBase = TAuthUserLink;
public:
    TDBAuthUserLink() = default;

    TDBAuthUserLink(const TAuthUserLink& base)
        : TBase(base) {

    }

    static NFrontend::TScheme GetScheme(const IBaseServer& server) {
        auto result = TBase::GetScheme(server);
        return result;
    }

    using TId = ui32;

    bool operator!() const {
        return !AuthUserId;
    }

    TMaybe<ui64> GetRevisionMaybe() const {
        return {};
    }

    static TString GetTableName() {
        return "users";
    }

    static TString GetIdFieldName() {
        return "link_id";
    }

    static TString GetHistoryTableName() {
        return "users_history";
    }

    ui32 GetInternalId() const {
        return LinkId;
    }

    NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result = TBase::SerializeToJson();
        return result;
    }

    class TDecoder: public TBaseDecoder {
        DECODER_FIELD(SystemUserId);
        DECODER_FIELD(AuthModuleId);
        DECODER_FIELD(AuthUserId);
        DECODER_FIELD(LinkId);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase) {
            SystemUserId = GetFieldDecodeIndex("user_name", decoderBase);
            AuthModuleId = GetFieldDecodeIndex("auth_module_id", decoderBase);
            AuthUserId = GetFieldDecodeIndex("auth_user_id", decoderBase);
            LinkId = GetFieldDecodeIndex("link_id", decoderBase);
        }
    };

    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, SystemUserId);
        READ_DECODER_VALUE(decoder, values, AuthModuleId);
        READ_DECODER_VALUE(decoder, values, AuthUserId);
        READ_DECODER_VALUE(decoder, values, LinkId);
        return true;
    }

    NStorage::TTableRecord SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        result.Set("user_name", SystemUserId);
        result.SetOrNull("auth_module_id", AuthModuleId);
        result.Set("auth_user_id", AuthUserId);
        result.SetNotEmpty("link_id", LinkId);
        return result;
    }
};
