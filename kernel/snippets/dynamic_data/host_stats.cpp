#include "host_stats.h"

#include <util/stream/file.h>
#include <util/string/split.h>

namespace NSnippets {

    THostStats::THostStats(const TString& filename) {
        TFileInput fin(filename);
        Load(&fin);
    }

    THostStats::THostStats(const TBlob& blob) {
        TMemoryInput in(blob.Data(), blob.Size());
        Load(&in);
    }

    void THostStats::InitFromFilename(const TString& filename) {
        TFileInput fin(filename);
        Load(&fin);
    }

    void THostStats::InitFromBlob(const TBlob& blob) {
        TMemoryInput in(blob.Data(), blob.Size());
        Load(&in);
    }

    bool THostStats::Empty() const {
        return Host2Frequency.empty() || !TotalFrequency;
    }

    double THostStats::GetHostWeight(const TStringBuf& host) const {
        static const TString WIKI = "wikipedia.org";
        if (!host.EndsWith(WIKI)) {
            if (auto fptr = MapFindPtr(Host2Frequency, host)) {
                const double fraq = ((double)*fptr) / TotalFrequency;
                if (fraq > 0.00001) {
                    return 1.0 / fraq;
                } else {
                    return std::numeric_limits<double>::max();
                }
            }
        }
        return 0.0;
    }

}
