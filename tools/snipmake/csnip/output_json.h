#pragma once
#include "job.h"

#include <util/stream/output.h>

namespace NSnippets {

    class TPassageReply;

    struct TJsonOutput : IOutputProcessor {
        IOutputStream& Out;

        TJsonOutput(IOutputStream& out = Cout);
        static void DoPrint(const TJob& job, const TPassageReply& res, IOutputStream& out);
        void Process(const TJob& job) override;
    };

    struct TDiffJsonOutput : IOutputProcessor {
        IOutputStream& Out;

        TDiffJsonOutput(IOutputStream& out = Cout);
        void Process(const TJob& job) override;
    };
}
