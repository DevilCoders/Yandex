#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>

template <class TOwnerIdExt = TString, class TSlaveIdExt = TString>
class TDBLink {
private:
    CS_ACCESS(TDBLink, ui64, LinkId, 0);
    CSA_DEFAULT(TDBLink, TSlaveIdExt, SlaveId);
    CSA_DEFAULT(TDBLink, TOwnerIdExt, OwnerId);
public:
    using TOwnerId = TOwnerIdExt;
    using TSlaveId = TSlaveIdExt;
    TDBLink() = default;
    class TDecoder: public TBaseDecoder {
        RTLINE_ACCEPTOR(TDecoder, LinkId, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, SlaveId, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, OwnerId, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase) {
            LinkId = GetFieldDecodeIndex("link_id", decoderBase);
            SlaveId = GetFieldDecodeIndex("slave_id", decoderBase);
            OwnerId = GetFieldDecodeIndex("owner_id", decoderBase);
        }
    };

    using TId = ui64;

    static TString GetIdFieldName() {
        return "link_id";
    }

    ui64 GetInternalId() const {
        return LinkId;
    }

    bool operator !() const {
        return !LinkId;
    }

    NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        TJsonProcessor::Write(result, "link_id", LinkId);
        TJsonProcessor::Write(result, "slave_id", SlaveId);
        TJsonProcessor::Write(result, "owner_id", OwnerId);
        return result;
    }

    bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TJsonProcessor::Read(jsonInfo, "link_id", LinkId)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "slave_id", SlaveId, true)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "owner_id", OwnerId, true)) {
            return false;
        }
        return true;
    }

    NStorage::TTableRecord SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        if (LinkId) {
            result.Set("link_id", LinkId);
        }
        result.Set("slave_id", SlaveId);
        result.Set("owner_id", OwnerId);
        return result;
    }

    bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, LinkId);
        READ_DECODER_VALUE(decoder, values, SlaveId);
        READ_DECODER_VALUE(decoder, values, OwnerId);
        return true;
    }
};
