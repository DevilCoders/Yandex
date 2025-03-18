#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

namespace NProtobufFromXml {
    using PbMessage = google::protobuf::Message;
    using PbDescriptor = google::protobuf::Descriptor;
    using PbFieldDescriptor = google::protobuf::FieldDescriptor;
    using PbReflection = google::protobuf::Reflection;

}
