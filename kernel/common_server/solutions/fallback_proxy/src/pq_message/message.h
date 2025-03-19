#pragma once

#include <kernel/common_server/library/persistent_queue/abstract/pq.h>

namespace NCS {
    namespace NFallbackProxy {
        class TRequestPQMessage: public IPQMessage {
        private:
            CSA_READONLY_DEF(NNeh::THttpRequest, Request);
            const TString MessageId;

        public:
            TRequestPQMessage(const NNeh::THttpRequest& request, const TString& messageId);
            static IPQMessage::TPtr BuildRequestPQMessage(const NNeh::THttpRequest& request, const TString& messageId);
            static TMaybe<TRequestPQMessage> BuildFromPQMessage(const IPQMessage& pqMessage);
            virtual const TString& GetMessageId() const noexcept override {
                return MessageId;
            }
            virtual TBlob GetContent() const noexcept override;
        };
    }
}
