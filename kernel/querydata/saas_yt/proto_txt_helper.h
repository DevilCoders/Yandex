#pragma once

#include <util/stream/input.h>
#include <util/stream/output.h>

namespace google {
    namespace protobuf {
        class Message;
    }
}

namespace NQueryDataSaaS {

    void ReadTextConfig(IInputStream& input, ::google::protobuf::Message* message);
    void ReadTextConfig(TStringBuf input, ::google::protobuf::Message* message);

    void WriteTextConfig(const ::google::protobuf::Message& message, IOutputStream& output);
    TString WriteTextConfig(const ::google::protobuf::Message& message);

}
