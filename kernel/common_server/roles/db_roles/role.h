#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/roles/abstract/role.h>

class TDBRole: public TUserRoleContainer {
private:
    using TBase = TUserRoleContainer;
    CS_ACCESS(TDBRole, ui32, RoleId, 0);
public:
    using TId = TString;

    TDBRole() = default;
    TDBRole(const TUserRoleContainer& base)
        : TBase(base)
    {

    }

    const TId& GetInternalId() const {
        return RoleName;
    }

    TMaybe<ui64> GetRevisionMaybe() const {
        return Revision;
    }

    static TString GetTableName() {
        return "roles";
    }

    static TString GetIdFieldName() {
        return "role_name";
    }

    static TString GetHistoryTableName() {
        return "roles_history";
    }

    class TDecoder: public TBaseDecoder {
        RTLINE_ACCEPTOR(TDecoder, RoleId, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, RoleName, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Revision, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase) {
            RoleId = GetFieldDecodeIndex("role_id", decoderBase);
            RoleName = GetFieldDecodeIndex("role_name", decoderBase);
            Revision = GetFieldDecodeIndex("revision", decoderBase);
        }
    };

    bool operator !() const {
        return !RoleName;
    }

    NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    NStorage::TTableRecord SerializeToTableRecord() const;
    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

    static NFrontend::TScheme GetScheme(const IBaseServer& server);

};
