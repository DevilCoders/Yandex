#pragma once
#include "job.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NSnippets {

    struct TSerpFormatOutput : IOutputProcessor {
        typedef TVector<TString> TItems;
        typedef THashMap<TString, TItems> TData;
        TData Data;
        TAdaptiveLock Lock;

        void Process(const TJob& job) override;
        void Complete() override;
    };

}
