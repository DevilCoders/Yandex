#include "proto_txt_helper.h"

#include <google/protobuf/messagext.h>
#include <google/protobuf/text_format.h>

#include <util/stream/mem.h>
#include <util/stream/str.h>

#include <util/generic/yexception.h>

namespace NQueryDataSaaS {

    void ReadTextConfig(IInputStream& input, ::google::protobuf::Message* message) {
        NProtoBuf::TextFormat::Parser parser;
        NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&input);
        const bool success = parser.Parse(&adaptor, message);
        Y_ENSURE(success, "Failed to parse message");
    }

    void WriteTextConfig(const ::google::protobuf::Message& message, IOutputStream& output) {
        NProtoBuf::TextFormat::Printer printer;
        printer.SetHideUnknownFields(true);
        NProtoBuf::io::TOutputStreamProxy proxy(&output);
        NProtoBuf::io::CopyingOutputStreamAdaptor stream(&proxy);
        const bool success = printer.Print(message, &stream);
        Y_ENSURE(success, "Failed to serialize message: " << message.GetDescriptor()->full_name());
    }

    void ReadTextConfig(TStringBuf input, google::protobuf::Message* message) {
        TMemoryInput minp(input.data(), input.size());
        ReadTextConfig(minp, message);
    }

    TString WriteTextConfig(const ::google::protobuf::Message& message) {
        TString res;
        TStringOutput stringOutputStream(res);
        WriteTextConfig(message, stringOutputStream);
        return res;
    }

}
