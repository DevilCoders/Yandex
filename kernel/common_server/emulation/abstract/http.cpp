#include "http.h"
#include <util/stream/file.h>
#include <kernel/common_server/util/types/string_normal.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/template/abstract.h>

namespace NCS {
    TEmulationHttpCase::TFactory::TRegistrator<TEmulationHttpCase> TEmulationHttpCase::Registrator(TEmulationHttpCase::GetTypeName());

    NJson::TJsonValue THttpHeaderEmulation::SerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        TJsonProcessor::Write(result, "name", Name);
        TJsonProcessor::Write(result, "value", Value);
        return result;
    }

    bool THttpHeaderEmulation::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TJsonProcessor::Read(jsonInfo, "name", Name) || !Name) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "value", Value)) {
            return false;
        }
        return true;
    }

    NJson::TJsonValue TEmulationHttpCase::DoSerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        TJsonProcessor::WriteContainerArray(result, "uri", Uri);
        TJsonProcessor::Write(result, "reply_body", ReplyBody);
        TJsonProcessor::Write(result, "reply_code", ReplyCode);
        TJsonProcessor::Write(result, "content_type", ContentType);
        TJsonProcessor::WriteObjectsArray(result, "reply_headers", ReplyHeaders);
        return result;
    }

    bool TEmulationHttpCase::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        {
            TSet<TString> uriSet;
            if (!TJsonProcessor::ReadContainer(jsonInfo, "uri", uriSet, true, false)) {
                return false;
            }
            for (auto&& i : uriSet) {
                Uri.emplace(NCS::TStringNormalizer::TruncRet(i, '/'));
            }
        }
        if (!TJsonProcessor::Read(jsonInfo, "content_type", ContentType, true)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "reply_body", ReplyBody)) {
            return false;
        }
        if (ReplyBody.StartsWith("$FILE:")) {
            try {
                TFileInput fi(ReplyBody.substr(6));
                ReplyBody = fi.ReadAll();
            } catch (...) {
                TFLEventLog::Log("cannot open file")("reason", CurrentExceptionMessage());
                return false;
            }
        }
        if (!TJsonProcessor::Read(jsonInfo, "reply_code", ReplyCode)) {
            return false;
        }
        if (!TJsonProcessor::ReadObjectsContainer(jsonInfo, "reply_headers", ReplyHeaders)) {
            return false;
        }
        return true;
    }

    bool TEmulationHttpCase::CheckHttpRequest(const IReplyContext& context) const {
        return Uri.contains(context.GetUri());
    }

    NUtil::THttpReply TEmulationHttpCase::GetHttpReply(const IReplyContext& context) const {
        TTemplateSettings parametrizer("emulation." + context.GetUri(), ICSOperator::GetServer().GetSettings());
        NUtil::THttpReply result;
        result.SetCode(ReplyCode);
        THttpHeaders replyHeaders;
        for (auto&& i : ReplyHeaders) {
            replyHeaders.AddHeader(i.GetName(), i.GetValue());
        }
        replyHeaders.AddOrReplaceHeader("Content-Type", parametrizer.Apply(ContentType));
        result.SetHeaders(std::move(replyHeaders));
        if (!!ReplyBody) {
            result.SetContent(parametrizer.Apply(ReplyBody));
        }
        return result;
    }

}
