#pragma once

#include <google/protobuf/message.h>
#include <library/cpp/xml/document/xml-document.h>

namespace NProtobufXml {
    class TConverter {
    public:
        static void ProtoToXml(const google::protobuf::Message& message, NXml::TNode& node);
        static void XmlToProto(const NXml::TConstNode& node, google::protobuf::Message& message, const bool autodetectAttributes = false);
    };
}
