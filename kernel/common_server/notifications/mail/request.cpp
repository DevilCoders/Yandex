#include "request.h"

#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/json_processing.h>

TMailRequest::TMailRequest(const TString& email, const TString& account, const TString& templateId)
    : Email(email)
    , Account(account)
    , TemplateId(templateId)
{   }

bool TMailRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
    request.SetUri("/api/0/" + Account + "/transactional/" + TemplateId + "/send");
    request.SetContentType("application/json; charset=UTF-8");

    NJson::TJsonMap postData;
    postData["async"] = true;

    NJson::TJsonArray to;
    to.AppendValue(NJson::TJsonMap({{"email", Email}}));
    postData.InsertValue("to", to);

    NJson::TJsonArray bcc;
    for (auto bccEmail: BccEmails) {
        bcc.AppendValue(NJson::TJsonMap({{"email", bccEmail}}));
    }
    postData.InsertValue("bcc", bcc);

    if (Args) {
        NJson::TJsonValue args;
        NJson::ReadJsonTree(Args.GetRef(), &args);
        postData.InsertValue("args", args);
    }

    if (Attachments) {
        TJsonProcessor::WriteObjectsContainer(postData, "attachments", Attachments);
    }

    request.SetPostData(postData);
    return true;
}

bool TMailRequest::TResponse::DeserializeFromJson(const NJson::TJsonValue& json, const bool isError) {
    auto& result = json["result"];
    if (!TJsonProcessor::Read(result, "status", Status, true)) {
        return false;
    }
    if (!TJsonProcessor::Read(result, "message", Message, isError)) {
        return false;
    }
    if (!TJsonProcessor::Read(result, "message_id", MessageId, !isError)) {
        return false;
    }
    if (!TJsonProcessor::Read(result, "task_id", TaskId, !isError)) {
        return false;
    }
    return true;
}

bool TMailRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& json) {
    if (!DeserializeFromJson(json)) {
        return false;
    }
    return true;
}

bool TMailRequest::TResponse::DoParseJsonError(const NJson::TJsonValue& json) {
    if (!DeserializeFromJson(json, true)) {
        return false;
    }
    return true;
}
