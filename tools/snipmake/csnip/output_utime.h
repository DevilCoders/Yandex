#pragma once
#include "job.h"

#include <util/generic/vector.h>

namespace NSnippets {

    struct TUTimeOutput : IOutputProcessor {
        bool IdSuffix = false;
        TUTimeOutput(bool idSuffix);
        void Process(const TJob& job) override;
    };

    struct TUTimeTableOutput : IOutputProcessor {
        TVector<int> Times;
        void Process(const TJob& job) override;
        void Complete() override;
    };

}
