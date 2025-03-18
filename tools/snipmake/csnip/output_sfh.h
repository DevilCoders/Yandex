#pragma once
#include "job.h"

#include <util/stream/output.h>

namespace NSnippets {

    struct TSfhOutput : IOutputProcessor {
        IOutputStream& Out;

        TSfhOutput(IOutputStream& out = Cout);
        void Process(const TJob& job) override;
    };

}
