#include "security.h"


NStorage::TTableRecord TSecretKeyInfo::SerializeToTableRecord() const {
    NStorage::TTableRecord result;
    result.Set("secret_data", SecretData);
    result.Set("secret_name", SecretName);
    result.Set("secret_type", SecretType);
    result.Set("secret_id", SecretId);
    return result;
}


bool TSecretKeyInfo::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    READ_DECODER_VALUE(decoder, values, SecretName);
    READ_DECODER_VALUE(decoder, values, SecretData);
    READ_DECODER_VALUE(decoder, values, SecretType);
    if (decoder.GetSecretId() != -1) {
        READ_DECODER_VALUE(decoder, values, SecretId);
    }
    return true;
}

NJson::TJsonValue TSecretKeyInfo::GetReport() const {
    return SerializeToTableRecord().BuildWT().SerializeToJson();
}

TSecretKeyInfo::TDecoder::TDecoder(const TMap<TString, ui32>& decoderBase) {
    SecretName = GetFieldDecodeIndex("secret_name", decoderBase);
    SecretData = GetFieldDecodeIndex("secret_data", decoderBase);
    SecretType = GetFieldDecodeIndex("secret_type", decoderBase);
    SecretId = GetFieldDecodeIndexOptional("secret_id", decoderBase);
}
