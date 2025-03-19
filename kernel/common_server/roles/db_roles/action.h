#pragma once
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/user_role/abstract/abstract.h>

class TDBItemPermissions: public TItemPermissionContainer {
private:
    using TBase = TItemPermissionContainer;
public:
    TDBItemPermissions() = default;
    TDBItemPermissions(const TItemPermissionContainer& base)
        : TBase(base)
    {

    }
    static TString GetIdFieldName() {
        return "item_id";
    }
    static NFrontend::TScheme GetScheme(const IBaseServer& server);

    static TString GetTableName() {
        return "item_permissions";
    }

    static TString GetHistoryTableName() {
        return "item_permissions_history";
    }

    TMaybe<ui64> GetRevisionMaybe() const {
        return Revision;
    }

    using TId = TString;

    const TString& GetInternalId() const {
        return GetItemId();
    }

    class TDecoder: public TBase::TDecoder {
        RTLINE_ACCEPTOR(TDecoder, ItemId, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Revision, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase)
            : TDBItemPermissions::TBase::TDecoder(decoderBase)
        {
            ItemId = GetFieldDecodeIndex("item_id", decoderBase);
            Revision = GetFieldDecodeIndex("revision", decoderBase);
        }
    };

    bool operator !() const {
        return !ItemId;
    }

    NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    NStorage::TTableRecord SerializeToTableRecord() const;
    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
};
