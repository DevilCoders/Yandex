#pragma once
#include "job.h"

namespace NSnippets {

    struct TCsvOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

}
