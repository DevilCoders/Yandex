#pragma once

#include <util/stream/output.h>

#include <google/protobuf/message.h>

namespace NProtoConfig {
    void PrintConfigUsage(const NProtoBuf::Message& message, IOutputStream& outputStream, bool colored = true);

    template <typename TMessage>
    void PrintConfigUsage(IOutputStream& outputStream, bool colored = true) {
        TMessage message;
        PrintConfigUsage(message, outputStream, colored);
    }

}
