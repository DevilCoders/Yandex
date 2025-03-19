#include "dek.h"

namespace NCS {

    NJson::TJsonValue TDBDek::SerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        TJsonProcessor::Write(result, "id", Id);
        TJsonProcessor::Write(result, "encrypted", Encrypted);
        TJsonProcessor::Write(result, "ttl", TTL);
        return result;
    }

    NStorage::TTableRecord TDBDek::SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        result.SetNotEmpty("id", Id);
        result.SetBytes("encrypted", Encrypted);
        result.Set("ttl", TTL);
        return result;
    }

    bool TDBDek::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, Id);
        if (!decoder.GetValueBytes(decoder.GetEncrypted(), values, Encrypted)) {
            TFLEventLog::Log("cannot read bytes from decoder");
            return false;
        }
        READ_DECODER_VALUE(decoder, values, TTL);
        return true;
    }

    NFrontend::TScheme TDBDek::GetScheme(const IBaseServer& /*server*/) {
        NFrontend::TScheme result;
        result.Add<TFSString>("id").SetReadOnly(true);
        result.Add<TFSString>("encrypted").SetReadOnly(true);
        result.Add<TFSNumeric>("ttl").SetReadOnly(true).SetRequired(false);
        return result;
    }
}
