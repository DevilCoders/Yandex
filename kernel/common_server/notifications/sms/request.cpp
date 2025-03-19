#include "request.h"

#include <kernel/common_server/library/logging/events.h>
#include <library/cpp/xml/document/xml-document.h>

TSmsRequest::TSmsRequest(const TString& sender, const TString& text)
    : Sender(sender)
    , Text(text)
{
}

bool TSmsRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
    request.SetUri("/sendsms");
    request.AddCgiData("text", Text).Encode();
    request.AddCgiData("phone", Phone).Encode().IgnoreEmpty();
    request.CgiInserter().IgnoreEmpty()
        .Add("sender", Sender)
        .Add("uid", UID)
        .Add("phone_id", PhoneId)
        .Add("route", Route)
        .Add("udh", Udh)
        .Add("identity", Identity)
        .Add("text_template_params", TextTemplateParams.SerializeToJson())
        .Add("utf8", 1);
    return true;
}

bool TSmsRequest::TResponse::DoParseXMLReply(const NXml::TDocument& doc) {
    try {
        SmsId = doc.Root().Node("message-sent").Attr<TString>("id", "");
        if (SmsId.empty()) {
            TFLEventLog::Log("sms id not found", TLOG_ERR)("xml", doc.ToString());
            return false;
        }
        return true;
    } catch (...) {
        TFLEventLog::Log(CurrentExceptionMessage());
        return false;
    }
}

bool TSmsRequest::TResponse::DoParseXMLError(const NXml::TDocument& doc) {
    try {
        ErrorMessage = doc.Root().Node("error").Value<TString>("");
        ErrorCode = doc.Root().Node("errorcode").Value<TString>("");
        if (ErrorMessage.empty() && ErrorCode.empty()) {
            TFLEventLog::Log("error code or message not found", TLOG_ERR)("xml", doc.ToString());
            return false;
        }
        return true;
    } catch (...) {
        TFLEventLog::Log(CurrentExceptionMessage());
        return false;
    }
}
