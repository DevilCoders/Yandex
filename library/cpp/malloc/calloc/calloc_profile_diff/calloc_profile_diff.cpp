#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/on_disk/mms/mapping.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/unordered_map.h>

#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/system/info.h>

struct TCallocStats {
    ui64 Count = 0;
    ui64 Size = 0;
};

Y_DECLARE_PODTYPE(TCallocStats);

using TCallocProfile = NMms::TUnorderedMap<NMms::TMmapped, NMms::TStringType<NMms::TMmapped>, TCallocStats>;


int main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    bool bySize = false;
    opts.AddLongOption("by-size", "Diff by size of allocations instead of count").StoreTrue(&bySize);
    size_t maxRes = 0;
    opts.AddLongOption("max-res", "Max results to print").DefaultValue(100).StoreResult(&maxRes);
    bool canShrink = false;
    opts.AddLongOption("can-shrink", "Allocation size/count may go down temporarily").StoreTrue(&canShrink);
    opts.SetFreeArgsMin(1);
    opts.SetFreeArgsMax(Max<ui32>());
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    const bool singleFile = parsedOpts.GetFreeArgs().size() == 1;

    TVector<NMms::TMapping<TCallocProfile>> callocProfiles;
    for (const TString& filePath : parsedOpts.GetFreeArgs()) {
        callocProfiles.emplace_back(filePath);
    }

    struct TAllocHistory : public TVector<ui64> {
        ui64 Diff() const {
            return size() == 1 ? back() : back() - front();
        }
        bool IsGrowing(const bool canShrink) const {
            if (size() == 1) {
                return true;
            }
            if (canShrink) {
                return back() > front();
            }
            return back() > front() && IsSorted(begin(), end());
        }
    };
    using TDiffStats = THashMap<TStringBuf, TAllocHistory>;
    using TIter = TDiffStats::const_iterator;
    TDiffStats diffStats;
    TAllocHistory allocHistory;
    allocHistory.reserve(callocProfiles.size());
    for (const auto& [stackTrace, _] : *callocProfiles.back()) {
        allocHistory.clear();
        for (const NMms::TMapping<TCallocProfile>& mapping : callocProfiles) {
            if (const TCallocStats* callocStats = MapFindPtr(*mapping, stackTrace)) {
                allocHistory.push_back(bySize ? callocStats->Size : callocStats->Count);
            } else {
                allocHistory.push_back(0);
            }
        }
        if (allocHistory.IsGrowing(canShrink)) {
            diffStats[stackTrace] = allocHistory;
        }
    }

    TVector<TIter> sortedDiff;
    sortedDiff.reserve(diffStats.size());
    for (TIter iter = diffStats.cbegin(); iter != diffStats.cend(); ++iter) {
        sortedDiff.push_back(iter);
    }
    Sort(sortedDiff, [](TIter iter1, TIter iter2) {
        return iter1->second.Diff() > iter2->second.Diff();
    });

    for (size_t i = 0; i < Min(maxRes, sortedDiff.size()); ++i) {
        Cout << "# diff: " << sortedDiff[i]->second.Diff();
        if (!singleFile) {
            Cout << " values:";
            for (const ui64 value : sortedDiff[i]->second) {
                Cout << ' ' << value;
            }
        }
        Cout << '\n';
        Cout << sortedDiff[i]->first << '\n';
    }

    return 0;
}
