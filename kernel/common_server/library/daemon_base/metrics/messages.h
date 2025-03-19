#pragma once

#include <library/cpp/mediator/messenger.h>

#include <util/stream/output.h>

class TCollectMetricsMessage: public IMessage {
protected:
    IOutputStream& Output;

public:
    TCollectMetricsMessage(IOutputStream& stream)
        : Output(stream)
    {}

    IOutputStream& GetOutputAccess() {
        return Output;
    }
};
