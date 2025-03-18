#pragma once
#include "job.h"

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/stream/output.h>

namespace NSnippets {

    struct TCtxDiffOutput : IOutputProcessor {
        bool Inverse = false;
        bool WithId = false;
        bool NeedSubId = false;

        TCtxDiffOutput(bool inverse = false, bool withId = false, bool needSubId = false);
        void Process(const TJob& job) override;
    };

    struct TCtxOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

    struct TUniqueCtxOutput : IOutputProcessor {
        typedef THashSet<TString> SetStrok;
        typedef THashMap<TString, SetStrok> Dict;
        Dict UrlToUsrReq;

        void Process(const TJob& job) override;
    };

    struct TICtxOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

    struct TPatchCtxOutput : IOutputProcessor {
        TString Patch;

        TPatchCtxOutput(const TString& patch);
        void Process(const TJob& job) override;
    };


}
