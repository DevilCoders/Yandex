#include "request.h"
#include <library/cpp/xml/document/node-attr.h>

namespace NExternalAPI {

    NCS::IPQMessage::TPtr IQueueRequest::BuildMessage() const {
        auto mSimple = MakeHolder<NCS::TPQMessageSimple>(TBlob(), BuildMessageId(MessageId));
        if (!FillMessage(*mSimple)) {
            return nullptr;
        }
        return mSimple.Release();
    }

    bool IXMLQueueRequest::FillMessage(NCS::TPQMessageSimple& result) const {
        TMaybe<NXml::TDocument> xmlContent = BuildXMLQueueRequest();
        if (!xmlContent) {
            return false;
        }
        result.SetContent(TBlob::FromString(xmlContent->ToString("", false)));
        return true;
    }

}
