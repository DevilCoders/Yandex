#pragma once
#include "job.h"

#include <util/generic/ptr.h>
#include <util/stream/output.h>

namespace NSnippets {

    class TLossWordsExplain;

    struct TLossWordsOutput : IOutputProcessor {
        THolder<TLossWordsExplain> Exp;

        TLossWordsOutput(IOutputStream& out = Cout);
        void Process(const TJob& job) override;
        void Complete() override;
    };

}
