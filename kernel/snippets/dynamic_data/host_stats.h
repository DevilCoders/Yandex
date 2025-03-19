#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/ysaveload.h>
#include <util/memory/blob.h>

namespace NSnippets {

    class THostStats {
    private:
        THashMap<TString, ui64> Host2Frequency;
        ui64 TotalFrequency = 0;

    public:
        THostStats() {}
        THostStats(const TBlob& blob);
        THostStats(const TString& filename);
        void InitFromFilename(const TString& filename);
        void InitFromBlob(const TBlob& blob);

        bool Empty() const;
        double GetHostWeight(const TStringBuf& host) const;
        Y_SAVELOAD_DEFINE(Host2Frequency, TotalFrequency);
    };

}
