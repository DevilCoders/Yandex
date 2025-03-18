#pragma once
#include "job.h"

namespace NSnippets {

    struct TMRConverter : IConverter {
        void Convert(const TContextData& contextData) override;
    };

}
