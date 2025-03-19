#include "external.h"

namespace NCS {

    void TExternalCipherConfig::DoInit(const TYandexConfig::Section* section) {
        TBase::DoInit(section);
        RequestEncodeField = section->GetDirectives().Value("RequestEncodeField", RequestEncodeField);
        RequestPlainField = section->GetDirectives().Value("RequestPlainField", RequestPlainField);
        ReplyEncodeField = section->GetDirectives().Value("ReplyEncodeField", ReplyEncodeField);
        ReplyPlainField = section->GetDirectives().Value("ReplyPlainField", ReplyPlainField);
        EncodeUri = section->GetDirectives().Value("EncodeUri", EncodeUri);
        DecodeUri = section->GetDirectives().Value("DecodeUri", DecodeUri);
    }

    void TExternalCipherConfig::DoToString(IOutputStream& os) const {
        TBase::DoToString(os);
        os << "RequestEncodeField: " << RequestEncodeField << Endl;
        os << "RequestPlainField: " << RequestPlainField << Endl;
        os << "ReplyEncodeField: " << ReplyEncodeField << Endl;
        os << "ReplyPlainField: " << ReplyPlainField << Endl;
        os << "EncodeUri: " << EncodeUri << Endl;
        os << "DecodeUri: " << DecodeUri << Endl;
    }

    bool TEncodeRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.SetUri(EncodeUri);
        NJson::TJsonValue postData;
        if (!postData.SetValueByPath(RequestPlainField, Plain)) {
            return false;
        }
        request.SetPostData(postData);
        return true;
    }

    bool TDecodeRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.SetUri(DecodeUri);
        NJson::TJsonValue postData;
        if (!postData.SetValueByPath(RequestEncodeField, Encoded)) {
            return false;
        }
        request.SetPostData(postData);
        return true;
    }

    bool TEncodeRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        NJson::TJsonValue encodeJson;
        if (!json.GetValueByPath(EncodeField, encodeJson)) {
            return false;
        }
        if (!encodeJson.IsString()) {
            return false;
        }
        Encoded = encodeJson.GetString();
        return true;
    }

    bool TDecodeRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
        NJson::TJsonValue plainJson;
        if (!json.GetValueByPath(PlainField, plainJson)) {
            return false;
        }
        if (!plainJson.IsString()) {
            return false;
        }
        Plain = plainJson.GetString();
        return true;
    }

    void TDecodeRequest::TResponse::ConfigureFromRequest(const IServiceApiHttpRequest* request) {
        auto decodeRequest = dynamic_cast<const TDecodeRequest*>(request);
        if (!decodeRequest) {
            TFLEventLog::Error("cannot cast IServiceApiHttpRequest to TDecodeRequest");
            return;
        }
        SetPlainField(decodeRequest->GetReplyPlainField());
    }

    void TEncodeRequest::TResponse::ConfigureFromRequest(const IServiceApiHttpRequest* request) {
        auto encodeRequest = dynamic_cast<const TEncodeRequest*>(request);
        if (!encodeRequest) {
            TFLEventLog::Error("cannot cast IServiceApiHttpRequest to TEncodeRequest");
            return;
        }
        SetEncodeField(encodeRequest->GetReplyEncodeField());
    }

    IAbstractCipher::TPtr TExternalCipherConfig::DoConstruct(const IBaseServer* server) const {
        return MakeAtomicShared<TExternalCipher>(*server, *this);
    }

    TExternalCipherConfig::TFactory::TRegistrator<TExternalCipherConfig> TExternalCipherConfig::Registrator("external_cipher");
}
