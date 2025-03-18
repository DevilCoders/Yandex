#pragma once
#include "job.h"

namespace NSnippets {

    struct TReqUrlOutput : IOutputProcessor {
        bool IdPrefix = false;

        TReqUrlOutput(bool idPrefix);
        void Process(const TJob& job) override;
    };

    struct TBriefOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

    struct TFactorsOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

    struct TNullOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

} //namespace NSnippets
