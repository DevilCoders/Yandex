#pragma once
#include "job.h"

#include <util/generic/string.h>
#include <util/generic/hash_set.h>
#include <util/stream/output.h>

namespace NSnippets {

    struct TDiffOutput : IOutputProcessor {
        size_t Total = 0;
        size_t Same = 0;
        size_t Diff = 0;
        bool Inverse = false;
        bool WantAttrs = false;
        bool NoData = false;
        THashSet<TString> DiffRequestIds;
        THashSet<TString> AllRequestIds;

        TDiffOutput(bool inverse = false, bool wantAttrs = false, bool nodata = false);
        void PrintStats();
        void Process(const TJob& job) override;
        void Complete() override;
    };

    struct TDiffQurlsOutput : IOutputProcessor {
        IOutputStream& Out;

        TDiffQurlsOutput(IOutputStream& out = Cout);
        void Process(const TJob& job) override;
    };

    struct TDiffScoreOutput : IOutputProcessor {
        size_t Total = 0;
        size_t Same = 0;
        size_t Diff = 0;
        size_t Marks[3];

        TDiffScoreOutput();
        void Process(const TJob& job) override;
    };

}
