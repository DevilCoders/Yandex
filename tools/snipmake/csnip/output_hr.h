#pragma once
#include "job.h"

#include <util/stream/output.h>

namespace NSnippets {

    struct TProtobufOutput : IOutputProcessor {
        IOutputStream& Out;

        TProtobufOutput(IOutputStream& out = Cout);
        void Process(const TJob& job) override;
    };

    struct TSrCtxOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

    struct THrCtxOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

}
