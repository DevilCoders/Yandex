#pragma once
#include "job.h"

#include <util/stream/output.h>

namespace NSnippets {

    struct TArcOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

}
