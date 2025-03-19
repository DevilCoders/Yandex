#include "object.h"

namespace NCS {
    NJson::TJsonValue TDBReserveEncrypted::SerializeToJson() const {
        NJson::TJsonValue result;
        JWRITE(result, "reserve_id", ReserveId);
        JWRITE(result, "encrypted", Encrypted);
        JWRITE(result, "reserve", Reserve);
        JWRITE(result, "hash", Hash);
        JWRITE(result, "reserve_hash", ReserveHash);
        return result;
    }
    bool TDBReserveEncrypted::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        JREAD_UINT(jsonInfo, "reserve_id", ReserveId);
        JREAD_STRING_OPT(jsonInfo, "encrypted", Encrypted);
        JREAD_STRING(jsonInfo, "reserve", Reserve);
        JREAD_STRING_OPT(jsonInfo, "hash", Hash);
        JREAD_STRING(jsonInfo, "reserve_hash", ReserveHash);
        return true;
    }
    NStorage::TTableRecord TDBReserveEncrypted::SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        if (ReserveId) {
            result.Set("reserve_id", ReserveId);
        }
        if (!!Encrypted) {
            result.SetBytes("encrypted", Encrypted);
        }
        result.SetBytes("reserve", Reserve);
        if (!!Hash) {
            result.Set("hash", Hash);
        }
        result.Set("reserve_hash", ReserveHash);
        return result;
    }

    bool TDBReserveEncrypted::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, ReserveId);
        if (!decoder.GetValueBytes(decoder.GetEncrypted(), values, Encrypted)) {
            TFLEventLog::Log("cannot read bytes from decoder");
            return false;
        }
        if (!decoder.GetValueBytes(decoder.GetReserve(), values, Reserve)) {
            TFLEventLog::Log("cannot read bytes from decoder");
            return false;
        }
        READ_DECODER_VALUE(decoder, values, Hash);
        READ_DECODER_VALUE(decoder, values, ReserveHash);
        return true;
    }

    NFrontend::TScheme TDBReserveEncrypted::GetScheme(const IBaseServer& /*server*/) {
        NFrontend::TScheme result;
        result.Add<TFSNumeric>("reserve_id").SetRequired(true).SetReadOnly(true);
        result.Add<TFSString>("encrypted").SetRequired(false).SetReadOnly(true);
        result.Add<TFSNumeric>("reserve").SetRequired(true).SetReadOnly(true);
        result.Add<TFSNumeric>("hash").SetRequired(false).SetReadOnly(true);
        result.Add<TFSNumeric>("reserve_hash").SetRequired(true).SetReadOnly(true);
        return result;
    }
}