#include "message.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {

    NFallbackProxy::TRequestPQMessage::TRequestPQMessage(const NNeh::THttpRequest& request, const TString& messageId)
        : Request(request)
        , MessageId(messageId)
    {
    }

    IPQMessage::TPtr NFallbackProxy::TRequestPQMessage::BuildRequestPQMessage(const NNeh::THttpRequest& request, const TString& messageId) {
        THolder<TRequestPQMessage> result = MakeHolder<TRequestPQMessage>(request, messageId);
        return result.Release();
    }

    TMaybe<NCS::NFallbackProxy::TRequestPQMessage> NFallbackProxy::TRequestPQMessage::BuildFromPQMessage(const IPQMessage& pqMessage) {
        NNeh::THttpRequest request;
        NJson::TJsonValue jsonInfo;
        if (!NJson::ReadJsonFastTree(pqMessage.GetContent().AsStringBuf(), &jsonInfo)) {
            TFLEventLog::Error("pq message cannot be interpreted as json for request parsing");
            return Nothing();
        }
        if (!request.DeserializeFromJson(jsonInfo)) {
            TFLEventLog::Error("json pq message cannot be interpreted as request");
            return Nothing();
        }
        return TRequestPQMessage(request, pqMessage.GetMessageId());
    }

    TBlob NFallbackProxy::TRequestPQMessage::GetContent() const noexcept {
        return TBlob::FromString(Request.SerializeToJson().GetStringRobust());
    }

}
