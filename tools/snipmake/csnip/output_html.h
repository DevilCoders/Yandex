#pragma once
#include "job.h"

#include <util/stream/output.h>

namespace NSnippets {

    class TPassageReply;

    struct THtmlOutput : IOutputProcessor {
        IOutputStream& Out;

        THtmlOutput(IOutputStream& out = Cout);
        static void DoPrint(const TJob& job, const TPassageReply& res, IOutputStream& out, bool diffInVisibleParts);
        static void DoPrintMailRu(const TJob& job, const TPassageReply& res, IOutputStream& out);
        void Process(const TJob& job) override;
        void Complete() override;
    };

    struct TDiffHtmlOutput : IOutputProcessor {
        IOutputStream& Out;

        TDiffHtmlOutput(IOutputStream& out = Cout);
        void Process(const TJob& job) override;
        void Complete() override;
    };
}
