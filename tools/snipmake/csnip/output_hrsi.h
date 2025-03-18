#pragma once
#include "job.h"

namespace NSnippets {

    struct THrSerpItemOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

}
