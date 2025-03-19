#pragma once
#include "pq.h"
#include <util/generic/guid.h>

namespace NXml {
    class TDocument;
    class TNode;
    class TConstNode;
}

namespace NExternalAPI {
    class IQueueRequest {
    private:
        CS_ACCESS(IQueueRequest, TString, MessageId, TGUID::CreateTimebased().AsUuidString());
    protected:
        virtual bool FillMessage(NCS::TPQMessageSimple& result) const = 0;
        virtual TString BuildMessageId(const TString& baseMessageId) const {
            return baseMessageId;
        }
    public:
        IQueueRequest() = default;
        virtual ~IQueueRequest() = default;
        virtual NCS::IPQMessage::TPtr BuildMessage() const final;
    };

    class IXMLQueueRequest: public IQueueRequest {
    protected:
        virtual TMaybe<NXml::TDocument> BuildXMLQueueRequest() const = 0;
        virtual bool FillMessage(NCS::TPQMessageSimple& result) const override final;
    public:

    };
}

